#ifndef XmlCHECKBOX_H
#define XmlCHECKBOX_H

#include <HdWidgets.h>
#include <XmlAbstractObject.h>
#include <XmlModule.h>


class XmlCheckBox : public HdCheckBox, public XmlAbstractObject
{
    Q_OBJECT

public:
    explicit XmlCheckBox(QWidget *parent, pugi::xml_node node) : HdCheckBox(parent), XmlAbstractObject(node)
    {
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);

        setText(getElementName());
        setTristate(false);
        connect(this, &XmlCheckBox::checkStateChanged, this, &XmlCheckBox::updateCheckState);

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());

        setChecked(getAttributeValue("value", true));
    }

    void setText(QString txt)
    {
        QCheckBox::setText(txt);
        setAttributeValue("name", txt);
    }

    void setChecked(bool checked)
    {
        QCheckBox::setChecked(checked);
        setAttributeValue("value", checked);
    }

    static XmlCheckBox* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight)
    {
        XmlCheckBox *checkBox = new XmlCheckBox(parent, node);
        checkBox->setDynamicHeight(rowheight);
        connect(parent, &XmlModule::dpiScaleChanged, checkBox, &XmlCheckBox::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            checkBox->updateDpiScale(parent->getDpiScale());

        return checkBox;
    }

public slots:

    void updateCheckState(bool checked)
    {
        setAttributeValue("value", checked);
    }

};

#endif
