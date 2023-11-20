#include "mainwindow.h"
#include "ui_mainwindow.h"

#include    <QHostAddress>
#include    <QHostInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化
    QActionGroup *tcpGroupAction = new QActionGroup(this);
    tcpGroupAction->setExclusive(true);

    tcpGroupAction->addAction(ui->actionClient);
    tcpGroupAction->addAction(ui->actionServer);
    connect(tcpGroupAction, &QActionGroup::triggered, this, &MainWindow::updateTcpGroup);

    ui->btn_connect->setEnabled(true);
    ui->btn_disconnect->setEnabled(false);
    this->setWindowTitle("TCP客户端");

    QString localIP=getLocalIP();      // 获取本机IP
    ui->comboServer->addItem(localIP);
    ui->comboClient->addItem(localIP);

    // 创建TCP socket
    tcpClient=new QTcpSocket(this);

    // 关联tcp客户端socket相关槽函数
    connect(tcpClient,SIGNAL(connected()),   this,SLOT(do_connected()));     // TCPClient连接
    connect(tcpClient,SIGNAL(disconnected()),this,SLOT(do_disconnected()));
    connect(tcpClient,SIGNAL(readyRead()),   this,SLOT(do_tcpClient_ReadyRead()));
    connect(tcpClient,&QTcpSocket::stateChanged,this,&MainWindow::do_socketStateChange);

    // 创建TCP server
    tcpServer=new QTcpServer(this);
    connect(tcpServer,SIGNAL(newConnection()),this,SLOT(do_newConnection()));  // TCP每新的连接可用时执行do_newConnection()槽函数。
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getLocalIP()
{
    QString hostName = QHostInfo::localHostName();       // 获取本地主机名
    QHostInfo hostInfo = QHostInfo::fromName(hostName);  // 从hostInfo对象中获取所有与该主机名相关的IP地址信息
    QString   localIP="";

    if (!hostInfo.addresses().isEmpty()) {
        foreach (const QHostAddress& address, hostInfo.addresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                qDebug() << "IPv4 Address:" << address.toString();
                localIP=address.toString();
                break;
            }
        }
        return localIP;
    } else {
        qDebug() << "Failed to retrieve the host address.";
        return localIP;
    }
}

void MainWindow::updateTcpGroup(QAction *action)
{
    Q_UNUSED(action)

    if(ui->actionClient->isChecked())
    {
        ui->comboServer->setEnabled(true);
        ui->spinPort->setEnabled(true);
        ui->btn_connect->setEnabled(true);
        ui->btn_disconnect->setEnabled(false);

        ui->comboClient->setEnabled(false);
        ui->spinBox->setEnabled(false);
        ui->btn_stop->setEnabled(false);
        ui->btn_start->setEnabled(false);

        this->setWindowTitle("TCP客户端");

        // 设置TCP客户端，关闭TCP服务端
        if (tcpServer->isListening())
        {
            if (tcpSocket != nullptr)
                if (tcpSocket->state()==QAbstractSocket::ConnectedState)
                    tcpSocket->disconnectFromHost();
            tcpServer->close();         //停止监听
        }
    }
    else if(ui->actionServer->isChecked())
    {
        ui->comboServer->setEnabled(false);
        ui->spinPort->setEnabled(false);
        ui->btn_connect->setEnabled(false);
        ui->btn_disconnect->setEnabled(false);

        ui->comboClient->setEnabled(true);
        ui->spinBox->setEnabled(true);
        ui->btn_stop->setEnabled(false);
        ui->btn_start->setEnabled(true);

        this->setWindowTitle("TCP服务端");

        // 设置用作TCP服务端，断开tcp客户端断开
        if (tcpClient->state()==QAbstractSocket::ConnectedState)
            tcpClient->disconnectFromHost();          // 客户端断开连接
    }
}

// connected()信号的槽函数
void MainWindow::do_connected()
{
    ui->plainTextEdit->appendPlainText("已连接到服务器");
    ui->plainTextEdit->appendPlainText("服务器地址:"+
                                   tcpClient->peerAddress().toString());
    ui->plainTextEdit->appendPlainText("端口:"+
                                   QString::number(tcpClient->peerPort()));
    ui->btn_connect->setEnabled(false);
    ui->btn_disconnect->setEnabled(true);
}

// disConnected()信号的槽函数
void MainWindow::do_disconnected()
{
    ui->plainTextEdit->appendPlainText("断开与服务器的连接");
    ui->btn_connect->setEnabled(true);
    ui->btn_disconnect->setEnabled(false);
}

// readyRead()信号的槽函数
void MainWindow::do_tcpClient_ReadyRead()
{
    // tcp客户端socket读取
    while(tcpClient->canReadLine())
        ui->plainTextEdit->appendPlainText("接受："+tcpClient->readLine());
}

// QTcpServer  newConnection的槽函数
void MainWindow::do_newConnection()
{
    ui->plainTextEdit->appendPlainText("一个客户端连接");
    tcpSocket = tcpServer->nextPendingConnection();   // 创建TCP服务端通信socket

    // 关联tcpSocket相关槽函数
    connect(tcpSocket, SIGNAL(connected()),this, SLOT(do_clientConnected()));
    connect(tcpSocket, SIGNAL(disconnected()),this, SLOT(do_clientDisconnected()));
    connect(tcpSocket,SIGNAL(readyRead()),  this,SLOT(do_tcpSocket_ReadyRead()));
    connect(tcpSocket,&QTcpSocket::stateChanged,this,&MainWindow::do_socketStateChange);
}

// 有客户端接入时
void MainWindow::do_clientConnected()
{
    ui->plainTextEdit->appendPlainText("连接上客户端socket");
    ui->plainTextEdit->appendPlainText("客户端地址:"+
                                 tcpSocket->peerAddress().toString());
    ui->plainTextEdit->appendPlainText("端口:"+
                                 QString::number(tcpSocket->peerPort()));
}

// 客户端断开连接时
void MainWindow::do_clientDisconnected()
{
    ui->plainTextEdit->appendPlainText("与客户端socket断开连接");
    tcpSocket->deleteLater();  // 删除socket
}

// 读取缓冲区
void MainWindow::do_tcpSocket_ReadyRead()
{
    // tcp 服务端socket读取
    while(tcpSocket->canReadLine())
        ui->plainTextEdit->appendPlainText("接收："+tcpSocket->readLine());
}

// socket状态变化
void MainWindow::do_socketStateChange(QAbstractSocket::SocketState socketState)
{
    switch(socketState)
    {
    case QAbstractSocket::UnconnectedState:
        ui->statusbar->showMessage("tcpsocket状态：UnconnectedState");
        break;
    case QAbstractSocket::HostLookupState:
        ui->statusbar->showMessage("tcpsocket状态：HostLookupState");
        break;
    case QAbstractSocket::ConnectingState:
        ui->statusbar->showMessage("tcpsocket状态：ConnectingState");
        break;
    case QAbstractSocket::ConnectedState:
        ui->statusbar->showMessage("tcpsocket状态：ConnectedState");
        break;
    case QAbstractSocket::BoundState:
        ui->statusbar->showMessage("tcpsocket状态：BoundState");
        break;
    case QAbstractSocket::ClosingState:
        ui->statusbar->showMessage("tcpsocket状态：ClosingState");
        break;
    case QAbstractSocket::ListeningState:
        ui->statusbar->showMessage("tcpsocket状态：ListeningState");
    }
}

// TCP客户端连接服务器
void MainWindow::on_btn_connect_clicked()
{
    QString ipaddr=ui->comboServer->currentText();
    quint16 port=ui->spinPort->value();
    tcpClient->connectToHost(ipaddr,port);      // 连接到服务器
}

// TCP客户端断开连接
void MainWindow::on_btn_disconnect_clicked()
{
    if (tcpClient->state()==QAbstractSocket::ConnectedState)
        tcpClient->disconnectFromHost();        // 断开服务器连接
}

void MainWindow::on_btn_stop_clicked()
{
    if (tcpServer->isListening())   //tcpServer正在监听
    {
        if (tcpSocket != nullptr)
            if (tcpSocket->state()==QAbstractSocket::ConnectedState)
                tcpSocket->disconnectFromHost();
        tcpServer->close();         //停止监听
        ui->btn_start->setEnabled(true);
        ui->btn_stop->setEnabled(false);
        ui->statusbar->showMessage("停止监听");
    }
}

// TCP服务端开始监听
void MainWindow::on_btn_start_clicked()
{
    QString  IP=ui->comboClient->currentText();  // IP地址
    quint16  port=ui->spinPort->value();         // 端口
    QHostAddress   address(IP);

    tcpServer->listen(address,port);             // 开始监听
    ui->plainTextEdit->appendPlainText("开始监听...");
    ui->plainTextEdit->appendPlainText("服务器地址："+tcpServer->serverAddress().toString()+ \
                                       " 端口："+QString::number(tcpServer->serverPort()));

    ui->btn_start->setEnabled(false);
    ui->btn_stop->setEnabled(true);
    ui->statusbar->showMessage("正在监听");
}

// 发送信息
void MainWindow::on_btn_send_clicked()
{
    QString txt=ui->lineEdit->text();
    if(!txt.isEmpty())
    {
        ui->plainTextEdit->appendPlainText("发送："+txt);
        ui->lineEdit->clear();
        ui->lineEdit->setFocus();

        QByteArray  str=txt.toUtf8();
        str.append('\n');
        if(ui->actionClient->isChecked())
            tcpClient->write(str);
        else if(ui->actionServer->isChecked())
            tcpSocket->write(str);
    }
}
