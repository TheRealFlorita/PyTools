#ifndef XMLEXPANDABLEBOX_H
#define XMLEXPANDABLEBOX_H

#include <QTimer>
#include <HdShadowEffect.h>
#include <HdWidgets.h>

#include <XmlAbstractObject.h>
#include <XmlModule.h>


class XmlExpandableBox : public HdGroupBox, public XmlAbstractObject
{
    Q_OBJECT

private:
    XmlModule *module;
    HdBoxLayout *boxlayout;
    int rowheight;
    int labelwidth;
    HdSpinBox *spinBox;
    QList<HdWidget*> rowWidgets;

public:
    explicit XmlExpandableBox(XmlModule *parent, pugi::xml_node node, int label_width = 0) : HdGroupBox(parent), XmlAbstractObject(node)
    {
        module = parent;
        rowheight = module->getRowHeight();

        if (label_width <= 0)
            labelwidth = parent->getObjectLabelWidth(node);
        else
            labelwidth = label_width;

        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        setTitle(getAttributeValue("name", QString()));

        /* Checkable group box */
        if (getAttributeValue("check-state", -1) >= 0)
        {
            setCheckable(true);
            setChecked(getAttributeValue("check-state", true));
            connect(this, &XmlExpandableBox::toggled, this, &XmlExpandableBox::updateCheckState);
        }
        else if (getAttributeValue("check-state", -1) != -1)
            deleteAttribute("check-state");

        /* Label width attribute */
        if (node.attribute("label-width"))
        {
            if (QString(node.attribute("label-width").value()) == "local")
                labelwidth = parent->getObjectLabelWidth(node);
            else if (QString(node.attribute("label-width").value()).toInt() > 0)
                labelwidth = QString(node.attribute("label-width").value()).toInt();
        }

        /* Ensure items element exists*/
        if (!xmlNode.child("items"))
            xmlNode.prepend_child("items");

        /* Ensure rows element exists*/
        if (!xmlNode.child("rows"))
            xmlNode.append_child("rows");

        /* Create base layout */
        QBoxLayout *baselayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        baselayout->setSpacing(0);
        baselayout->setContentsMargins(0,0,0,0);

        /* Create grid layout */
        HdGridLayout *gridlayout = new HdGridLayout();
        gridlayout->setSpacing(0);
        gridlayout->setDynamicMargins(0,8,0,0);
        gridlayout->setDynamicVerticalSpacing(0);
        gridlayout->setDynamicHorizontalSpacing(24);

        baselayout->addLayout(gridlayout);
        connect(this, &XmlExpandableBox::dpiScaleChanged, gridlayout, &HdGridLayout::updateDpiScale);

        /* Create layout for expandable box elements */
        boxlayout = new HdBoxLayout(QBoxLayout::TopToBottom);

        if (getNumberOfItems() <= 1)
            boxlayout->setDynamicSpacing(8);
        else
            boxlayout->setDynamicSpacing(24);

        boxlayout->setDynamicMargins(0,0,0,0);
        gridlayout->addLayout(boxlayout,0,0,-1,1);
        connect(this, &XmlExpandableBox::dpiScaleChanged, boxlayout, &HdBoxLayout::updateDpiScale);

        /* Add spacer that stretches if the group box is in a horizontal layout */
        QWidget *spacer = new QWidget(this);
        spacer->setContentsMargins(0,0,0,0);
        spacer->setMinimumHeight(0);
        spacer->setFixedWidth(0);
        spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        baselayout->addWidget(spacer);

        /* Add spin box to control the number of rows */
        spinBox = new HdSpinBox(this);
        spinBox->setUseSizeHintWidth(true);
        spinBox->setDynamicHeight(rowheight);
        setRange(getAttributeValue("min", 0), getAttributeValue("max", 10));
        setSingleStep(1);
        setValue(getAttributeValue("value", 1));
        updateRows(getValue());
        connect(this, &XmlExpandableBox::dpiScaleChanged, spinBox, &HdSpinBox::updateDpiScale);
        connect(spinBox, qOverload<int>(&QSpinBox::valueChanged), this, &XmlExpandableBox::updateRows);
        gridlayout->addWidget(spinBox,0,1,1,1,Qt::AlignTop|Qt::AlignRight);

        QString xpath = QString::fromStdString(xmlNode.path()).trimmed().chopped(getElementType().length()+1);
        if (xpath.contains("group-box") || xpath.contains("expandable-box") || xpath.contains("selection-box"))
        {
            /* Set styling */
            if (getAttributeValue("name", QString()).isEmpty())
                setObjectName("NoTitleNested");
            else
                setObjectName("Nested");
        }
        else
        {
            /* Set styling */
            if (getAttributeValue("name", QString()).isEmpty())
                setObjectName("NoTitle");

            if (settings.getValue("drop-shadow/blur-radius").toInt() > 0)
            {
                HdShadowEffect *effect = new HdShadowEffect(this, qreal(1.0), settings.getValue("drop-shadow/blur-radius").toReal(), settings.getColor("drop-shadow/color"));
                connect(this, &XmlExpandableBox::dpiScaleChanged, effect, &HdShadowEffect::updateDpiScale);
                setGraphicsEffect(effect);
            }
        }

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());
    }

    void setValue(int value)
    {
        spinBox->setValue(value);
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

    int getNumberOfItems()
    {
        pugi::xpath_node_set item_nodes = xmlNode.select_nodes("./items//*[@name]");
        return int(item_nodes.size());
    }

    void updateRows(int value)
    {      
        setAttributeValue("value", value);

        int count = rowWidgets.length();

        /* Delete rows */
        if (count > value)
        {
            while (value < rowWidgets.length())
            {
                HdWidget *last = rowWidgets.takeLast();
                boxlayout->removeWidget(last);
                last->layout()->disconnect();
                last->deleteLater();

                pugi::xml_node row_node = getRowNode(rowWidgets.length());
                if (isEmptyRow(row_node))
                    row_node.parent().remove_child(row_node);
            }
        }
        /* Add rows */
        else
        {
            for (int i = count; i < value; ++i)
            {
                hide();
                pugi::xml_node row_node = getRowNode(i);
                HdWidget *row_widget = newRowWidget();
                HdBoxLayout *rowlayout = static_cast<HdBoxLayout*>(row_widget->layout());

                for (pugi::xml_node child_node: row_node.children())
                    if (XmlModule::generateXmlObject(module, Q_NULLPTR, child_node, rowheight, labelwidth, true))
                        XmlModule::generateXmlObject(module, rowlayout, child_node, rowheight, labelwidth, false);
                show();
            }
        }
    }

    void setChecked(bool checked)
    {
        if (isCheckable())
        {
            HdGroupBox::setChecked(checked);
            setAttributeValue("check-state", checked);
        }
    }

    void updateCheckState(bool checked)
    {
        if (isCheckable())
            setAttributeValue("check-state", checked);
    }

    static XmlExpandableBox* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int row_height, int label_width)
    {
        Q_UNUSED(row_height);
        XmlExpandableBox *expandableBox = new XmlExpandableBox(parent, node, label_width);
        connect(parent, &XmlModule::dpiScaleChanged, expandableBox, &XmlExpandableBox::updateDpiScale);
        return expandableBox;
    }

private:

    pugi::xml_node getRowNode(int index)
    {
        QString xpath = "./rows/row[@i='" + QString::number(index) + "']";
        pugi::xml_node row_node = xmlNode.select_node(xpath.toStdString().c_str()).node();

        if (!row_node)
        {
            pugi::xpath_node_set row_nodes = xmlNode.select_nodes("./rows/row");
            for (int n = 0; n < int(row_nodes.size()); ++n)
            {
                pugi::xml_node child_node = row_nodes[n].node();

                if (QString(child_node.attribute("i").value()).toInt() > index)
                {
                    row_node = xmlNode.child("rows").insert_child_before("row", child_node);
                    row_node.append_attribute("i").set_value(index);
                    break;
                }
            }

            if (!row_node)
            {
                row_node = xmlNode.child("rows").append_child("row");
                row_node.append_attribute("i").set_value(index);
            }

            for (pugi::xml_node child_node: xmlNode.child("items").children())
                row_node.append_copy(child_node);
        }
        return row_node;
    }

    bool isEmptyRow(pugi::xml_node row_node)
    {
        const QStringList node_types =  {"file-selection", "folder-selection", "line-edit", "multi-file-selection", "text-box"};

        /* Remove row node if children are empty */
        pugi::xpath_node_set child_nodes = row_node.select_nodes(".//*");
        for (int i = 0; i < int(child_nodes.size()); ++i)
        {
            pugi::xml_node child_node = child_nodes[i].node();
            if (node_types.contains(child_node.name()) && child_node.attribute("value"))
                if (!QString(child_node.attribute("value").value()).trimmed().isEmpty())
                    return false;
        }

        pugi::xpath_node_set item_nodes = row_node.select_nodes(".//table/rows/row/item");
        for (int i = 0; i < int(item_nodes.size()); ++i)
        {
            pugi::xml_node item_node = item_nodes[i].node();
            if (node_types.contains(item_node.name()) && item_node.attribute("value"))
                if (!QString(item_node.attribute("value").value()).trimmed().isEmpty())
                    return false;
        }
        return true;
    }

    HdWidget* newRowWidget()
    {
        HdWidget *rowWidget = new HdWidget(this);
        rowWidget->setObjectName("Transparent");
        rowWidget->setStyleSheet("QWidget#Transparent {background-color: transparent;}");
        connect(this, &XmlExpandableBox::dpiScaleChanged, rowWidget, &HdWidget::updateDpiScale);

        HdBoxLayout *rowlayout = new HdBoxLayout(QBoxLayout::TopToBottom, rowWidget);
        rowlayout->setDynamicSpacing(8);
        boxlayout->addWidget(rowWidget);
        connect(this, &XmlExpandableBox::dpiScaleChanged, rowlayout, &HdBoxLayout::updateDpiScale);
        rowWidgets.append(rowWidget);
        return rowWidget;
    }

};

#endif
