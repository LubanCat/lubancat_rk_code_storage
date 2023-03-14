"""
    将cifar10的test_batch转换成.jpg格式的图片,每个类别单独存放在一个文件夹，文件夹名称为0-9
"""
from imageio import imwrite
import numpy as np
import os
import pickle

# CIFAR-10数据集所在的绝对路径,根据自己实际目录设置
base_dir = "/mnt/e/Users/Administrator/Desktop/wsl_user/pytorch/"

data_dir = os.path.join(base_dir, "data", "cifar-10-batches-py")
test_o_dir = os.path.join( base_dir, "Data", "cifar-10-png", "raw_test")

# 解压
def unpickle(file):
    with open(file, 'rb') as fo:
        dict_ = pickle.load(fo, encoding='bytes')
    return dict_

# 生成测试集图片
if __name__ == '__main__':
    print("start...")
    test_data_path = os.path.join(data_dir, "test_batch")
    test_data = unpickle(test_data_path)
    for i in range(0, 10000):
        img = np.reshape(test_data[b'data'][i], (3, 32, 32))
        img = img.transpose(1, 2, 0)

        label_num = str(test_data[b'labels'][i])
        o_dir = os.path.join(test_o_dir, label_num)
        if not os.path.isdir(o_dir):
            os.makedirs(o_dir)

        img_name = label_num + '_' + str(i) + '.jpg'
        img_path = os.path.join(o_dir, img_name)
        imwrite(img_path, img)
    print("done.")
