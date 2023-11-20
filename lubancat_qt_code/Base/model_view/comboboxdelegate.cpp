#include "comboboxdelegate.h"

ComboboxDelegate::ComboboxDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void ComboboxDelegate::setItems(QStringList items)
{
    itemList=items;
}

QWidget *ComboboxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    // 创建一个QComboBox
    QComboBox *editor = new QComboBox(parent);
    // 初始QComboBox下拉列表
    for (int i=0;i<itemList.count();i++)
        editor->addItem(itemList.at(i));

    return editor;
}

void ComboboxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString str = index.model()->data(index, Qt::EditRole).toString();

    // 获具体comboBox
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    // 设置内容
    comboBox->setCurrentText(str);
}

void ComboboxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);

    // 获取当前数据
    QString str = comboBox->currentText();

    // 设置模型数据
    model->setData(index, str, Qt::EditRole);
}

void ComboboxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    // 调整大小位置
    editor->setGeometry(option.rect);
}


