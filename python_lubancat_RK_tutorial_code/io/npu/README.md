## npu目录结构

```
./
├── librknnrt.so
├── README.md
├── rknn_toolkit_lite2
│   ├── docs
│   │   ├── change_log.txt
│   │   ├── Rockchip_User_Guide_RKNN_Toolkit_Lite2_V1.4.0_CN.pdf
│   │   └── Rockchip_User_Guide_RKNN_Toolkit_Lite2_V1.4.0_EN.pdf
│   ├── examples
│   │   └── inference_with_lite
│   │       ├── resnet18_for_rk356x.rknn
│   │       ├── resnet18_for_rk3588.rknn
│   │       ├── space_shuttle_224.jpg
│   │       └── test.py
│   └── packages
│       ├── rknn_toolkit_lite2-1.4.0-cp37-cp37m-linux_aarch64.whl
│       ├── rknn_toolkit_lite2-1.4.0-cp39-cp39-linux_aarch64.whl
│       └── rknn_toolkit_lite2_1.4.0_packages.md5sum
└── yolov5
    ├── bus.jpg
    ├── test.py
    └── yolov5s.rknn

6 directories, 15 files
```

## 说明

使用LubancatRK系列板卡（RK3566，RK3568），板卡环境是基于Python 3.7.3版本(镜像系统是Debian10)进行实验及讲解，PC环境使用ubuntu20.04（python3.8.10）。

配套教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/circuit/rknn.html，教程编写时的RKNN-Toolkit2是1.4.0版本。

rknn_toolkit_lite2源文件也可以直接从官方github获取，https://github.com/rockchip-linux/rknn-toolkit2/tree/master/rknn_toolkit_lite2

librknnrt.so也可以从官方github获取，https://github.com/rockchip-linux/rknpu2/tree/master/runtime/RK356X/Linux/librknn_api/aarch64

