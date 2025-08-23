#ifndef XMLLAYOUT_H
#define XMLLAYOUT_H

#include <HdWidgets.h>

#include <XmlAbstractObject.h>
#include <XmlModule.h>

class XmlSpacer : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

public:
    explicit XmlSpacer(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        setContentsMargins(0,0,0,0);
        setMinimumSize(getAttributeValue("min-width", 0), getAttributeValue("min-height", 0));

        if (getAttribute("width"))
            setDynamicWidth(getAttributeValue("width", 0));
        if (getAttribute("height"))
            setDynamicHeight(getAttributeValue("height", 0));
    }

    static XmlSpacer* createFromXmlNode(XmlModule *parent, pugi::xml_node node)
    {
        XmlSpacer *spacer = new XmlSpacer(parent,node);
        connect(parent, &XmlModule::dpiScaleChanged, spacer, &XmlSpacer::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            spacer->updateDpiScale(parent->getDpiScale());

        return spacer;
    }
};


class XmlBoxLayout : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    HdBoxLayout *box;

public:
    explicit XmlBoxLayout(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
    {
        box = new HdBoxLayout(QBoxLayout::TopToBottom, this);

        if (getElementType() == "horizontal-layout")
            box->setDirection(QBoxLayout::LeftToRight);

        int mg = getAttributeValue("margin", 0);
        box->setDynamicMargins(getAttributeValue("margin-left", mg),
                               getAttributeValue("margin-top", mg),
                               getAttributeValue("margin-right", mg),
                               getAttributeValue("margin-bottom", mg));
        box->setDynamicSpacing(std::max(0, getAttributeValue("spacing", 0)));
        connect(this, &XmlBoxLayout::dpiScaleChanged, box, &HdBoxLayout::updateDpiScale);
    }

    HdBoxLayout* getLayout()
    {
        return box;
    }

    static XmlBoxLayout* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlBoxLayout *boxLayout = new XmlBoxLayout(parent, node);
        connect(parent, &XmlModule::dpiScaleChanged, boxLayout, &XmlBoxLayout::updateDpiScale);

        /* Label width attribute */
        if (node.attribute("label-width"))
        {
            if (QString(node.attribute("label-width").value()) == "local")
                labelwidth = parent->getObjectLabelWidth(node);
            else if (QString(node.attribute("label-width").value()).toInt() > 0)
                labelwidth = QString(node.attribute("label-width").value()).toInt();
        }

        for (pugi::xml_node child_node: node.children())
            if (!XmlModule::generateXmlObject(parent, boxLayout->getLayout(), child_node, rowheight, labelwidth, true))
                return boxLayout;

        for (pugi::xml_node child_node: node.children())
            XmlModule::generateXmlObject(parent, boxLayout->getLayout(), child_node, rowheight, labelwidth, false);

        if (parent->getDpiScale() != 1.0)
            boxLayout->updateDpiScale(parent->getDpiScale());

        return boxLayout;
    }

};


class XmlGridLayout : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    HdGridLayout *grid;

public:
    explicit XmlGridLayout(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
    {
        grid = new HdGridLayout(this);

        int mg = getAttributeValue("margin", 0);
        grid->setDynamicMargins(getAttributeValue("margin-left", mg),
                                getAttributeValue("margin-top", mg),
                                getAttributeValue("margin-right", mg),
                                getAttributeValue("margin-bottom", mg));
        grid->setDynamicSpacing(std::max(0, getAttributeValue("spacing", 0)));

        int sp = getAttributeValue("spacing", 0);
        grid->setDynamicHorizontalSpacing(std::max(0, getAttributeValue("horizontal-spacing", sp)));
        grid->setDynamicVerticalSpacing(std::max(0, getAttributeValue("vertical-spacing", sp)));

        for (int i = 0; i < getAttributeValue("row-count", 0); ++i)
            grid->setRowStretch(i,1);

        for (int j = 0; j < getAttributeValue("column-count", 0); ++j)
            grid->setColumnStretch(j,1);

        connect(this, &XmlGridLayout::dpiScaleChanged, grid, &HdGridLayout::updateDpiScale);
    }

    HdGridLayout* getLayout()
    {
        return grid;
    }

    static XmlGridLayout* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlGridLayout *gridLayout = new XmlGridLayout(parent, node);
        connect(parent, &XmlModule::dpiScaleChanged, gridLayout, &XmlGridLayout::updateDpiScale);

        /* Label width attribute */
        if (node.attribute("label-width"))
        {
            if (QString(node.attribute("label-width").value()) == "local")
                labelwidth = parent->getObjectLabelWidth(node);
            else if (QString(node.attribute("label-width").value()).toInt() > 0)
                labelwidth = QString(node.attribute("label-width").value()).toInt();
        }

        /* Iterate layout items */
        for (pugi::xml_node item_node: node.children("layout-item"))
        {
            int irow = std::abs(QString(item_node.attribute("row").value()).toInt());
            int icolumn = std::abs(QString(item_node.attribute("column").value()).toInt());
            int irow_span = QString(item_node.attribute("row-span").value()).toInt();
            int icolumn_span = QString(item_node.attribute("column-span").value()).toInt();

            if (irow_span <= -2 || irow_span == 0)
                irow_span = 1;

            if (icolumn_span <= -2 || icolumn_span == 0)
                icolumn_span = 1;

            HdBoxLayout *itemLayout = new HdBoxLayout(QBoxLayout::LeftToRight);
            gridLayout->getLayout()->addLayout(itemLayout, irow, icolumn, irow_span, icolumn_span);

            for (pugi::xml_node child_node: item_node.children())
                if (XmlModule::generateXmlObject(parent, itemLayout, child_node, rowheight, labelwidth, true))
                    return gridLayout;

            for (pugi::xml_node child_node: item_node.children())
                XmlModule::generateXmlObject(parent, itemLayout, child_node, rowheight, labelwidth, false);
        }

        if (parent->getDpiScale() != 1.0)
            gridLayout->updateDpiScale(parent->getDpiScale());

        return gridLayout;
    }

};

#endif
