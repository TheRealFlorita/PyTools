#ifndef XMLCOMBOBOXDELEGATE_H
#define XMLCOMBOBOXDELEGATE_H

#include <QComboBox>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QVariant>

#include <XmlAbstractObject.h>
#include <Settings.h>


class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

private:
    QStringList Items;
    QStyle *style;
    QColor fillColor;

public:
    ComboBoxDelegate(QWidget *parent = Q_NULLPTR) : QStyledItemDelegate(parent)
    {
        if (parent != Q_NULLPTR)
            style = parent->style();
        fillColor = settings.getColor("color/theme-light");
    }

    ComboBoxDelegate(QStringList &boxItems, QWidget *parent = Q_NULLPTR) : QStyledItemDelegate(parent)
    {
        Items = QStringList(boxItems);
        style = parent->style();
        fillColor = settings.getColor("color/theme-light");
    }

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)

        QComboBox* editor = new QComboBox(parent);
        editor->setObjectName("ComboBoxDelegate");
        editor->addItems(Items);
        return editor;
    }

    void setFillColor(QColor color)
    {
        fillColor = QColor(color);
    }

    void setItems(QStringList &boxItems)
    {
        Items = QStringList(boxItems);
    }

    QStringList getItems()
    {
        return Items;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        QComboBox *comboBox = static_cast<QComboBox*>(editor);

        if (Items.contains(index.data(Qt::EditRole).toString(), Qt::CaseSensitive))
            comboBox->setCurrentText(index.data(Qt::EditRole).toString());
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        QComboBox *comboBox = static_cast<QComboBox*>(editor);
        model->setData(index, comboBox->currentText(), Qt::EditRole);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const override
    {
        editor->setGeometry(option.rect);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem myOption = option;

        QString text = "...";

        if (index.data(Qt::EditRole).toString() != "")
            if (Items.contains(index.data(Qt::EditRole).toString(), Qt::CaseSensitive))
                text = index.data(Qt::EditRole).toString();

        myOption.text = text;

        painter->fillRect(myOption.rect, settings.getColor("color/theme-medium"));

        QPainterPath path;
        path.addRoundedRect(myOption.rect, 0, 0);
        painter->fillPath(path, fillColor);

        style->drawControl(style->CE_ItemViewItem, &myOption, painter);
    }

};


class XmlComboBoxDelegate : public ComboBoxDelegate, public XmlAbstractObject
{
    Q_OBJECT

public:
    explicit XmlComboBoxDelegate(QWidget *parent, pugi::xml_node node) : ComboBoxDelegate(parent), XmlAbstractObject(node)
    {
        if (!xmlNode.child("items"))
            xmlNode.prepend_child("items");

        /* Read items */
        QStringList texts;
        pugi::xpath_node_set item_nodes = xmlNode.select_nodes("./items/item");
        for (size_t i = 0; i < item_nodes.size(); ++i)
        {
            pugi::xml_node item_node = item_nodes[i].node();

            QString text = item_node.attribute("value").value();
            if (!text.trimmed().isEmpty() && !texts.contains(text))
                texts.append(item_node.attribute("value").value());
            else
                item_node.parent().remove_child(item_node);

            item_node.remove_attribute("index");
        }

        setItems(texts);
    }
};

#endif
