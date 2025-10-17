#ifndef XMLSELECTIONBOX_H
#define XMLSELECTIONBOX_H

#include <HdShadowEffect.h>
#include <HdWidgets.h>

#include <XmlAbstractObject.h>
#include <XmlModule.h>


class XmlSelectionBox : public HdGroupBox, public XmlAbstractObject
{
    Q_OBJECT

private:
    XmlModule *module;
    HdBoxLayout *boxlayout;
    int rowheight;
    int labelwidth;
    HdComboBox *comboBox;
    QList<HdWidget*> rowWidgets;

public:
    explicit XmlSelectionBox(XmlModule *parent, pugi::xml_node node, int label_width = 0) : HdGroupBox(parent), XmlAbstractObject(node)
    {
        module = parent;
        rowheight = module->getRowHeight();

        if (label_width <= 0)
            labelwidth = parent->getObjectLabelWidth(node);
        else
            labelwidth = label_width;

        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        setTitle(getAttributeValue("name", QString()));

        /* Checkable selection box */
        if (getAttributeValue("check-state", -1) >= 0)
        {
            setCheckable(true);
            setChecked(getAttributeValue("check-state", true));
            connect(this, &XmlSelectionBox::toggled, this, &XmlSelectionBox::updateCheckState);
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

        /* Create base layout */
        QBoxLayout *baselayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        baselayout->setSpacing(0);
        baselayout->setContentsMargins(0,0,0,0);

        /* Create grid layout */
        HdGridLayout *gridlayout = new HdGridLayout();
        gridlayout->setSpacing(0);
        gridlayout->setDynamicMargins(0,8,0,0);
        gridlayout->setDynamicVerticalSpacing(24);
        gridlayout->setDynamicHorizontalSpacing(0);

        baselayout->addLayout(gridlayout);
        connect(this, &XmlSelectionBox::dpiScaleChanged, gridlayout, &HdGridLayout::updateDpiScale);

        /* Create layout for selection box elements */
        if (getAttributeValue("alignment", QString()) == "horizontal")
            boxlayout = new HdBoxLayout(QBoxLayout::LeftToRight);
        else
            boxlayout = new HdBoxLayout(QBoxLayout::TopToBottom);
        boxlayout->setDynamicSpacing(8);
        boxlayout->setDynamicMargins(0,0,0,0);
        gridlayout->addLayout(boxlayout,1,0,-1,10);
        connect(this, &XmlSelectionBox::dpiScaleChanged, boxlayout, &HdBoxLayout::updateDpiScale);

        /* Add spacer that stretches if the selection box is in a horizontal layout */
        QWidget *spacer = new QWidget(this);
        spacer->setContentsMargins(0,0,0,0);
        spacer->setMinimumHeight(0);
        spacer->setFixedWidth(0);
        spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        baselayout->addWidget(spacer);

        /* Add combo box to control layout */
        comboBox = new HdComboBox(this);
        comboBox->setDynamicHeight(rowheight);
        comboBox->setDuplicatesEnabled(false);
        comboBox->addItems(texts);
        connect(this, &XmlSelectionBox::dpiScaleChanged, comboBox, &HdComboBox::updateDpiScale);
        connect(comboBox, QOverload<int>::of(&HdComboBox::currentIndexChanged), this, &XmlSelectionBox::updateItem);

        if (comboBox->count() > 0)
            comboBox->setCurrentText(getAttributeValue("value", comboBox->itemText(0)));
        updateItem(comboBox->currentIndex());

        if (getAttribute("label"))
        {
            HdLabel *label = new HdLabel(this);
            label->setDynamicSize(labelwidth, rowheight);
            label->setText(getAttributeValue("label", QString()));
            connect(this, &XmlSelectionBox::dpiScaleChanged, label, &HdLabel::updateDpiScale);

            HdBoxLayout *horlayout = new HdBoxLayout(HdBoxLayout::LeftToRight);
            horlayout->setDynamicSpacing(3);
            horlayout->addWidget(label);
            horlayout->addWidget(comboBox);
            gridlayout->addLayout(horlayout,0,0,1,-1,Qt::AlignTop|Qt::AlignLeft);
            connect(this, &XmlSelectionBox::dpiScaleChanged, horlayout, &HdBoxLayout::updateDpiScale);
        }
        else
            gridlayout->addWidget(comboBox,0,0,1,1,Qt::AlignTop|Qt::AlignLeft);

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
                connect(this, &XmlSelectionBox::dpiScaleChanged, effect, &HdShadowEffect::updateDpiScale);
                setGraphicsEffect(effect);
            }
        }

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());
    }

    int getCurrentIndex()
    {
        return comboBox->currentIndex();
    }

    QString getCurrentText()
    {
        return comboBox->currentText();
    }

    int getNumberOfItems()
    {
        pugi::xpath_node_set item_nodes = xmlNode.select_nodes("./items/item");
        return int(item_nodes.size());
    }

    void updateItem(int index)
    {
        Q_UNUSED(index)
        QString text = comboBox->currentText();
        setAttributeValue("value", text);

        QString xpath = "./items/item[@value='" + text + "']";
        pugi::xml_node item_node = xmlNode.select_node(xpath.toStdString().c_str()).node();

        while (boxlayout->count() > 0)
        {
            QWidget *w = boxlayout->takeAt(0)->widget();
            boxlayout->removeWidget(w);
            w->deleteLater();
        }

        if (item_node)
        {
            hide();
            for (pugi::xml_node child_node: item_node.children())
                if (XmlModule::generateXmlObject(module, boxlayout, child_node, rowheight, labelwidth, true))
                    XmlModule::generateXmlObject(module, boxlayout, child_node, rowheight, labelwidth, false);
            show();
        }

        QTimer::singleShot(0, this, [this](){ window()->setStyleSheet(window()->styleSheet()); });
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

    static XmlSelectionBox* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int row_height, int label_width)
    {
        Q_UNUSED(row_height)
        XmlSelectionBox *selectionBox = new XmlSelectionBox(parent, node, label_width);
        connect(parent, &XmlModule::dpiScaleChanged, selectionBox, &XmlSelectionBox::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            selectionBox->updateDpiScale(parent->getDpiScale());

        return selectionBox;
    }
};

#endif
