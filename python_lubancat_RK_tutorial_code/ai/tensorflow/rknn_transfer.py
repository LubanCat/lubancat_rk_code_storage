import numpy as np
import cv2
from rknn.api import RKNN
import tensorflow as tf

img_height = 180
img_width = 180
IMG_PATH = 'test.jpg'
class_names = ['daisy', 'dandelion', 'roses', 'sunflowers', 'tulips']

if __name__ == '__main__':

    # Create RKNN object
    #rknn = RKNN(verbose='Debug')
    rknn = RKNN()

    # Pre-process config
    print('--> Config model')
    rknn.config(mean_values=[0, 0, 0], std_values=[255, 255, 255], target_platform='rk3568')
    print('done')

    # Load model
    print('--> Loading model')
    ret = rknn.load_tflite(model='model.tflite')
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=False)
    #ret = rknn.build(do_quantization=True,dataset='./dataset.txt')
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # Export rknn model
    print('--> Export rknn model')
    ret = rknn.export_rknn('./model.rknn')
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print('done')
    

#Init runtime environment
print('--> Init runtime environment')
ret = rknn.init_runtime()
#    if ret != 0:
#        print('Init runtime environment failed!')
#        exit(ret)
print('done')

img = cv2.imread(IMG_PATH)
img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
img = cv2.resize(img,(180,180))
img = np.expand_dims(img, 0)

#print('--> Accuracy analysis')
#rknn.accuracy_analysis(inputs=['./test.jpg'])
#print('done')

print('--> Running model')
outputs = rknn.inference(inputs=[img])
print(outputs)
outputs = tf.nn.softmax(outputs)
print(outputs)

print(
    "This image most likely belongs to {} with a {:.2f} percent confidence."
    .format(class_names[np.argmax(outputs)], 100 * np.max(outputs))
)
#print("图像预测是:", class_names[np.argmax(outputs)])
print('--> done')

rknn.release()

# 详细请参考教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/ai/tensorflow.html