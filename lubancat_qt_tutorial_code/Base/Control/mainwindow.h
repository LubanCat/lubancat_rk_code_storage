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

private:
    void initInterface();
    void initControl();

private slots:
    void on_setCurrentIndex();
    void pushbutton_clicked();
    void toolbutton_clicked();
    void radiobutton_clicked();
    void checkbox_stateChange(const int state);
    void on_fontComboBox_currentFontChanged(const QFont &f);
    void on_comboBox_currentIndexChanged(const QString &arg1);

    void on_spinBox_R_valueChanged(int);
    void on_spinBox_G_valueChanged(int);
    void on_doubleSpinBox_B_valueChanged(double);

    void lineedit_textEdited(const QString str);
    void lineedit_returnPressed();

    void handletimeout();

    void on_calendarWidget_clicked(const QDate &date);
    void on_dateEdit_userDateChanged(const QDate &date);
    void on_timeEdit_userTimeChanged(const QTime &time);
    void on_dateTimeEdit_dateTimeChanged(const QDateTime &dateTime);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
