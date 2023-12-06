#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUI();                     //UI初始化
    this->setCentralWidget(ui->plain_textedit);   //plain_textedit填充满工作区
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initUI()
{
    //窗口菜单栏添加手动一个
    QAction *aboutQtAct = new QAction(this);
    aboutQtAct->setObjectName("action_about");
    aboutQtAct->setText("About &Qt");
    aboutQtAct->setIcon(QIcon(":/about.png"));
    connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt); //关联action_about操作的槽函数
    ui->menu->addAction(aboutQtAct);

    //窗口工具栏上添加工具，字体和字号，以及退出
    ui->toolBar->addSeparator();      //工具栏上增加分隔条

    fontSize = new QSpinBox(this);    //QSpinBox来设置plain_textedit字号
    fontSize->setMinimum(10);
    fontSize->setMaximum(30);
    fontSize->setValue(ui->plain_textedit->font().pointSize()); //设置plain_textedit字号
    fontSize->setMinimumWidth(30);
    ui->toolBar->addWidget(fontSize);      //添加字号选择到工具栏

    fontName = new QFontComboBox(this);    //字体名称ComboBox
    fontName->setMinimumWidth(50);
    ui->toolBar->addWidget(fontName);      //添加字体选择到工具栏

    //窗口状态栏添加信息
    labFile = new QLabel(this);            //用于显示当前
    labFile->setMinimumWidth(40);
    labFile->setText("文本状态");
    ui->statusbar->addWidget(labFile);     //添加到状态栏

    labRow = new QLabel(this);             //状态栏里显示行
    labRow->setMinimumWidth(10);
    labRow->setText("行: ");
    ui->statusbar->addWidget(labRow); //添加到状态栏

    labColumn = new QLabel(this);     //状态栏里显示列
    labColumn->setMinimumWidth(10);
    labColumn->setText("列: ");
    ui->statusbar->addWidget(labColumn); //添加到状态栏
//    labRow = new QLabel(this);        //状态栏里显示行
//    labRow->setText("行: ");
//    ui->statusbar->addPermanentWidget(labRow); //添加到状态栏

//    labColumn = new QLabel(this);    //状态栏里显示列
//    labColumn->setText("列: ");
//    ui->statusbar->addPermanentWidget(labColumn); //添加到状态栏

    labInfo=new QLabel(this);  //用于显示字体名称的标签
    labInfo->setText("字体名称："+fontName->currentFont().family());
    ui->statusbar->addPermanentWidget(labInfo); //添加到状态栏

    //关联fontSize值的改变和设置字号
    connect(fontSize,SIGNAL(valueChanged(int)), this, SLOT(do_fontSize_changed(int)));
    //关联fontName选择字体的改变和设置文本框字体
    connect(fontName,SIGNAL(currentFontChanged(QFont)), this, SLOT(do_fontSelected(QFont)));
}


//字号设置
void MainWindow::do_fontSize_changed(int fontSize)
{
    QTextCharFormat fmt=ui->plain_textedit->currentCharFormat();
    fmt.setFontPointSize(fontSize); //设置字体大小
    ui->plain_textedit->mergeCurrentCharFormat(fmt);
//    progressBar1->setValue(fontSize);
}


//字体选择
void MainWindow::do_fontSelected(const QFont &font)
{
    labInfo->setText("字体名称："+font.family());  //状态栏上显示
    QTextCharFormat fmt=ui->plain_textedit->currentCharFormat();
    fmt.setFont(font);
    ui->plain_textedit->mergeCurrentCharFormat(fmt);
}


//行与列
void MainWindow::on_plain_textedit_cursorPositionChanged()
{
    int row,col;
    QTextCursor cursor;

    cursor = ui->plain_textedit->textCursor();
    col = cursor.columnNumber();
    row = cursor.blockNumber();

    labRow->setText("行: "+QString::number(row));
    labColumn->setText("列: "+QString::number(col));
}

//创建
void MainWindow::on_action_new_triggered()
{
    ui->plain_textedit->clear();
    ui->plain_textedit->document()->setModified(false);    //表示已经保存了,修改状态
    labFile->setText("新建了一个文件");                     //状态栏输出信息
}

//保存编辑框内容
void MainWindow::on_action_save_triggered()
{
    ui->plain_textedit->document()->setModified(false);       //表示内容已经保存了,改变状态
    labFile->setText("文件已保存");                       //状态栏输出信息
}

//打开
void MainWindow::on_action_open_triggered()
{
    labFile->setText("打开的文件");                       //状态栏输出信息
}


void MainWindow::on_plain_textedit_copyAvailable(bool b)
{
    ui->action_cut->setEnabled(b);
    ui->action_clear->setEnabled(b);
    ui->action_paste->setEnabled(ui->plain_textedit->canPaste());
}

