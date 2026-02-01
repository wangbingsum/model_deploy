## 项目管理

    - pip安装国内源

        pip install 包名 -i https://pypi.tuna.tsinghua.edu.cn/simple

    - trt模型导出

        trtexec --onnx=./model/resnet50.onnx --saveEngine=./model/resnet50_fp16.engine --fp16

    - trt模型运行(加载测试)

        trtexec --loadEngine=.\model\resnet50_fp16.engine

## 项目开发

    - [ ] 1. 线程池
      - [x] 1.1 线程池的基本原理(面试的时候需要用到)
      - [ ] 1.2 线程池的C++实现
    - [ ] 2. 内存池
    - [ ] 3. 模型部署: resnet50, yolov5, CLIP, Qwen-VL 
      - [ ] 3.1 基本的模型导出为onnx, 再转为trt模型, 需要做8bit量化, 比较量化精度损失, 做算子融合, 图优化, 高性能算子实现
    - [ ] 4. C++ 11面试相关,要看一个视频,总结一下面试要点
    - [ ] 5. C++ 高性能优化,要看一部分算法优化相关的视频
    - [ ] 6. 列一下常用的C++库,展示一下项目深度