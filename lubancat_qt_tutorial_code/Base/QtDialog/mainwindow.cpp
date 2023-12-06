#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QTextStream>
#include <QList>
#include <QDateTime>
#include <QFontDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QErrorMessage>
#include <QInputDialog>
#include <QLineEdit>
#include <QProgressDialog>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/qprintpreviewwidget.h>
#include <QDebug>
#include "custcommessage.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btn_openFile_clicked()
{
    QString curPath=QDir::currentPath();//获取应用程序的路径
    QString dlgTitle="选择文件"; //对话框标题
    QString filter="文本文件(*.txt);;图片文件(*.jpg *.gif *.png);;所有文件(*.*)"; //文件过滤器
    QString fileName=QFileDialog::getOpenFileName(this,dlgTitle,curPath,filter);
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        ui->statusBar->showMessage("打开文件失败");
        return;
    }
    ui->statusBar->showMessage("打开文件:"+fileName);

    QTextStream in(&file);
    ui->textEdit->clear();
    ui->textEdit->setPlainText(in.readAll());
    file.close();
}

void MainWindow::on_btn_openFiles_clicked()
{
    QString curPath=QDir::currentPath();//获取应用程序的路径
    QString dlgTitle="选择多个文件"; //对话框标题
    QString filter="文本文件(*.txt);;图片文件(*.jpg *.gif *.png);;所有文件(*.*)"; //文件过滤器
    QStringList filelist = QFileDialog::getOpenFileNames(this,dlgTitle,curPath,filter);

    ui->textEdit->clear();
    foreach(QString str,filelist)
        ui->textEdit->append(str);
}

void MainWindow::on_btn_openDir_clicked()
{
    QString curPath=QDir::currentPath();//获取应用程序的路径
    QString dlgTitle="选择文件夹"; //对话框标题
    QString dirname = QFileDialog::getExistingDirectory(this,dlgTitle,curPath);
    if (dirname.isEmpty())
        return;

    QDir dir(dirname);
    QList<QFileInfo> files=dir.entryInfoList();

    ui->statusBar->showMessage("打开文件夹:"+dirname);

    ui->textEdit->clear();
    for(int i = 0;i<files.count(); i++)
    {
        ui->textEdit->append(files.at(i).filePath());
        ui->textEdit->append(files.at(i).fileName());
    }
}

void MainWindow::on_btn_save_clicked()
{
    if(ui->textEdit->toPlainText().isEmpty())
    {
        ui->statusBar->showMessage("内容不能为空");
        return;
    }

    QString curPath=QDir::currentPath();//获取应用程序的路径
    QString filePath=curPath+"/"+QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss")+".txt";
    qDebug()<<filePath;
    QString dlgTitle="保存文件"; //对话框标题
    QString filter="文本文件(*.txt);;图片文件(*.jpg *.gif *.png);;所有文件(*.*)"; //文件过滤器
    QString fileName = QFileDialog::getSaveFileName(this, dlgTitle, filePath,filter);
    if (fileName.isEmpty())
        return ;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        ui->statusBar->showMessage("保存文件失败");
    } else {
        QTextStream stream(&file);
        stream << ui->textEdit->toPlainText();
        stream.flush();
        file.close();
        ui->statusBar->showMessage("保存文件:"+fileName);
    }
}

void MainWindow::on_btn_font_clicked()
{
    bool choose;
    QFont initfont=qApp->font();
    QString dlgTitle="选择字体"; //对话框标题
    QFont font=QFontDialog::getFont(&choose,initfont,this,dlgTitle); //选择字体

    if(choose)
    {
        ui->textEdit->setFont(font);
        ui->statusBar->showMessage("设置字体:"+font.toString());
    }
}

void MainWindow::on_btn_color_clicked()
{
    QColor color=QColorDialog::getColor(Qt::white,this,"选择颜色");
    if(color.isValid())
    {
        qDebug()<<color;
        ui->textEdit->setTextColor(color);
    }
}

void MainWindow::on_btn_critical_clicked()
{
    QString dlgTitle="critical";
    QString str="严重错误";
    QMessageBox::critical(this, dlgTitle, str,QMessageBox::Ok);
}

void MainWindow::on_btn_information_clicked()
{
    QMessageBox meg;
    QString dlgTitle="information";
    QString str="提示消息";
    QMessageBox msg(QMessageBox::Information,dlgTitle, str,QMessageBox::Yes, NULL);
    msg.exec();
}

void MainWindow::on_btn_question_clicked()
{
    QString dlgTitle="question";
    QString str="询问消息";
    int choose=QMessageBox::question(this, dlgTitle, str,QMessageBox::Yes,QMessageBox::No,QMessageBox::Cancel);

    switch (choose) {
    case QMessageBox::Yes:
        ui->statusBar->showMessage("你选择了OK");
        break;
    case QMessageBox::No:
        ui->statusBar->showMessage("你选择了NO");
        break;
    case QMessageBox::Cancel:
        ui->statusBar->showMessage("你选择了Cancel");
        break;
    default:
        ui->statusBar->showMessage("你关闭了对话框");
        break;
    }
}

void MainWindow::on_btn_warning_clicked()
{
    QString dlgTitle="warning";
    QString str="警告消息";
    QMessageBox::warning(this, dlgTitle, str,QMessageBox::Ok);
}

void MainWindow::on_btn_about_clicked()
{
    QString dlgTitle="about";
    QString str="关于";
    QMessageBox::about(this, dlgTitle, str);
}

void MainWindow::on_btn_aboutQt_clicked()
{
    QString dlgTitle="about Qt";
    QMessageBox::aboutQt(this, dlgTitle);
}

void MainWindow::on_btn_error_clicked()
{
    QErrorMessage *errDlg = new QErrorMessage(this);
    errDlg->setWindowTitle("QErrorMessage");
    errDlg->showMessage("错误信息");
}

void MainWindow::on_btn_getDouble_clicked()
{
    QString dlgTitle="getDouble";
    QString str="浮点数输入框";

    int min=0, max=100,currValue=20.0,decimal=2;

    bool choose=false;
    double input = QInputDialog::getDouble(this, dlgTitle,str,currValue,min,max,decimal, &choose);
    if (choose)
        ui->statusBar->showMessage(QString::number(input));
}

void MainWindow::on_btn_getInt_clicked()
{
    QString dlgTitle="getInt";
    QString str="整数输入框";

    int min=0, max=100,step=1,currValue=20;

    bool choose=false;
    int input = QInputDialog::getInt(this, dlgTitle,str,currValue,min,max,step, &choose);
    if (choose)
        ui->statusBar->showMessage(QString::number(input));
}

void MainWindow::on_btn_getItem_clicked()
{
    QString dlgTitle="getItem";
    QString str="Item输入框";

    QStringList items;
    items <<"0"<<"1"<<"2"<<"3"<<"4"<<"5";
    int curIndex=0;//初始Item
    bool editable=true; //ComboBox是否可编辑

    bool choose=false;
    QString input = QInputDialog::getItem(this, dlgTitle,str,items,curIndex,editable, &choose);
    if (choose)
        ui->statusBar->showMessage(input);
}

void MainWindow::on_btn_getMultiLineText_clicked()
{
    QString dlgTitle="getItem";
    QString str="Item输入框";

    QStringList items;
    items <<"0"<<"1"<<"2"<<"3"<<"4"<<"5";

    QString defaultstr="输入文本";
    const QString defaulttext="输入文本";

    bool choose=false;
    QString input = QInputDialog::getMultiLineText(this, dlgTitle,str,defaultstr, &choose);
    if (choose)
        ui->statusBar->showMessage(input);
}

void MainWindow::on_btn_getText_clicked()
{
    QString dlgTitle="getText";
    QString str="文本输入框";

    QString defaultstr="输入文本";
    QLineEdit::EchoMode echoMode=QLineEdit::Normal;

    bool choose=false;
    QString input = QInputDialog::getText(this, dlgTitle,str, echoMode,defaultstr, &choose);
    if (choose)
    {
        if(input.isEmpty())
            return;

        ui->statusBar->showMessage(input);
    }
}

void MainWindow::on_btn_progress_clicked()
{
    QProgressDialog *progressDlg=new QProgressDialog(this);

    progressDlg->setWindowModality(Qt::WindowModal);
    progressDlg->setMinimumDuration(5);
    progressDlg->setWindowTitle(tr("QProgressDialog"));
    progressDlg->setLabelText(tr("进度条"));
    progressDlg->setCancelButtonText(tr("取消"));
    progressDlg->setRange(0,100);

    progressDlg->setValue(59);
}

void MainWindow::on_btn_print_clicked()
{
    QPrintDialog *print = new QPrintDialog(this);

    print->exec();
}


void MainWindow::on_btn_logo_clicked()
{
    CustomMessage *msg=new CustomMessage(this);
    msg->setMessage(QString("自定义问对话框"));
    if(msg->exec()==QDialog::Accepted)
    {
        qDebug()<<"你点击了确定按钮";
    }
    else
    {
        qDebug()<<"你点击了取消按钮";
    }
    delete msg;
}
