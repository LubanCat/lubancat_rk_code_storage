#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setCentralWidget(ui->openGLWidget);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
//    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    ui->openGLWidget->setFormat(format);

}

MainWindow::~MainWindow()
{
    delete ui;
}

