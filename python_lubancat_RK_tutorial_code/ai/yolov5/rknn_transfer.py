# 使用请参考教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/index.html
import os
import cv2
import traceback
import numpy as np
from rknn.api import RKNN

ONNX_MODEL = 'yolov5s.onnx'
RKNN_MODEL = 'yolov5s.rknn'
IMG_PATH = './bus.jpg'
DATASET = './dataset.txt'
PYTORCH_MODEL = './yolov5s_relu.pt'
IMG_SIZE = 640

if __name__ == '__main__':
    # 创建RKNN
    # 如果测试遇到问题，请开启verbose=True，查看调试信息。
    #rknn = RKNN(verbose=True)
    rknn = RKNN()
    
    # Set inputs
    img = cv2.imread(IMG_PATH)
    # img, ratio, (dw, dh) = letterbox(img, new_shape=(IMG_SIZE, IMG_SIZE))
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
    
    # pre-process config
    print('--> Config model')
    # 
    rknn.config(mean_values=[[0, 0, 0]], std_values=[[255, 255, 255]], target_platform="rk3568")
    print('done')
    
    # Load pytorch model
    print('--> Loading model')
    ret = rknn.load_pytorch(model=PYTORCH_MODEL, input_size_list=[[1,3,IMG_SIZE,IMG_SIZE]])
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')
        
    # Load ONNX model
    # print('--> Loading model')
    # ret = rknn.load_onnx(model=ONNX_MODEL)
    # if ret != 0:
        # print('Load model failed!')
        # exit(ret)
    # print('done')

    # 构建模型,默认开启了量化
    print('--> Building model')
    ret = rknn.build(do_quantization=True, dataset=DATASET)
    #ret = rknn.build(do_quantization=False)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')
    
    # Accuracy analysis
    #print('--> Accuracy analysis')
    #Ret = rknn.accuracy_analysis(inputs=[img])
    #if ret != 0:
    #    print('Accuracy analysis failed!')
    #    exit(ret)
    #print('done')

    # 导出RKNN模型
    print('--> Export rknn model')
    ret = rknn.export_rknn(RKNN_MODEL)
    #if ret != 0:
    #    print('Export rknn model failed!')
    #    exit(ret)
    print('done')
    
    # Init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime()
    # ret = rknn.init_runtime()
    ret = rknn.init_runtime(target='rk3568', device_id='192.168.103.115:5555', perf_debug=True)
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
    print('done')
    
    # 调试，模型性能进行评估
    rknn.eval_perf(inputs=[img], is_print=True)
    
    rknn.release()
