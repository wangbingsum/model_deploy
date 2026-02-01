# 适配你的环境：ONNX转TensorRT INT8引擎
trtexec --onnx=./model/resnet50.onnx `
        --saveEngine=./model/resnet50_int8.engine `
        --int8 `
        --calib=./calibration_data/calib_data.npy `  # 校准集路径
        --workspace=8192 `  # 8GB工作空间（复杂模型建议增大）
        --batch=1 `         # 与校准集批次一致
        --verbose           # 打印详细日志，排查问题