#ifndef XMLLINEEDIT_H
#define XMLLINEEDIT_H

#include <HdWidgets.h>

#include <XmlAbstractObject.h>
#include <XmlModule.h>


class XmlLineEdit : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

public:
    HdBoxLayout *layout;
    HdLineEdit *lineEdit;
    HdLabel *label;

public:
    explicit XmlLineEdit(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
    {
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);

        /* Create layout */
        if (getAttributeValue("alignment", QString()) == "vertical")
            layout = new HdBoxLayout(QBoxLayout::TopToBottom, this);
        else
            layout = new HdBoxLayout(QBoxLayout::LeftToRight, this);
        layout->setContentsMargins(0,0,0,0);
        layout->setDynamicSpacing(3);
        connect(this, &XmlLineEdit::dpiScaleChanged, layout, &HdBoxLayout::updateDpiScale);

        /* Label */
        label = new HdLabel(this);
        setLabelText(getAttributeValue("name", QString()));
        connect(this, &XmlLineEdit::dpiScaleChanged, label, &HdLabel::updateDpiScale);
        layout->addWidget(label);

        /* Line edit */
        lineEdit = new HdLineEdit(this);
        lineEdit->setMinimumWidth(10);
        lineEdit->setFocusPolicy(Qt::StrongFocus);
        setText(getAttributeValue("value", QString()));
        connect(this, &XmlLineEdit::dpiScaleChanged, lineEdit, &HdLineEdit::updateDpiScale);
        connect(lineEdit, &HdLineEdit::editingFinished, this, &XmlLineEdit::updateText);
        layout->addWidget(lineEdit);

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());

        setText(getAttributeValue("value", QString()));
    }

    void setDynamicHeight(int h)
    {
        lineEdit->setDynamicHeight(h);
    }

    void setDynamicLabelWidth(int w)
    {
        label->setDynamicWidth(w);
    }

    void setLabelText(QString text)
    {
        label->setText(text);
        setAttributeValue("name", text);
    }

    void setText(QString text)
    {
        lineEdit->setText(text);
        setAttributeValue("value", text);
    }

    QString getText()
    {
        return lineEdit->text();
    }

    static XmlLineEdit* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlLineEdit *lineEdit = new XmlLineEdit(parent, node);

        /* Label width attribute */
        if (QString(node.attribute("label-width").value()).toInt() > 0)
            lineEdit->setDynamicLabelWidth(QString(node.attribute("label-width").value()).toInt());
        else if (QString(node.attribute("label-width").value()) != "local")
            lineEdit->setDynamicLabelWidth(labelwidth);

        lineEdit->setDynamicHeight(rowheight);
        connect(parent, &XmlModule::dpiScaleChanged, lineEdit, &XmlLineEdit::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            lineEdit->updateDpiScale(parent->getDpiScale());

        return lineEdit;
    }

public slots:

    void updateText()
    {
        setAttributeValue("value", lineEdit->text());
    }

};

#endif
