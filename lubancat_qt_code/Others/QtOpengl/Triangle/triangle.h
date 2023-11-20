#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <QOpenGLWidget>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector3D>
#include <QDebug>
#include <QTimer>
#include <QPainter>

class Triangle : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    explicit  Triangle(QWidget *parent = nullptr);
    ~Triangle();

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

private slots:
    void animation();

private:
    QOpenGLShaderProgram *m_program = nullptr;

    GLint m_posAttr = 0;
    GLint m_colAttr = 0;
    GLint m_matrixUniform = 0;
    int m_frame = 0;
};

#endif // TRIANGLE_H
