#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QActionGroup>
#include <QTcpSocket>
#include <QTcpServer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QActionGroup *tcpGroupAction;
    void updateTcpGroup(QAction *action);
    QString getLocalIP();

    QTcpSocket  *tcpClient;             // TCP客户端socket

    QTcpServer *tcpServer;              // TCP服务器
    QTcpSocket *tcpSocket=nullptr;      // TCP通讯的Socket


private slots:
    void on_btn_connect_clicked();
    void on_btn_disconnect_clicked();
    void on_btn_send_clicked();
    void on_btn_stop_clicked();
    void on_btn_start_clicked();

    // TCPclient相关自定义槽函数
    void do_connected();
    void do_disconnected();
    void do_tcpClient_ReadyRead();

    // TCPserver的自定义槽函数
    void do_newConnection();
    void do_clientConnected();
    void do_clientDisconnected();
    void do_tcpSocket_ReadyRead();

    // Socket状态显示槽函数
    void do_socketStateChange(QAbstractSocket::SocketState socketState);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
