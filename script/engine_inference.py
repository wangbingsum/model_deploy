import tensorrt as trt
import pycuda.driver as cuda
import pycuda.autoinit
import numpy as np
import cv2
from torchvision import transforms

# ===================== 硬编码固定配置（与静态ResNet50引擎完全一致，不可修改）=====================
ENGINE_PATH = "./model/resnet50_fp16.engine"  # 静态TRT引擎路径
INPUT_SHAPE = (1, 3, 224, 224)                # 固定输入形状 (B,C,H,W)
OUTPUT_SHAPE = (1, 1000)                      # 固定输出形状 (B,num_classes)
MEAN = [0.485, 0.456, 0.406]                  # 预处理均值（与训练/引擎构建一致）
STD = [0.229, 0.224, 0.225]                   # 预处理标准差（与训练/引擎构建一致）
TEST_IMAGE = "./input/dog.jpg"                # 测试图像路径

# ===================== 1. 图像预处理（输出固定形状，与引擎严格对齐）=====================
def preprocess(img_path):
    """极简预处理，直接输出INPUT_SHAPE固定形状"""
    img = cv2.imread(img_path)
    if img is None:
        raise FileNotFoundError(f"无法读取图像：{img_path}，请检查路径是否正确")
    # BGR→RGB（与PyTorch训练/引擎构建一致）
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    # 固定预处理流水线，不可修改
    transform = transforms.Compose([
        transforms.ToPILImage(),
        transforms.Resize((256, 256)),
        transforms.CenterCrop((224, 224)),
        transforms.ToTensor(),
        transforms.Normalize(mean=MEAN, std=STD)
    ])
    # 生成固定形状输入 (1,3,224,224)
    input_data = transform(img).numpy()[np.newaxis, ...].astype(np.float32)
    return input_data

# ===================== 2. 同步推理核心类（无异步、无get_binding_shape、纯固定形状）=====================
class SyncTRTInfer:
    def __init__(self):
        # 1. 加载静态引擎（极简实现，仅打印错误日志）
        self.engine = self._load_engine()
        # 2. 创建推理上下文（静态引擎直接创建，无需任何形状设置）
        self.context = self.engine.create_execution_context()
        # 3. 硬编码分配内存（无形状查询，直接按固定形状计算）
        self._allocate_fixed_memory()
        # 4. 固定绑定地址（ResNet50静态引擎默认：输入0、输出1）
        self.bindings = [int(self.d_input), int(self.d_output)]
        print(f"[INFO] 静态引擎初始化完成 | 输入：{INPUT_SHAPE} | 输出：{OUTPUT_SHAPE} | 同步推理模式")

    def _load_engine(self):
        """加载TRT静态引擎，极简反序列化"""
        try:
            with open(ENGINE_PATH, "rb") as f:
                engine_data = f.read()
            # 日志器设为ERROR级别，仅打印严重错误，简化输出
            runtime = trt.Runtime(trt.Logger(trt.Logger.ERROR))
            engine = runtime.deserialize_cuda_engine(engine_data)
            if not engine:
                raise RuntimeError("引擎反序列化失败，请检查引擎文件是否损坏/版本是否匹配")
            return engine
        except Exception as e:
            raise RuntimeError(f"引擎加载失败：{e}（请确认引擎是TRT10.10.0.31生成的静态ResNet50引擎）")

    def _allocate_fixed_memory(self):
        """硬编码分配CPU锁定内存+GPU设备内存，无任何形状查询API"""
        # 计算输入/输出内存体积（总元素数）
        input_volume = trt.volume(INPUT_SHAPE)
        output_volume = trt.volume(OUTPUT_SHAPE)
        # 分配CPU主机锁定内存（提升同步拷贝速度，TRT官方推荐）
        self.h_input = cuda.pagelocked_empty(input_volume, np.float32)
        self.h_output = cuda.pagelocked_empty(output_volume, np.float32)
        # 分配GPU设备内存（按字节数分配，直接用nbytes更简洁）
        self.d_input = cuda.mem_alloc(self.h_input.nbytes)
        self.d_output = cuda.mem_alloc(self.h_output.nbytes)

    def infer(self, input_data):
        """
        核心同步推理方法（无异步流、无同步操作，一步到位）
        :param input_data: 预处理后的输入数据，形状必须为INPUT_SHAPE，类型float32
        :return: 推理结果，形状为OUTPUT_SHAPE
        """
        # 强校验输入形状（避免传错数据）
        if input_data.shape != INPUT_SHAPE or input_data.dtype != np.float32:
            raise ValueError(f"输入数据不匹配 | 预期形状：{INPUT_SHAPE}、类型：float32 | 实际：{input_data.shape}、{input_data.dtype}")
        
        # 同步执行：CPU→GPU拷贝 → 推理 → GPU→CPU拷贝（无任何异步，顺序执行）
        np.copyto(self.h_input, input_data.ravel())  # CPU数据拷贝到锁定内存
        cuda.memcpy_htod(self.d_input, self.h_input) # 同步：CPU→GPU
        self.context.execute_v2(bindings=self.bindings) # 同步推理（注意：同步用execute_v2，非execute_async_v2）
        cuda.memcpy_dtoh(self.h_output, self.d_output) # 同步：GPU→CPU
        
        # 恢复固定输出形状并返回
        return self.h_output.reshape(OUTPUT_SHAPE)

    def release(self):
        """同步释放所有资源，无冗余操作"""
        self.d_input.free()  # 释放GPU输入内存
        self.d_output.free() # 释放GPU输出内存
        self.engine = None   # 置空引擎
        self.context = None  # 置空上下文
        print("[INFO] 所有TRT/CUDA资源已同步释放")

# ===================== 3. 主推理流程（极简同步调用，一键运行）=====================
if __name__ == "__main__":
    try:
        # 1. 初始化同步推理器
        trt_infer = SyncTRTInfer()
        # 2. 预处理图像（输出固定形状）
        input_data = preprocess(TEST_IMAGE)
        print(f"[INFO] 图像预处理完成 | 形状：{input_data.shape} | 类型：{input_data.dtype}")
        # 3. 执行同步推理
        print("[INFO] 开始执行同步推理...")
        output = trt_infer.infer(input_data)
        # 4. 解析推理结果（ResNet50 1000分类通用逻辑）
        pred_label = np.argmax(output)  # 概率最大的类别索引
        pred_prob = np.max(output)      # 最大类别置信度
        # 打印结果
        print("\n[✅ 推理结果]")
        print(f"类别索引：{pred_label}")
        print(f"置信度：{pred_prob:.4f} ({pred_prob * 100:.2f}%)")
    except Exception as e:
        print(f"\n[❌ 推理失败] {e}")
    finally:
        # 无论是否报错，均释放资源
        if 'trt_infer' in locals():
            trt_infer.release()