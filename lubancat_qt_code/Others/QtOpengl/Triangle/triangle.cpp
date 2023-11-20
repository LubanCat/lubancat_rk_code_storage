#include "triangle.h"
#include <QScreen>

Triangle::Triangle(QWidget *parent) : QOpenGLWidget(parent)
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Triangle::animation);
    timer->start(20);
}

Triangle::~Triangle(){
//    delete m_program;
}

static const char *vertexShaderSource =
    "#version 320 es\n"
    "layout (location = 0) in vec4 posAttr;\n"
    "layout (location = 1) in vec3 colAttr;\n"
    "uniform mat4  matrix;\n"
    "out lowp vec3 col;\n"
    "void main() {\n"
    "   col = colAttr;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *fragmentShaderSource =
    "#version 320 es\n"
    "in highp vec3 col;\n"
    "out highp vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = vec4(col, 1.0);\n"
    "}\n";

void Triangle::initializeGL()
{
    // 初始化OpenGL函数,如果继承QOpenGlFunctions,必须使用这个初始化函数
    initializeOpenGLFunctions();

    //着色器
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);

    if(!m_program->link()){
        qDebug() << "link failed!\n";
    }
    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");

    m_program->bind();

    //三个顶点，
    static const GLfloat vertices[] = {
         0.0f,  0.707f,
        -0.5f, -0.5f,
         0.5f, -0.5f
    };

    //颜色，在OpenGL或GLSL中强制归一化到[0.0,1.0]之间的
    static const GLfloat colors[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    //链接顶点属性，告诉OpenGL该如何解析顶点数据
    glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

    //启用顶点属性
    glEnableVertexAttribArray(m_posAttr);
    glEnableVertexAttribArray(m_colAttr);
}

void Triangle::resizeGL(int w, int h)
{
    // OpenGL渲染窗口的尺寸大小，glViewport可以设置位置和宽高
//    glViewport(0, 0, w, h);
    Q_UNUSED(w);
    Q_UNUSED(h);

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale); //视口的宽度和高度将根据设备的像素比例进行缩放

}

void Triangle::paintGL()
{

    // 设置清屏颜色
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // 清空屏幕的颜色缓冲区,填充glClearColor设置的颜色
    glClear(GL_COLOR_BUFFER_BIT);

    m_program->bind();

    QMatrix4x4 matrix;    //4x4的单位矩阵
    matrix.perspective(60.0f, 4.0f / 3.0f, 0.1f, 100.0f);                //透视矩阵变换
    matrix.translate(0, 0, -2);                                          //平移变换
    matrix.rotate(100.0f * m_frame / screen()->refreshRate(), 0, 1, 0);  //旋转

    m_program->setUniformValue(m_matrixUniform, matrix);

    glDrawArrays(GL_TRIANGLES, 0, 3);                           //传递图元，绘制三角

    //关闭启用顶点、颜色数组等
    //......
    ++m_frame;
}

void Triangle::animation()
{
    update();
}

