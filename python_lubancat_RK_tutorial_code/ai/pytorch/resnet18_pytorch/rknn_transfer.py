# 参考教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/ai/resnet18_pytorch.html
import numpy as np
import cv2
from rknn.api import RKNN
import torchvision.models as models
import torch
import os

def softmax(x):
    return np.exp(x)/sum(np.exp(x))

def torch_version():
    import torch
    torch_ver = torch.__version__.split('.')
    torch_ver[2] = torch_ver[2].split('+')[0]
    return [int(v) for v in torch_ver]

if __name__ == '__main__':

    if torch_version() < [1, 9, 0]:
        import torch
        print("Your torch version is '{}', in order to better support the Quantization Aware Training (QAT) model,\n"
              "Please update the torch version to '1.9.0' or higher!".format(torch.__version__))
        exit(0)

    MODEL = './resnet18.onnx'

    # Create RKNN object
    rknn = RKNN(verbose=True)

    # Pre-process config
    print('--> Config model')
    rknn.config(mean_values=[127.5, 127.5, 127.5], std_values=[255, 255, 255], target_platform='rk3568')
    #rknn.config(mean_values=[123.675, 116.28, 103.53], std_values=[58.395, 58.395, 58.395], target_platform='rk3568')
    #rknn.config(mean_values=[125.307, 122.961, 113.8575], std_values=[51.5865, 50.847, 51.255], target_platform='rk3568')
    print('done')

    # Load model
    print('--> Loading model')
    #ret = rknn.load_pytorch(model=model, input_size_list=input_size_list)
    ret = rknn.load_onnx(model=MODEL)
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=False)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # Export rknn model
    print('--> Export rknn model')
    ret = rknn.export_rknn('./resnet_18_100.rknn')
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print('done')

    # Set inputs
    img = cv2.imread('./0_125.jpg')
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img,(32,32))
    #img = np.expand_dims(img, 0)
    
    # Init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime()
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
    print('done')

    # Inference
    print('--> Running model')
    outputs = rknn.inference(inputs=[img])
    np.save('./pytorch_resnet18_qat_0.npy', outputs[0])
    #show_outputs(softmax(np.array(outputs[0][0])))
    print(outputs)
    print('done')

    rknn.release()
