# 详细请参考教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/ai/paddlepaddle.html
import numpy as np
import cv2
from rknnlite.api import RKNNLite

IMG_PATH = 'test1.jpg'
RKNN_MODEL = 'lenet.rknn'

# Create RKNN object
rknn_lite = RKNNLite()

# 加载rknn模型
print('--> Load RKNN model')
ret = rknn_lite.load_rknn(RKNN_MODEL)
if ret != 0:
    print('Load RKNN model failed')
    exit(ret)
print('done')

# 初始化运行环境
print('--> Init runtime environment')
ret = rknn_lite.init_runtime()
if ret != 0:
    print('Init runtime environment failed!')
    exit(ret)
print('done')

# 图像
img = cv2.imread(IMG_PATH)
img_resize = cv2.resize(img,(28,28))
img_gray = cv2.cvtColor(img_resize, cv2.COLOR_RGB2GRAY)
im = np.array(img_gray).reshape(1, -1).astype(np.float32)
im = 1 - im / 255

# 推理
print('--> Running model')
outputs = rknn_lite.inference(inputs=[im])
print("result: ", outputs)
print("本次识别的数字是:", np.argmax(outputs))

rknn_lite.release()

