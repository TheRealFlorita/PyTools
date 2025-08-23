#ifndef XMLCOMBOBOX_H
#define XMLCOMBOBOX_H

#include <HdLayouts.h>
#include <HdWidgets.h>

#include <XmlAbstractObject.h>
#include <XmlModule.h>


class XmlComboBox : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

protected:
    HdBoxLayout *layout;
    HdComboBox *comboBox;
    HdLabel *label;

public:
    explicit XmlComboBox(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
    {
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);

        /* Ensure items node exists */
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
                texts.append(text);
            else
                item_node.parent().remove_child(item_node);

            item_node.remove_attribute("index");
        }

        /* Create layout */
        if (getAttributeValue("alignment", QString()) == "vertical")
            layout = new HdBoxLayout(QBoxLayout::TopToBottom, this);
        else
            layout = new HdBoxLayout(QBoxLayout::LeftToRight, this);
        layout->setContentsMargins(0,0,0,0);
        layout->setDynamicSpacing(3);
        connect(this, &XmlComboBox::dpiScaleChanged, layout, &HdBoxLayout::updateDpiScale);

        /* Label */
        label = new HdLabel(this);
        setLabelText(getElementName());
        connect(this, &XmlComboBox::dpiScaleChanged, label, &HdLabel::updateDpiScale);
        layout->addWidget(label);

        /* Combobox */
        comboBox = new HdComboBox(this);
        comboBox->setDuplicatesEnabled(false);
        comboBox->addItems(texts);
        connect(this, &XmlComboBox::dpiScaleChanged, comboBox, &HdComboBox::updateDpiScale);
        connect(comboBox, QOverload<int>::of(&HdComboBox::currentIndexChanged), this, &XmlComboBox::updateIndex);
        layout->addWidget(comboBox);

        if (comboBox->count() > 0)
            setCurrentText(getAttributeValue("value", comboBox->itemText(0)));

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());
    }

    void setDynamicHeight(int h)
    {
        comboBox->setDynamicHeight(h);
    }

    void setDynamicLabelWidth(int w)
    {
        label->setDynamicWidth(w);
    }

    void setLabelText(QString txt)
    {
        label->setText(txt);
        setAttributeValue("name", txt);
    }

    void setCurrentIndex(int index)
    {
        comboBox->setCurrentIndex(index);
        setAttributeValue("value", comboBox->currentText());
    }

    int getCurrentIndex()
    {
        return comboBox->currentIndex();
    }

    void setCurrentText(QString txt)
    {
        comboBox->setCurrentText(txt);
        setAttributeValue("value", comboBox->currentText());
    }

    QString getCurrentText()
    {
        return comboBox->currentText();
    }

    HdBoxLayout* getLayout()
    {
        return layout;
    }

    HdComboBox* getComboBox()
    {
        return comboBox;
    }    

    static XmlComboBox* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlComboBox *comboBox = new XmlComboBox(parent, node);

        /* Label width attribute */
        if (QString(node.attribute("label-width").value()).toInt() > 0)
            comboBox->setDynamicLabelWidth(QString(node.attribute("label-width").value()).toInt());
        else if (QString(node.attribute("label-width").value()) != "local")
            comboBox->setDynamicLabelWidth(labelwidth);

        comboBox->setDynamicHeight(rowheight);
        connect(parent, &XmlModule::dpiScaleChanged, comboBox, &XmlComboBox::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            comboBox->updateDpiScale(parent->getDpiScale());

        return comboBox;
    }


public slots:

    void updateIndex(int index)
    {
        Q_UNUSED(index)
        setAttributeValue("value", comboBox->currentText());
    }

};

#endif
