#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btn_openFile_clicked();
    void on_btn_openFiles_clicked();
    void on_btn_openDir_clicked();
    void on_btn_save_clicked();

    void on_btn_font_clicked();
    void on_btn_color_clicked();

    void on_btn_critical_clicked();
    void on_btn_information_clicked();
    void on_btn_question_clicked();
    void on_btn_warning_clicked();
    void on_btn_about_clicked();
    void on_btn_aboutQt_clicked();

    void on_btn_error_clicked();

    void on_btn_getDouble_clicked();
    void on_btn_getInt_clicked();
    void on_btn_getItem_clicked();
    void on_btn_getMultiLineText_clicked();
    void on_btn_getText_clicked();

    void on_btn_progress_clicked();

    void on_btn_print_clicked();

    void on_btn_logo_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
