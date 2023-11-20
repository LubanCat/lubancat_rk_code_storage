#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>

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

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    myBrush(&painter);         // 线条和笔刷填充
    myDrawPoints(&painter);    // 画点
    myDrawLines(&painter);     // 画线
    myDrawRect(&painter);      // 画矩形
    myDrawShape(&painter);     // 其他图形
    myDrawGradient(&painter);  // 渐变
    myDrawText(&painter);      // 文字
    myDrawPath(&painter);

//    QPen pen;
//    QBrush brush(QColor(0, 255, 0, 125));

//    //颜色
//    for(int i=0; i<3; i++)
//        for(int j=0; j<3; j++)
//        {
//            painter.setBrush(QColor(255-j*50, i*50, i*j*10, 100));
//            painter.drawRect(205+i*30, 110+j*30, 30, 30);
//        }

//    //简单绘制图片个
//    QPixmap pix;
//    pix.load(":/image/test.png");
//    painter.drawPixmap(650, 305, 25, 25, pix);

//    //缩放
//    pix = pix.scaled(pix.width()*2, pix.height()*2,Qt::KeepAspectRatio);
    //    painter.drawPixmap(650, 330, pix);
}

void MainWindow::myBrush(QPainter *qp)
{
    QPen pen;
    QBrush brush(QColor(0, 255, 0, 125));

    //笔刷
    pen.setStyle(Qt::DotLine);     //设置线条为点划线
    brush.setColor(Qt::blue);      //设置颜色蓝色
    brush.setStyle(Qt::NoBrush);   //设置没有图案
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(5,10,90,90);      //在指定位置绘制

    pen.setStyle(Qt::SolidLine);           //重新设置线条为宽线
    brush.setColor(Qt::red);               //重新设置填充颜色红色
    brush.setStyle(Qt::SolidPattern);      //设置填充颜色均匀,单一颜色填充
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(105,10,90,90);      //在指定位置绘制

    pen.setStyle(Qt::DashLine);
    brush.setColor(Qt::yellow);
    brush.setStyle(Qt::Dense3Pattern);
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(205,10,90,90);

    pen.setStyle(Qt::DotLine);
    brush.setColor(Qt::blue);
    brush.setStyle(Qt::HorPattern);
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(305,10,90,90);

    pen.setStyle(Qt::DashDotLine);
    brush.setColor(Qt::blue);
    brush.setStyle(Qt::CrossPattern);
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(405,10,90,90);

    pen.setStyle(Qt::DashDotDotLine);
    brush.setColor(Qt::blue);
    brush.setStyle(Qt::BDiagPattern);
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(505,10,90,90);

    pen.setStyle(Qt::SolidLine);
    brush.setColor(Qt::blue);
    brush.setStyle(Qt::DiagCrossPattern);
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(605,10,90,90);

    pen.setStyle(Qt::NoPen);
    brush.setColor(Qt::blue);
    brush.setStyle(Qt::NoBrush);
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(705,10,90,90);
}

void MainWindow::myDrawPoints(QPainter *qp)
{
    qp->setPen(QColor(255,0,0));
    qp->setBrush(QColor(255,0,0));
    QPolygonF points;

    for(int i=0; i<10; i++)
        for(int j=0; j<10; j++)
        {
            qp->drawPoint(QPoint(305+i*10,110+j*10));  //绘制单个点,在指定位置
            points.append(QPoint(405+i*10,110+j*10));
        }

    qp->setPen(QColor(0,0,255));
    qp->setBrush(QColor(0,0,255));
    qp->drawPoints(points);          //批量绘制点
}

void MainWindow::myDrawLines(QPainter *qp)
{
    QPen pen;

    //画直线
    pen.setWidth(5);
    pen.setStyle(Qt::SolidLine);    //宽线
    pen.setCapStyle(Qt::SquareCap); //端点样式是方形线条端
    qp->setPen(pen);
    qp->drawLine(QPointF(5, 110), QPointF(195, 110));

    pen.setStyle(Qt::DashLine);     //虚线
//    pen.setColor(QColor(255, 0, 0));
    qp->setPen(pen);
    qp->drawLine(QPointF(5, 130), QPointF(195, 130));

    pen.setStyle(Qt::DotLine);
    pen.setCapStyle(Qt::FlatCap);
    qp->setPen(pen);
    qp->drawLine(QPointF(5, 150), QPointF(195, 150));

    pen.setStyle(Qt::DashDotLine);
//    pen.setColor(QColor(255, 255, 0));
    qp->setPen(pen);
    qp->drawLine(QPointF(5, 170), QPointF(195, 170));

    pen.setStyle(Qt::DashDotDotLine);
    pen.setCapStyle(Qt::RoundCap);
    qp->setPen(pen);
    qp->drawLine(QPointF(5, 190), QPointF(195, 190));

    pen.setStyle(Qt::CustomDashLine);
    pen.setWidth(2);
    qp->setPen(pen);
    qp->drawLine(QPointF(5, 210), QPointF(195, 210));

}

void MainWindow::myDrawRect(QPainter *qp)
{
    QPen pen;
    QBrush brush;

    //画矩形，有填充矩形
    pen.setColor(QColor(255, 0, 0));
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(QColor(0, 255, 0, 125));
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(505, 110, 90, 90);

    //画矩形，无填充矩形
    pen.setStyle(Qt::SolidLine);
    pen.setColor(QColor(255, 0, 0));
    brush.setStyle(Qt::NoBrush);
    qp->setPen(pen);
    qp->setBrush(brush);
    qp->drawRect(605, 110, 90, 90);

    //画圆角矩形
    QRectF rectangle(705.0, 110.0, 90.0, 90.0);
    qp->drawRoundedRect(rectangle, 20.0, 20.0);

}

void MainWindow::myDrawShape(QPainter *qp)
{
    QPen pen;
    //画弧线
    pen.setStyle(Qt::SolidLine);
    qp->setPen(pen);

    int startAngle = 30 * 16;//起始角度
    int spanAngle = 120 * 16;//跨越度数
    qp->drawArc(QRectF(5.0, 210.0, 90.0, 90.0), startAngle, spanAngle);

    //扇形
    qp->drawPie(QRectF(105.0, 210.0, 90.0, 90.0), startAngle, spanAngle);

    //带弦的弧
    qp->drawChord(QRect(205, 210, 90, 90), startAngle, spanAngle);

    //椭圆
    qp->drawEllipse(QRectF(305,220,90,60));
    //圆
    qp->drawEllipse(QRectF(405,210,90,90));

    //折线
    QPointF Polylinepoints[5] = {
        QPointF(505.0, 210.0),
        QPointF(555.0, 290.0),
        QPointF(535.0, 275.0),
        QPointF(595.0, 295.0),
        QPointF(505.0, 215.0),
    };
    qp->drawPolyline(Polylinepoints, 5);

    //多边形
    QPointF Polygonpoints[4] = {
        QPointF(610.0, 280.0),
        QPointF(620.0, 210.0),
        QPointF(680.0, 230.0),
        QPointF(690.0, 270.0),
    };
    qp->drawConvexPolygon(Polygonpoints, 4);
}

void MainWindow::myDrawGradient(QPainter *qp)
{
    //线性渐变
    QLinearGradient linearGradient(QPointF(40, 190), QPointF(70, 190));
    linearGradient.setColorAt(0, Qt::yellow);
    linearGradient.setColorAt(0.5, Qt::red);
    linearGradient.setColorAt(1, Qt::green);
    linearGradient.setSpread(QGradient::RepeatSpread);
    qp->setBrush(linearGradient);
    qp->drawRect(5, 310, 90, 90);

    //辐射渐变
    QRadialGradient radialGradient(QPointF(150, 350),40,QPointF(550,40));
    radialGradient.setColorAt(0, QColor(255, 255, 100, 150));
    radialGradient.setColorAt(1, QColor(0, 0, 0, 50));
    qp->setBrush(radialGradient);
    qp->drawEllipse(QPointF(150, 350), 40, 40);

    //锥形渐变
    QConicalGradient conicalGradient(QPointF(250, 350), 100);
    conicalGradient.setColorAt(0.2, Qt::cyan);
    conicalGradient.setColorAt(0.9, Qt::black);
    qp->setBrush(conicalGradient);
    qp->drawEllipse(QPointF(250, 350), 40, 40);
}

void MainWindow::myDrawText(QPainter *qp)
{
    //设置一个矩形
    QRectF rect(305, 310, 190, 90);
    qp->drawRect(rect);
    qp->setPen(QColor(Qt::red));
    qp->drawText(rect, Qt::AlignHCenter, "野火电子");

    //绘制文字
    QFont font("宋体", 10, QFont::Bold, true);
    font.setLetterSpacing(QFont::AbsoluteSpacing,5);
    qp->setFont(font);
    qp->drawText(310, 350, "Qt 嵌入式教程");

    //绘制文字
    qp->setPen(Qt::green);
    font.setLetterSpacing(QFont::AbsoluteSpacing,0);
    font.setBold(false);
    font.setFamily("黑体");
    font.setPointSize(10);
    font.setItalic(true);
    qp->setFont(font);
    qp->drawText(310, 380, "绘制文字");
}

void MainWindow::myDrawPath(QPainter *qp)
{

    //简单的使用QPainterPath,使绘制图形能够被构建和重用
    QPainterPath path1;
    path1.addEllipse(510, 320, 50, 50);  //绘制一个圆
    path1.lineTo(505, 380);              //绘制一直线
    qp->setPen(Qt::blue);
    qp->setBrush(Qt::red);               //填充红色
    qp->drawPath(path1);

    //复制图形
    QPainterPath path2;
    path2.addPath(path1);
    path2.translate(70,0);
    qp->drawPath(path2);

    //绘制线
    QPainterPath path4;
    qp->setBrush(Qt::transparent);
    path4.lineTo(0,105);
    path4.lineTo(this->width(),105);
    path4.lineTo(this->width(),205);
    path4.lineTo(0,205);
    path4.lineTo(0,305);
    path4.lineTo(this->width(),305);
    path4.lineTo(this->width(),405);
    path4.lineTo(0,405);
    qp->drawPath(path4);
}



