# 详细请参考教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/ai/paddlepaddle.html
import paddle
import numpy as np
from paddle.vision.transforms import Normalize

transform = Normalize(mean=[127.5], std=[127.5], data_format='CHW')
# 下载数据集并初始化 DataSet
train_dataset = paddle.vision.datasets.MNIST(mode='train', transform=transform)
test_dataset = paddle.vision.datasets.MNIST(mode='test', transform=transform)

# 模型组网并初始化网络
lenet = paddle.vision.models.LeNet(num_classes=10)
model = paddle.Model(lenet)
paddle.summary(lenet,(1, 1, 28, 28))

# 模型训练的配置准备，准备损失函数，优化器和评价指标
model.prepare(paddle.optimizer.Adam(learning_rate=0.001, parameters=model.parameters()), 
             paddle.nn.CrossEntropyLoss(),
             paddle.metric.Accuracy())

# 模型训练
model.fit(train_dataset, epochs=5, batch_size=64, verbose=1)
# 模型评估
model.evaluate(test_dataset, batch_size=64, verbose=1)

# 保存模型
model.save('./output/mnist')
# 加载模型
#model.load('output/mnist')

# 从测试集中取出一张图片
img, label = test_dataset[0]
# 将图片shape从1*28*28变为1*1*28*28，增加一个batch维度，以匹配模型输入格式要求
img_batch = np.expand_dims(img.astype('float32'), axis=0)

# 执行推理并打印结果，此处predict_batch返回的是一个list，取出其中数据获得预测结果
out = model.predict_batch(img_batch)[0]
pred_label = out.argmax()
print('true label: {}, pred label: {}'.format(label[0], pred_label))

# export to ONNX
save_path = 'onnx/lenet' # 需要保存的路径
x_spec = paddle.static.InputSpec([1, 1, 28, 28], 'float32', 'x') 
# 为模型指定输入的形状和数据类型，支持持 Tensor 或 InputSpec ，InputSpec 支持动态的 shape。
paddle.onnx.export(lenet, save_path, input_spec=[x_spec], opset_version=12)


