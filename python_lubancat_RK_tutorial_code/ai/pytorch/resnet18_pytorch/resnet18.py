# 参考教程https://doc.embedfire.com/linux/rk356x/Python/zh/latest/ai/resnet18_pytorch.html
import os
import torchvision
import torch
import torch.nn as nn

#device=torch.device("cuda" if torch.cuda.is_available() else "cpu")
device=torch.device("cpu")

# Transform configuration and data augmentation
transform_train=torchvision.transforms.Compose([
        torchvision.transforms.Pad(4),
        torchvision.transforms.RandomHorizontalFlip(), #图像一半的概率翻转，一半的概率不翻转
        torchvision.transforms.RandomCrop(32), #图像随机裁剪成32*32
        # torchvision.transforms.RandomVerticalFlip(),
        # torchvision.transforms.RandomRotation(15),
        torchvision.transforms.ToTensor(), #转为Tensor ，归一化
        torchvision.transforms.Normalize([0.5,0.5,0.5], [0.5,0.5,0.5])
        #torchvision.transforms.Normalize((0.4914, 0.4822, 0.4465),(0.2023, 0.1994, 0.2010))
        ])
transform_test=torchvision.transforms.Compose([
        torchvision.transforms.ToTensor(),
        torchvision.transforms.Normalize([0.5,0.5,0.5], [0.5,0.5,0.5])
        #torchvision.transforms.Normalize((0.4914, 0.4822, 0.4465), (0.2023, 0.1994, 0.2010))
        ])
# epoch时才对数据集进行以上数据增强操作

num_classes=10
batch_size=128
learning_rate=0.001
num_epoches=100
classes = ("plane","car","bird","cat","deer","dog","frog","horse","ship","truck")

# load downloaded dataset
train_dataset = torchvision.datasets.CIFAR10('./data', download=True, train=True, transform=transform_train)
test_dataset = torchvision.datasets.CIFAR10('./data', download=True, train=False, transform=transform_test)

# Data loader 
train_loader = torch.utils.data.DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
test_loader = torch.utils.data.DataLoader(test_dataset, batch_size=batch_size, shuffle=False)

# Define 3*3 convolutional neural network
def conv3x3(in_channels, out_channels, stride=1):
    return nn.Conv2d(in_channels, out_channels, kernel_size=3, stride=stride, padding=1, bias=False)

class ResidualBlock(nn.Module):
    def __init__(self, in_channels, out_channels, stride=1, downsample=None):
        super(ResidualBlock, self).__init__()
        self.conv1 = conv3x3(in_channels, out_channels, stride)
        self.bn1 = nn.BatchNorm2d(out_channels)
        self.relu = nn.ReLU(inplace=True)
        self.conv2 = conv3x3(out_channels, out_channels)
        self.bn2 = nn.BatchNorm2d(out_channels)
        self.downsample = downsample
    def forward(self, x):
        residual=x
        out = self.conv1(x)
        out = self.bn1(out)
        out = self.relu(out)
        out = self.conv2(out)
        out = self.bn2(out)
        if(self.downsample):
            residual = self.downsample(x)
        out += residual
        out = self.relu(out)
        return out
 
# 自定义一个神经网络，使用nn.model，，通过__init__初始化每一层神经网络。
# 使用forward连接数据
class ResNet(nn.Module):
    def __init__(self, block, layers, num_classes):
        super(ResNet, self).__init__()
        self.in_channels = 16
        self.conv = conv3x3(3, 16)
        self.bn = torch.nn.BatchNorm2d(16)
        self.relu = torch.nn.ReLU(inplace=True)
        self.layer1 = self._make_layers(block, 16, layers[0])
        self.layer2 = self._make_layers(block, 32, layers[1], 2)
        self.layer3 = self._make_layers(block, 64, layers[2], 2)
        self.layer4 = self._make_layers(block, 128, layers[3], 2)
        self.avg_pool = torch.nn.AdaptiveAvgPool2d((1, 1))
        self.fc = torch.nn.Linear(128, num_classes)
        
    def _make_layers(self, block, out_channels, blocks, stride=1):
        downsample = None
        if (stride != 1) or (self.in_channels != out_channels):
            downsample = torch.nn.Sequential(
                conv3x3(self.in_channels, out_channels, stride=stride),
                torch.nn.BatchNorm2d(out_channels))
        layers = []
        layers.append(block(self.in_channels, out_channels, stride, downsample))
        self.in_channels = out_channels
        for i in range(1, blocks):
            layers.append(block(out_channels, out_channels))
        return torch.nn.Sequential(*layers)
    
    def forward(self, x):
        out = self.conv(x)
        out = self.bn(out)
        out = self.relu(out)
        out = self.layer1(out)
        out = self.layer2(out)
        out = self.layer3(out)
        out = self.layer4(out)
        out = self.avg_pool(out)
        out = out.view(out.size(0), -1)
        out = self.fc(out)
        return out

# Make model，使用cpu
model=ResNet(ResidualBlock, [2,2,2,2], num_classes).to(device=device)

# 打印model结构
# print(f"Model structure: {model}\n\n")

# 优化器和损失函数
criterion = nn.CrossEntropyLoss()  #交叉熵损失函数
optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate) #优化器随机梯度下降

if __name__ == "__main__":
    # Train the model
    total_step = len(train_loader)
    for epoch in range(0,num_epoches):
        for i, (images, labels) in enumerate(train_loader):
            images = images.to(device=device)
            labels = labels.to(device=device)
            # Forward pass
            outputs = model(images)
            loss = criterion(outputs, labels)
            # Backward and optimize
            optimizer.zero_grad()
            # 反向传播
            loss.backward()
            # 更新参数
            optimizer.step()
            #sum_loss += loss.item()
            #_, predicted = torch.max(outputs.data, dim=1)
            #total += labels.size(0)
            #correct += predicted.eq(labels.data).cpu().sum()
            if (i+1) % total_step == 0:
                print('Epoch [{}/{}], Step [{}/{}], Loss: {:.4f}'
                      .format(epoch+1, num_epoches, i+1, total_step, loss.item()))
    print("Finished Tranining")
    # 保存权重文件
    #torch.save(model.state_dict(), 'model_weights.pth')
    #torch.save(model, 'model.pt')

    print('\nTest the model')
    model.eval()
    with torch.no_grad():
        correct = 0
        total = 0
        for images, labels in test_loader:
            images = images.to(device=device)
            labels = labels.to(device=device)
            outputs = model(images)
            _, predicted = torch.max(outputs.data, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()
        print('在10000张测试集图片上的准确率:{:.4f} %'.format(100 * correct / total))

    # 导出onnx模型
    x = torch.randn((1, 3, 32, 32))
    torch.onnx.export(model, x, './resnet18.onnx', opset_version=12, input_names=['input'], output_names=['output'])


