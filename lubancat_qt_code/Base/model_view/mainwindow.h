#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QStringListModel>
#include <QInputDialog>
#include <QStandardItemModel>

#include "comboboxdelegate.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init_StringListModel();
    void init_StandardItemModel();

private slots:
    void StringListModel_add();
    void StringListModel_remove();
    void StringListModel_initList();
    void StringListModel_instert();

private:
    Ui::MainWindow *ui;

    QStringList list;
    QStringListModel listModel;

    QStandardItemModel  *m_model;
    ComboboxDelegate * combo_delegate;
};
#endif // MAINWINDOW_H
