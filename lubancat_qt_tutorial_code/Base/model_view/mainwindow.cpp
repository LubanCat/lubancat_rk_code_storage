#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init_StringListModel();

    init_StandardItemModel();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init_StringListModel()
{
    // 设置数据
    list<<"第一"<<"第二 "<<"第三"<<"第四"<<"第五";

    // 设置模型数据
    listModel.setStringList(list);

    // 视图,设置使用的模型
    ui->listView->setModel(&listModel);

    connect(ui->btn_add,&QPushButton::clicked,this,&MainWindow::StringListModel_add);
    connect(ui->btn_remove,&QPushButton::clicked,this,&MainWindow::StringListModel_remove);
    connect(ui->btn_initList,&QPushButton::clicked,this,&MainWindow::StringListModel_initList);
    connect(ui->btn_instert,&QPushButton::clicked,this,&MainWindow::StringListModel_instert);
}

void MainWindow::init_StandardItemModel()
{
    // 创建一个模型
    m_model = new QStandardItemModel(3,2,this);

    // 初始化模型
    for(int r=0;r<m_model->rowCount();r++)
    {
        for(int c=0;c<m_model->columnCount();c++)
        {
            QStandardItem* item=new QStandardItem(QString("行 %0, 列 %1").arg(r).arg(c));
            m_model->setItem(r,c,item);
        }
    }
    // tableView设置数据模型
    ui->tableView->setModel(m_model);

    //在最后列,插入一列
    m_model->insertColumn(m_model->columnCount());
    // 初始化
    for(int r=0;r<m_model->rowCount();r++)
    {
        QStandardItem* item=new QStandardItem(QString("第一"));
        m_model->setItem(r,m_model->columnCount()-1,item);
    }

    combo_delegate = new ComboboxDelegate(this);
    QStringList strList;
    strList<<"第一"<<"第二"<<"第三"<<"第四";
    combo_delegate->setItems(strList);
    // 设置最后一列为自定义代理，当编辑时显示Combobox选择
    ui->tableView->setItemDelegateForColumn(m_model->columnCount()-1, combo_delegate);
}

void MainWindow::StringListModel_add()
{
    QString addItem=QInputDialog::getText(this,"添加项","输入项的数据:");
    if(addItem.isEmpty()) return;

    // 最后一行添加一个item
    if(listModel.insertRow(listModel.rowCount()))
    {
        // 获取添加item的索引
        QModelIndex index=listModel.index(listModel.rowCount()-1,0);
        // 设置内容
        listModel.setData(index,addItem,Qt::DisplayRole);
    }
}

void MainWindow::StringListModel_remove()
{
    // 获取当前的索引
    QModelIndex index=ui->listView->currentIndex();

    // 使用QAbstractItemModel::removeRow移除一个项目(item);调用index.row()返回item的行号;
    listModel.removeRow(index.row());
}

void MainWindow::StringListModel_initList()
{
    // 重新添加数据
    listModel.setStringList(list);
}

void MainWindow::StringListModel_instert()
{
    // 获取当前项的模型索引
    QModelIndex index=ui->listView->currentIndex();

    listModel.insertRow(index.row());
    listModel.setData(index,"插入的项",Qt::DisplayRole);

//    ui->listView->setCurrentIndex(index);   //设置当前项
}

