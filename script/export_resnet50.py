import torch
import torchvision.models as models

# 1. 加载PyTorch官方预训练ResNet50模型（eval模式，关闭批量归一化/ dropout）
model = models.resnet50(pretrained=True)
model.eval()  # 必须设置，否则ONNX模型推理结果会出错

# 2. 创建虚拟输入（匹配模型输入尺寸，NCHW格式，可自定义batch_size，如[4,3,224,224]）
dummy_input = torch.randn(1, 3, 224, 224)  # 1个样本、3通道、224×224

# 3. 导出为ONNX模型
torch.onnx.export(
    model=model,                # 待转换的PyTorch模型
    args=(dummy_input, ),           # 虚拟输入（确定模型计算图）
    f="model/resnet50_pytorch.onnx",  # 输出ONNX模型文件名
    input_names=["input"],      # 输入节点名称（方便后续推理调用）
    output_names=["output"],    # 输出节点名称
    opset_version=12            # ONNX算子集版本（推荐11-13，兼容性好）
)

print("ResNet50 ONNX模型转换完成，保存为：resnet50_pytorch.onnx")