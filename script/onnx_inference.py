import onnxruntime as ort
import numpy as np
import cv2
from torchvision import transforms

# ===================== 配置参数（根据你的模型修改）=====================
INPUT_SHAPE = (3, 224, 224)  # 模型输入形状 (C, H, W)
BATCH_SIZE = 1  # 批次大小，需与引擎构建时一致
# 预处理参数（与训练/校准完全一致）
MEAN = [0.485, 0.456, 0.406]
STD = [0.229, 0.224, 0.225]
# 推理设备：0为GPU编号（单GPU直接用0）
DEVICE_ID = 0

def preprocess_image(image_path):
    """
    图像预处理：从本地图片→RGB→Resize→归一化→NCHW格式→FP32数组
    :param image_path: 本地图像路径
    :return: 预处理后的输入数据 (B, C, H, W)，FP32类型
    """
    # 读取图像并转换为RGB（OpenCV默认BGR）
    img = cv2.imread(image_path)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB) # type: ignore
    # 定义预处理流程（与训练/ONNX导出一致）
    transform = transforms.Compose([
        transforms.ToPILImage(),
        transforms.Resize((256, 256)),  # 先resize到256×256
        transforms.CenterCrop(INPUT_SHAPE[1:]),  # 中心裁剪到224×224
        transforms.ToTensor(),  # 转为[C, H, W]，值范围0~1
        transforms.Normalize(mean=MEAN, std=STD)  # 归一化
    ])
    # 执行预处理并扩展批次维度 (C, H, W) → (1, C, H, W)
    img_tensor = transform(img)
    input_data = np.expand_dims(img_tensor.numpy(), axis=0).astype(np.float32)
    return input_data

def main():
    onnx_path = "./model/resnet50.onnx"
    test_image_path = "./input/dog.jpg"
    input_data = preprocess_image(test_image_path)

    # 加载ONNX模型
    ort_sess = ort.InferenceSession(onnx_path, providers=['CUDAExecutionProvider'])
    # 推理
    onnx_output = ort_sess.run(None, {"input": input_data})[0]
    # 解析结果
    onnx_label = np.argmax(onnx_output, axis=1)[0] # type: ignore
    onnx_prob = np.max(onnx_output, axis=1)[0] # type: ignore
    print(f"ONNX模型预测：类别{onnx_label}，概率{onnx_prob:.4f}")

if __name__ == "__main__":
    main()
