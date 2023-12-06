#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QStyledItemDelegate>
#include <QObject>
#include <QComboBox>

class ComboboxDelegate : public QStyledItemDelegate
{
public:
    explicit ComboboxDelegate(QObject *parent = nullptr);

    // 自定义函数，初始化设置列表内容
    void    setItems(QStringList items);

    // 编辑组件创建
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index)const;

    // 从数据模型获取数据，显示到代理组件中
    void setEditorData(QWidget *editor, const QModelIndex &index)const;

    // 将代理组件的数据，保存到数据模型中
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index)const;

    // 更新代理编辑组件的大小
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index)const;

private:
    QStringList itemList; //选择列表

};

#endif // COMBOBOXDELEGATE_H
