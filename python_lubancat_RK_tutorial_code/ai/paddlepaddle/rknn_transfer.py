import numpy as np
from rknn.api import RKNN
import cv2

INPUT_SIZE = 28
IMG_PATH = '9.jpg'

# 创建RKNN执行对象,打印信息
#rknn = RKNN(verbose=True)
rknn = RKNN()

# 配置模型输入，用于NPU对数据输入的预处理
# mean_values：输入的均值
# std_values：输入的归一化值
# target_platform: 目标平台
# 更多设置参考下RKNN toolkit2用户指导手册
print('--> Config model')
rknn.config(mean_values=[127.5], std_values=[127.5], target_platform='rk3568')
#rknn.config(target_platform='rk3568')

# 加载ONNX模型
# inputs指定模型中的输入节点
# outputs指定模型中输出节点
# input_size_list指定模型输入的大小
print('--> Loading model')
rknn.load_onnx(model='./onnx/lenet.onnx')
print('done')

print('--> Building model')
ret = rknn.build(do_quantization=False)
print('done')

# 初始化RKNN运行环境，运行测试
print('--> Init runtime environment')
ret = rknn.init_runtime()
#    if ret != 0:
#       print('Init runtime environment failed')
#        exit(ret)
print('done')

# 图像
img = cv2.imread(IMG_PATH)
img = cv2.cvtColor(img, cv2.COLOR_RGB2GRAY)
img = cv2.resize(img,(28,28))
img = np.expand_dims(img, 0)

# 推理
print('--> Running model')
#outputs = rknn.inference(inputs=[img], data_format='nchw')
print("result: ", outputs)
print("本次预测数字是:", np.argmax(outputs))

# 导出rknn模型文件
print('--> Export rknn model')
rknn.export_rknn('./lenet.rknn')
print('done')

# 释放rknn
rknn.release()

