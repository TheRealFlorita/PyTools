#ifndef XMLSPINBOX_H
#define XMLSPINBOX_H

#include <HdWidgets.h>
#include <XmlModule.h>

class XmlSpinBox : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    HdBoxLayout *layout;
    HdSpinBox *spinBox;
    HdLabel *label;

public:
    explicit XmlSpinBox(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
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
        connect(this, &XmlSpinBox::dpiScaleChanged, layout, &HdBoxLayout::updateDpiScale);

        /* Label */
        label = new HdLabel(this);
        setLabelText(getAttributeValue("name", QString()));
        connect(this, &XmlSpinBox::dpiScaleChanged, label, &HdLabel::updateDpiScale);
        layout->addWidget(label);

        /* Spinbox */
        spinBox = new HdSpinBox(this);
        setSingleStep(getAttributeValue("step", 1));
        setRange(getAttributeValue("min", 0), getAttributeValue("max", 9999));
        setSuffix(getAttributeValue("suffix", QString()));
        connect(this, &XmlSpinBox::dpiScaleChanged, spinBox, &HdSpinBox::updateDpiScale);
        connect(spinBox, QOverload<int>::of(&HdSpinBox::valueChanged), this, &XmlSpinBox::updateValue);
        layout->addWidget(spinBox);

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());

        setValue(getAttributeValue("value", 0));
    }

    void setDynamicHeight(int h)
    {
        spinBox->setDynamicHeight(h);
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

    void setValue(int value)
    {
        spinBox->setValue(value);
        setAttributeValue("value", value);
    }

    int getValue()
    {
        return spinBox->value();
    }

    int value()
    {
        return spinBox->value();
    }

    void setRange(int min, int max)
    {
        spinBox->setRange(min, max);
        setAttributeValue("min", spinBox->minimum());
        setAttributeValue("max", spinBox->maximum());
    }

    void setSingleStep(int step)
    {
        spinBox->setSingleStep(step);
        setAttributeValue("step", spinBox->singleStep());
    }

    void setSuffix(QString suffix)
    {
        if (suffix.length() > 0)
            spinBox->setSuffix(" " + suffix);
        setAttributeValue("suffix", suffix);
    }

    QString getSuffix()
    {
        if (spinBox->suffix().length() > 0)
            return spinBox->suffix().remove(0,1);
        return QString();
    }

    QString text()
    {
        return spinBox->text();
    }

    static XmlSpinBox* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlSpinBox *spinBox = new XmlSpinBox(parent, node);

        /* Label width attribute */
        if (QString(node.attribute("label-width").value()).toInt() > 0)
            spinBox->setDynamicLabelWidth(QString(node.attribute("label-width").value()).toInt());
        else if (QString(node.attribute("label-width").value()) != "local")
            spinBox->setDynamicLabelWidth(labelwidth);

        spinBox->setDynamicHeight(rowheight);
        connect(parent, &XmlModule::dpiScaleChanged, spinBox, &XmlSpinBox::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            spinBox->updateDpiScale(parent->getDpiScale());

        return spinBox;
    }


public slots:

    void updateValue()
    {
        setValue(spinBox->value());
    }

};

#endif
