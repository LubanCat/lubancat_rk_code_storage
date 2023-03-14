# 参考教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/ai/resnet18_pytorch.html
import numpy as np
import cv2
import os
from rknnlite.api import RKNNLite

IMG_PATH = '2_67.jpg'
RKNN_MODEL = './resnet18.rknn'
img_height = 32
img_width = 32
class_names = ["plane","car","bird","cat","deer","dog","frog","horse","ship","truck"]

# Create RKNN object
rknn_lite = RKNNLite()

# load RKNN model
print('--> Load RKNN model')
ret = rknn_lite.load_rknn(RKNN_MODEL)
if ret != 0:
    print('Load RKNN model failed')
    exit(ret)
print('done')

# Init runtime environment
print('--> Init runtime environment')
ret = rknn_lite.init_runtime()
if ret != 0:
    print('Init runtime environment failed!')
    exit(ret)
print('done')

# load image
img = cv2.imread(IMG_PATH)
img = cv2.resize(img,(32,32))
img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
img = np.expand_dims(img, 0)


# runing model
print('--> Running model')
outputs = rknn_lite.inference(inputs=[img])
print("result: ", outputs)
print(
    "This image most likely belongs to {}."
    .format(class_names[np.argmax(outputs)])
)

rknn_lite.release()

# 详细使用请参考野火教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/index.html