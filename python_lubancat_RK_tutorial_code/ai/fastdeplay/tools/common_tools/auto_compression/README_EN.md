# FastDeploy One-Click Model Auto Compression

FastDeploy, based on PaddleSlim's Auto Compression Toolkit(ACT), provides developers with a one-click model auto compression tool that supports post-training quantization and knowledge distillation training.
We take the Yolov5 series as an example to demonstrate how to install and execute FastDeploy's one-click model auto compression.

## 1.Install

### Environment Dependencies

1.Install PaddlePaddle 2.4 version
```
https://www.paddlepaddle.org.cn/install/quick?docurl=/documentation/docs/zh/develop/install/pip/linux-pip.html
```

2.Install PaddleSlim 2.4 version
```bash
pip install paddleslim==2.4.0
```

### Install Fastdeploy Auto Compression Toolkit
FastDeploy One-Click Model Automation compression does not require a separate installation, users only need to properly install the [FastDeploy Toolkit](../../README.md)


## 2. How to Use

### Demo for One-Click Auto Compression Toolkit

Fastdeploy Auto Compression can include multiple strategies, At present, offline quantization and quantization distillation are mainly used for training. The following will introduce how to use it from two strategies, offline quantization and quantitative distillation.

#### Offline Quantization

##### 1. Prepare models and Calibration data set

Developers need to prepare the model to be quantized and the Calibration dataset on their own.
In this demo, developers can execute the following command to download the yolov5s.onnx model to be quantized and calibration data set.

```shell
# Download yolov5.onnx
wget https://paddle-slim-models.bj.bcebos.com/act/yolov5s.onnx

# Download dataset. This Calibration dataset is the first 320 images from COCO val2017
wget https://bj.bcebos.com/paddlehub/fastdeploy/COCO_val_320.tar.gz
tar -xvf COCO_val_320.tar.gz
```

##### 2. Run fastdeploy compress command to compress the model

The following command is to quantize the yolov5s model, if developers want to quantize other models, replace the config_path with other model configuration files in the configs folder.

```shell
fastdeploy compress --config_path=./configs/detection/yolov5s_quant.yaml --method='PTQ' --save_dir='./yolov5s_ptq_model/'
```

[notice] PTQ is short for post-training quantization

##### 3. Parameters

To complete the quantization, developers only need to provide a customized model config file, specify the quantization method, and the path to save the quantized model.

| Parameter     | Description                                                                                                   |
| ------------- | ------------------------------------------------------------------------------------------------------------- |
| --config_path | Quantization profiles needed for one-click quantization.[Configs](./configs/README.md)                        |
| --method      | Quantization method selection, PTQ for post-training quantization, QAT for quantization distillation training |
| --save_dir    | Output of quantized model paths, which can be deployed directly in FastDeploy                                 |

#### Quantized distillation training

##### 1.Prepare the model to be quantized and the training data set

FastDeploy currently supports quantized distillation training only for images without annotation. It does not support evaluating model accuracy during training.
The datasets are images from inference application, and the number of images is determined by the size of the dataset, covering all deployment scenarios as much as possible. In this demo, we prepare the first 320 images from the COCO2017 validation set for users.
Note: If users want to obtain a more accurate quantized model through quantized distillation training, feel free to prepare more data and train more rounds.

```shell
# Download yolov5.onnx
wget https://paddle-slim-models.bj.bcebos.com/act/yolov5s.onnx

# Download dataset. This Calibration dataset is the first 320 images from COCO val2017
wget https://bj.bcebos.com/paddlehub/fastdeploy/COCO_val_320.tar.gz
tar -xvf COCO_val_320.tar.gz
```

##### 2.Use fastdeploy compress command to compress models

The following command is to quantize the yolov5s model, if developers want to quantize other models, replace the config_path with other model configuration files in the configs folder.

```shell
# Please specify the single card GPU before training, otherwise it may get stuck during the training process.
export CUDA_VISIBLE_DEVICES=0
fastdeploy compress --config_path=./configs/detection/yolov5s_quant.yaml --method='QAT' --save_dir='./yolov5s_qat_model/'
```

##### 3.Parameters

To complete the quantization, developers only need to provide a customized model config file, specify the quantization method, and the path to save the quantized model.

| Parameter     | Description                                                                                                   |
| ------------- | ------------------------------------------------------------------------------------------------------------- |
| --config_path | Quantization profiles needed for one-click quantization.[Configs](./configs/README.md)                        |
| --method      | Quantization method selection, PTQ for post-training quantization, QAT for quantization distillation training |
| --save_dir    | Output of quantized model paths, which can be deployed directly in FastDeploy                                 |

## 3. FastDeploy One-Click Model Auto Compression Config file examples
FastDeploy currently provides users with compression [config](./configs/) files of multiple models, and the corresponding FP32 model, Users can directly download and experience it.

| Config file                | FP32 model | Note                                                       |
| -------------------- | ------------------------------------------------------------ |----------------------------------------- |
| [mobilenetv1_ssld_quant](./configs/classification/mobilenetv1_ssld_quant.yaml)      | [mobilenetv1_ssld](https://bj.bcebos.com/paddlehub/fastdeploy/MobileNetV1_ssld_infer.tgz)           |           |
| [resnet50_vd_quant](./configs/classification/resnet50_vd_quant.yaml)      |   [resnet50_vd](https://bj.bcebos.com/paddlehub/fastdeploy/ResNet50_vd_infer.tgz)          |     |
| [efficientnetb0_quant](./configs/classification/efficientnetb0_quant.yaml)      |   [efficientnetb0](https://bj.bcebos.com/paddlehub/fastdeploy/EfficientNetB0_small_infer.tgz)          |     |
| [mobilenetv3_large_x1_0_quant](./configs/classification/mobilenetv3_large_x1_0_quant.yaml)      |   [mobilenetv3_large_x1_0](https://bj.bcebos.com/paddlehub/fastdeploy/MobileNetV3_large_x1_0_ssld_infer.tgz)          |     |
| [pphgnet_tiny_quant](./configs/classification/pphgnet_tiny_quant.yaml)      |   [pphgnet_tiny](https://bj.bcebos.com/paddlehub/fastdeploy/PPHGNet_tiny_ssld_infer.tgz)          |     |
| [pplcnetv2_base_quant](./configs/classification/pplcnetv2_base_quant.yaml)      |   [pplcnetv2_base](https://bj.bcebos.com/paddlehub/fastdeploy/PPLCNetV2_base_infer.tgz)          |     |
| [yolov5s_quant](./configs/detection/yolov5s_quant.yaml)       |   [yolov5s](https://paddle-slim-models.bj.bcebos.com/act/yolov5s.onnx)         |     |
| [yolov6s_quant](./configs/detection/yolov6s_quant.yaml)       |  [yolov6s](https://paddle-slim-models.bj.bcebos.com/act/yolov6s.onnx)          |     |
| [yolov7_quant](./configs/detection/yolov7_quant.yaml)        | [yolov7](https://paddle-slim-models.bj.bcebos.com/act/yolov7.onnx)           |      |
| [ppyoloe_withNMS_quant](./configs/detection/ppyoloe_withNMS_quant.yaml)       |  [ppyoloe_l](https://bj.bcebos.com/v1/paddle-slim-models/act/ppyoloe_crn_l_300e_coco.tar)    | Support PPYOLOE's s,m,l,x series models, export the model normally when exporting the model from PaddleDetection, do not remove NMS |
| [ppyoloe_plus_withNMS_quant](./configs/detection/ppyoloe_plus_withNMS_quant.yaml)       |  [ppyoloe_plus_s](https://bj.bcebos.com/paddlehub/fastdeploy/ppyoloe_plus_crn_s_80e_coco.tar)    | Support PPYOLOE+'s s,m,l,x series models, export the model normally when exporting the model from PaddleDetection, do not remove NMS |
| [pp_liteseg_quant](./configs/segmentation/pp_liteseg_quant.yaml)    |   [pp_liteseg](https://bj.bcebos.com/paddlehub/fastdeploy/PP_LiteSeg_T_STDC1_cityscapes_without_argmax_infer.tgz)        |       |
| [deeplabv3_resnet_quant](./configs/segmentation/deeplabv3_resnet_quant.yaml)    |   [deeplabv3_resnet101](https://bj.bcebos.com/paddlehub/fastdeploy/Deeplabv3_ResNet101_OS8_cityscapes_without_argmax_infer.tgz)        |       |
| [fcn_hrnet_quant](./configs/segmentation/fcn_hrnet_quant.yaml)    |   [fcn_hrnet](https://bj.bcebos.com/paddlehub/fastdeploy/FCN_HRNet_W18_cityscapes_without_argmax_infer.tgz)        |       |
| [unet_quant](./configs/segmentation/unet_quant.yaml)    |   [unet](https://bj.bcebos.com/paddlehub/fastdeploy/Unet_cityscapes_without_argmax_infer.tgz)        |       |      |


## 3. Deploy quantized models on FastDeploy

Once obtained the quantized model, developers can deploy it on FastDeploy. Please refer to the following docs for more details

- [YOLOv5 Quantized Model Deployment](../../../examples/vision/detection/yolov5/quantize/)

- [YOLOv6 Quantized Model Deployment](../../../examples/vision/detection/yolov6/quantize/)

- [YOLOv7 Quantized Model Deployment](../../../examples/vision/detection/yolov7/quantize/)

- [PadddleClas Quantized Model Deployment](../../../examples/vision/classification/paddleclas/quantize/)

- [PadddleDetection Quantized Model Deployment](../../../examples/vision/detection/paddledetection/quantize/)

- [PadddleSegmentation Quantized Model Deployment](../../../examples/vision/segmentation/paddleseg/quantize/)
