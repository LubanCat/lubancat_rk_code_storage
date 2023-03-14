import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN

#ONNX_MODEL = 'yolov5s.onnx'
RKNN_MODEL = 'yolov5s.rknn'
IMG_PATH = './bus.jpg'
DATASET = './dataset.txt'

QUANTIZE_ON = True
IMG_SIZE = 640


if __name__ == '__main__':

    # 创建RKNN
    # 如果测试遇到问题，请开启verbose=True，查看调试信息。
    # rknn = RKNN(verbose=True)
    rknn = RKNN()
    
    # pre-process config
    print('--> Config model')
    # 连板调试需要指定target_platform
    rknn.config(mean_values=[[0, 0, 0]], std_values=[[255, 255, 255]], target_platform="rk3568")
    print('done')

    # 加载模型
    print('--> Loading model')
    ret = rknn.load_onnx(model=ONNX_MODEL)
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # 构建模型,开启量化
    print('--> Building model')
    ret = rknn.build(do_quantization=QUANTIZE_ON, dataset=DATASET)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # 导出RKNN模型
    # print('--> Export rknn model')
    # ret = rknn.export_rknn(RKNN_MODEL)
    # if ret != 0:
    #    print('Export rknn model failed!')
    #    exit(ret)
    # print('done')

    # 设置目标rk3568、rk3566、rk3588，连接板端调试，设置device_id（根据adb devices实际连接获得），perf_debug开启debug
    print('--> Init runtime environment')
    ret = rknn.init_runtime(target='rk3568', device_id='192.168.103.121:5555', perf_debug=True)
    # ret = rknn.init_runtime('rk3566')
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
    print('done')

    # 输入图片
    img = cv2.imread(IMG_PATH)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
    
    # 模型性能进行评估
    rknn.eval_perf(inputs=[img], is_print=True)
    
    
    # Inference
    #print('--> Running model')
    #outputs = rknn.inference(inputs=[img])
    #np.save('./onnx_yolov5_0.npy', outputs[0])
    #np.save('./onnx_yolov5_1.npy', outputs[1])
    #np.save('./onnx_yolov5_2.npy', outputs[2])
    #print('done')

    # post process
    #input0_data = outputs[0]
    #input1_data = outputs[1]
    #input2_data = outputs[2]

    #input0_data = input0_data.reshape([3, -1]+list(input0_data.shape[-2:]))
    #input1_data = input1_data.reshape([3, -1]+list(input1_data.shape[-2:]))
    #input2_data = input2_data.reshape([3, -1]+list(input2_data.shape[-2:]))

    #input_data = list()
    #input_data.append(np.transpose(input0_data, (2, 3, 0, 1)))
    #input_data.append(np.transpose(input1_data, (2, 3, 0, 1)))
    #input_data.append(np.transpose(input2_data, (2, 3, 0, 1)))

    # boxes, classes, scores = yolov5_post_process(input_data)

    # img_1 = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
    # if boxes is not None:
    #   draw(img_1, boxes, scores, classes)
    
    # show output
    # cv2.imwrite("out.jpg", img_1)
    # cv2.imshow("post process result", img_1)
    # cv2.waitKey(0)
    # cv2.destroyAllWindows()

    rknn.release()
