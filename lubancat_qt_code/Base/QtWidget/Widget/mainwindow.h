#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QMdiArea>
#include <QTreeWidgetItem>
#include <QFileInfoList>
#include <QTableWidgetItem>

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
    void CreateMenu();

    QFileInfoList searchFile(QTreeWidgetItem *root,QString path);

private slots:
    void on_setCurrentIndex();
    void menu_clicked();
    void on_cbox_frame_Shape_currentIndexChanged(int index);
    void on_cbox_frame_Shadow_currentIndexChanged(int index);
    void on_spinBox_lineWidth_valueChanged(int arg1);
    void on_spinBox_midLineWidth_valueChanged(int arg1);

    void on_checkBox_demo_clicked(bool checked);
    void on_cbox_demo_alignment_currentIndexChanged(int index);
    void on_checkBox_demo_flat_clicked(bool checked);

    void on_btn_stackedWidget_toggle_clicked();
    void on_btn_stackedWidget_insert_clicked();
    void on_btn_stackedWidget_delete_clicked();

    void on_btn_tabWidget_toggle_clicked();
    void on_btn_tabWidget_insert_clicked();
    void on_btn_tabWidget_delete_clicked();
    void on_cbox_tabWidget_currentIndexChanged(int index);

    void on_btn_mdiArea_add_clicked();
    void on_btn_mdiArea_close_clicked();
    void on_btn_mdiArea_closeAll_clicked();

    void on_cbox_mdiArea_layout_currentIndexChanged(int index);
    void on_cbox_mdiArea_model_currentIndexChanged(int index);

    void on_mdiArea_subWindowActivated(QMdiSubWindow *arg1);

    void on_btn_listWidget_add_clicked();
    void on_btn_listWidget_insert_clicked();
    void on_btn_listWidget_delete_clicked();
    void on_btn_listWidget_clear_clicked();

    void on_btn_treeWidget_path_clicked();
    void on_btn_treeWidget_delete_clicked();

    void on_cbox_table_title_clicked(bool checked);
    void on_cbox_tableWidget_EditTrigger_currentIndexChanged(int index);
    void on_cbox_tableWidget_SelectionBehavior_currentIndexChanged(int index);
    void on_cbox_tableWidget_SelectionMode_currentIndexChanged(int index);
    void on_tableWidget_cellClicked(int row, int column);
    void on_tableWidget_itemClicked(QTableWidgetItem *item);
    void on_btn_tableWidget_init_clicked();
    void on_btn_tableWidget_addrow_clicked();
    void on_btn_tableWidget_addcol_clicked();
    void on_btn_tableWidget_delrow_clicked();
    void on_btn_tableWidget_delcol_clicked();
    void on_btn_tableWidget_adjust_clicked();
    void on_btn_tableWidget_cleartable_clicked();
    void on_btn_tableWidget_cleardate_clicked();
    void on_btn_tableWidget_insertitem_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
