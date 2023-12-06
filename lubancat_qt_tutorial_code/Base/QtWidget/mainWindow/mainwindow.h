#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QSpinBox>
#include <QFontComboBox>
#include <QActionGroup>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    QAction *aboutQtAct;
    QLabel     *labFile;       //状态栏里用来显示
    QLabel     *labInfo;       //状态栏里使用信息
    QLabel     *labRow;        //状态栏里显示行
    QLabel     *labColumn;     //状态栏里使用信息

    QSpinBox        *fontSize;  //字体大小
    QFontComboBox   *fontName; //字体名称

    void    initUI();             //UI初始化

private:
    Ui::MainWindow *ui;

private slots:
    //  自定义槽函数
    void do_fontSize_changed(int fontSize);            //改变字号
    void do_fontSelected(const QFont &font);           //字体选择

    //
    void on_plain_textedit_cursorPositionChanged();
    void on_action_new_triggered();
    void on_action_save_triggered();
    void on_action_open_triggered();
    void on_plain_textedit_copyAvailable(bool b);
};
#endif // MAINWINDOW_H
