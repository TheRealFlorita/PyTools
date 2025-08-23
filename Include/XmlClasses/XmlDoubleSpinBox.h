#ifndef XMLDOUBLESPINBOX_H
#define XMLDOUBLESPINBOX_H

#include <HdWidgets.h>
#include <XmlAbstractObject.h>
#include <XmlModule.h>


class XmlDoubleSpinBox : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    HdBoxLayout *layout;
    HdDoubleSpinBox *spinBox;
    HdLabel *label;
    int ndecimals;

public:
    explicit XmlDoubleSpinBox(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
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
        connect(this, &XmlDoubleSpinBox::dpiScaleChanged, layout, &HdBoxLayout::updateDpiScale);

        /* Label */
        label = new HdLabel(this);
        setLabelText(getAttributeValue("name", QString()));
        connect(this, &XmlDoubleSpinBox::dpiScaleChanged, label, &HdLabel::updateDpiScale);
        layout->addWidget(label);

        /* Spinbox */
        spinBox = new HdDoubleSpinBox(this);
        setDecimals(std::max(1, getAttributeValue("decimals", 1)));
        setSingleStep(getAttributeValue("step", QVariant(0.1)).toDouble());
        setRange(getAttributeValue("min", 0.0), getAttributeValue("max",99.9));
        setSuffix(getAttributeValue("suffix", QString()));
        connect(this, &XmlDoubleSpinBox::dpiScaleChanged, spinBox, &HdDoubleSpinBox::updateDpiScale);
        connect(spinBox, QOverload<double>::of(&HdDoubleSpinBox::valueChanged), this, &XmlDoubleSpinBox::updateValue);
        layout->addWidget(spinBox);

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());

        setValue(getAttributeValue("value", 0.0));
    }

    void setDynamicHeight(int h)
    {
        spinBox->setDynamicHeight(h);
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

    void setValue(double value)
    {
        spinBox->setValue(value);
        setAttributeValue("value",spinBox->value(), getDecimals());
    }

    double getValue()
    {
        return spinBox->value();
    }

    double value()
    {
        return spinBox->value();
    }

    void setRange(double min, double max)
    {
        spinBox->setRange(min, max);
        setAttributeValue("min", spinBox->minimum(), getDecimals());
        setAttributeValue("max", spinBox->maximum(), getDecimals());
    }

    void setSingleStep(double step)
    {
        spinBox->setSingleStep(step);
        setAttributeValue("step", spinBox->singleStep(), getDecimals());
    }

    void setDecimals(int n)
    {
        spinBox->setDecimals(n);
        setAttributeValue("decimals", spinBox->decimals());
    }

    int getDecimals()
    {
        return spinBox->decimals();
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

    static XmlDoubleSpinBox* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlDoubleSpinBox *spinBox = new XmlDoubleSpinBox(parent, node);

        /* Label width attribute */
        if (QString(node.attribute("label-width").value()).toInt() > 0)
            spinBox->setDynamicLabelWidth(QString(node.attribute("label-width").value()).toInt());
        else if (QString(node.attribute("label-width").value()) != "local")
            spinBox->setDynamicLabelWidth(labelwidth);

        spinBox->setDynamicHeight(rowheight);
        connect(parent, &XmlModule::dpiScaleChanged, spinBox, &XmlDoubleSpinBox::updateDpiScale);

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
