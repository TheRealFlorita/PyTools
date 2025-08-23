#include <XmlModule.h>

#include <QScrollArea>
#include <QScrollBar>

#include <XmlCheckBox.h>
#include <XmlComboBox.h>
#include <XmlDoubleSpinBox.h>
#include <XmlExpandableBox.h>
#include <XmlFileSelection.h>
#include <XmlFolderSelection.h>
#include <XmlGroupBox.h>
#include <XmlLayout.h>
#include <XmlLineEdit.h>
#include <XmlMultiFileSelection.h>
#include <XmlSelectionBox.h>
#include <XmlSpinBox.h>
#include <XmlTableWidget.h>
#include <XmlTextBox.h>
#include <PyTools.h>
#include <DocumentationViewer.h>
#include <Settings.h>


XmlModule::XmlModule(pugi::xml_node node, QWidget *parent) : HdWidget(parent), XmlAbstractObject(node)
{
    /* Get module type */
    if (getElementType() == "expandable-module")
        moduleExpandable = true;
    else
        moduleExpandable = false;

    if (!getAttribute("name"))
        xmlNode.prepend_attribute("name");

    /* Create layout */
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    /* Create tab bar */
    tabBar = new FramelessTabBar(this);
    tabBar->setDynamicHeight(settings.getTabBarHeight(this));
    tabBar->setTabsClosable(false);
    tabBar->setTabsEditable(moduleExpandable);
    connect(this, &XmlModule::dpiScaleChanged, tabBar, &FramelessTabBar::updateDpiScale);
    layout->addWidget(tabBar,0,0,1,1);

    /* Enable tab movement for expandable modules */
    if (moduleExpandable)
        tabBar->setMovable(true);

    /* Create stacked widget */
    stackedWidget = new HdStackedWidget(this);
    connect(tabBar, &FramelessTabBar::currentChanged, stackedWidget, &HdStackedWidget::setCurrentIndex);
    layout->addWidget(stackedWidget,1,0,-1,-1);

    /* Add shadow to tab bar */
    HdShadowEffect *effect = new HdShadowEffect(tabBar, qreal(1.0), qreal(12.0), settings.getColor("drop-shadow/color"));
    effect->setDistance(1);
    connect(this, &XmlModule::dpiScaleChanged, effect, &HdShadowEffect::updateDpiScale);
    tabBar->setGraphicsEffect(effect);
    tabBar->raise();

    /* Connect signals */
    if (moduleExpandable)
    {
        connect(tabBar, &FramelessTabBar::tabMoved, this, &XmlModule::moveTab);
        connect(tabBar, &FramelessTabBar::tabRenamed, this, &XmlModule::renameTab);
        connect(tabBar, &FramelessTabBar::tabCreated, this, &XmlModule::newTab);
        connect(tabBar, &FramelessTabBar::tabToggled, this, &XmlModule::toggleTab);
        connect(tabBar, &FramelessTabBar::tabCopied, this, &XmlModule::copyTab);
        connect(tabBar, &FramelessTabBar::tabClosed, this, &XmlModule::closeTab);
        connect(tabBar, &FramelessTabBar::tabClosed, stackedWidget, qOverload<int>(&HdStackedWidget::removeWidget));
    }

    connect(tabBar, &FramelessTabBar::currentChanged, this, &XmlModule::changeCurrentTab);
    connect(window()->windowHandle(), &QWindow::screenChanged, this, &XmlModule::initialize);
}

void XmlModule::changeCurrentTab(int index)
{
    /* Set selected tab */
    if (tabBar->count() > 1)
        setAttributeValue("selected-tab", index);
    else
        deleteAttribute("selected-tab");
}

QString XmlModule::toString()
{
    /* Create document */
    pugi::xml_document doc;

    /* Add declaration node */
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    decl.append_attribute("standalone") = "yes";

    /* Copy top level node */
    doc.append_copy(xmlNode.root().child("application"));
    pugi::xpath_node_set module_nodes = doc.child("application").select_nodes("./modules/*[self::module or self::expandable-module]");
    for (size_t i = 0; i < module_nodes.size(); ++i)
    {
        pugi::xml_node module = module_nodes[i].node();
        if (QString(module.attribute("name").value()) != QString(xmlNode.attribute("name").value()) || QString(module.name()) != QString(xmlNode.name()))
            module.parent().remove_child(module);
    }

    /* Writer */
    xml_string_writer writer;
    doc.save(writer);

    return QString(writer.result.c_str());
}

QString XmlModule::getName()
{
    return getAttributeValue("name", QString());
}

QString XmlModule::getFilePath()
{
    return QFileInfo(getAttributeValue("file-path", QString())).absoluteFilePath();
}

bool XmlModule::hasDocumentation()
{
    return (!QString(xmlNode.child("documentation").attribute("file-path").value()).isEmpty());
}

QString XmlModule::getDocumentationFilePath()
{
    return xmlNode.child("documentation").attribute("file-path").value();
}

int XmlModule::getObjectLabelWidth(pugi::xml_node node)
{
    QList<QString> labels;
    pugi::xpath_node_set all_nodes = node.select_nodes(".//*");
    const QStringList itags = {"combo-box", "file-selection", "folder-selection", "line-edit", "multi-file-selection", "spin-box", "double-spin-box"};
    const QStringList gtags = {"group-box", "expandable-box", "grid-layout", "horizontal-layout", "selection-box", "vertical-layout"};

    for (size_t i = 0; i < all_nodes.size(); ++i)
    {
        pugi::xml_node inode = all_nodes[i].node();
        if (!itags.contains(inode.name()) || QString(node.attribute("alignment").value()) == "vertical" || inode.attribute("label-width"))
            continue;

        QString label = inode.attribute("name").value();
        pugi::xml_node pnode = inode.parent();

        while (pnode && pnode != node)
        {
            if (gtags.contains(pnode.name()) && QString(pnode.attribute("label-width").value()) == "local")
            {
                label.clear();
                break;
            }
            pnode = pnode.parent();
        }

        if (!label.isEmpty())
            labels << label;
    }
    int width = std::max(60, settings.getLabelWidth(labels, this) + 16);
    return width;
}

int XmlModule::getRowHeight()
{
    return rowheight;
}

void XmlModule::reset()
{
    /* Delete selected tab attribute */
    deleteAttribute("selected-tab");

    /* Delete tabs of expandable module */
    if (moduleExpandable)
        xmlNode.remove_child(xmlNode.child("tabs"));
    else
    {
        /* Enable all group boxes and expandable boxes */
        pugi::xpath_node_set box_nodes = xmlNode.select_nodes(".//*[self::group-box or self::expandable-box or self::selection-box]");
        for (size_t i = 0; i < box_nodes.size(); ++i)
            if (box_nodes[i].node().attribute("check-state"))
                if (QString(box_nodes[i].node().attribute("check-state").value()) != "-1")
                    box_nodes[i].node().attribute("check-state").set_value("1");

        /* Reset all expandable boxes */
        box_nodes = xmlNode.select_nodes(".//expandable-box");
        for (size_t i = 0; i < box_nodes.size(); ++i)
        {
            box_nodes[i].node().attribute("value").set_value("0");
            box_nodes[i].node().remove_child("rows");
        }

        /* Clear values */
        pugi::xpath_node_set all_nodes = xmlNode.select_nodes(".//*");
        for (size_t i = 0; i < all_nodes.size(); ++i)
            if (QString(all_nodes[i].node().name()) != "item" && QString(all_nodes[i].node().parent().name()) != "items")
                all_nodes[i].node().attribute("value").set_value("");

        /* Delete table items */
        pugi::xpath_node_set row_nodes = xmlNode.select_nodes("./tabs/table/rows/row");
        for (size_t i = 0; i < row_nodes.size(); ++i)
        {
            pugi::xml_node row_node = row_nodes[i].node();

            for (pugi::xml_node item_node: row_node.children("items"))
            {
                if (item_node.attribute("read-only"))
                     if (QVariant(item_node.attribute("read-only").value()).toBool())
                         continue;
                row_node.remove_child(item_node);
            }

            /* Delete if row is empty and has no header */
            if (int(row_node.select_nodes("items").size()) == 0 && QString(row_node.attribute("header").value()).trimmed() == "")
                row_node.parent().remove_child(row_node);
        }
    }

    initialize();
}

void XmlModule::initialize()
{
    /* Get selected tab */
    int selected = getAttributeValue("selected-tab", 0);

    /* Clear ui */
    while (tabBar->count() > 0)
        tabBar->QTabBar::removeTab(0);
    while (stackedWidget->count() > 0)
        stackedWidget->HdStackedWidget::removeWidget(0);

    /* Get sizes */
    tabBar->setDynamicHeight(settings.getTabBarHeight(this));
    rowheight = settings.getLineEditHeight(this);

    /* Find tabs */
    pugi::xpath_node_set tab_nodes = xmlNode.select_nodes("./tabs/*[self::tab or self::table]");
    int ntabs = int(tab_nodes.size());

    /* Create tabs */
    if (!moduleExpandable && ntabs == 0)
    {
        int index = tabBar->QTabBar::addTab(xmlNode.attribute("name").value());
        generateTab(index, xmlNode);
    }
    else if (moduleExpandable && ntabs == 0)
    {
        if (!xmlNode.child("tabs"))
            xmlNode.append_child("tabs");

        pugi::xml_node tab_node = xmlNode.child("tabs").append_copy(xmlNode.child("items"));
        tab_node.set_name("tab");
        tab_node.append_attribute("enabled").set_value(1);

        int index = tabBar->QTabBar::addTab("_tab_");
        generateTab(index, tab_node);
    }
    else
    {
        for (size_t i = 0; i < size_t(ntabs); ++i)
        {
            pugi::xml_node tab_node = tab_nodes[i].node();

            if (!tab_node.attribute("enabled") && moduleExpandable && QString(tab_node.name()) != "table")
                tab_node.append_attribute("enabled").set_value("1");

            int index = tabBar->QTabBar::addTab("_tab_");
            generateTab(index, tab_node);
        }
    }

    tabBar->setCurrentIndex(selected);
}

void XmlModule::moveTab(int from, int to)
{
    QWidget *widget = stackedWidget->widget(from);
    stackedWidget->insertWidget(to, widget);

    pugi::xpath_node_set tab_nodes = xmlNode.select_nodes("./tabs/*[self::tab or self::table]");
    pugi::xml_node from_node = tab_nodes[size_t(from)].node();
    pugi::xml_node to_node = tab_nodes[size_t(to)].node();

    if (from > to)
        xmlNode.child("tabs").insert_move_before(from_node, to_node);
    else
        xmlNode.child("tabs").insert_move_after(from_node, to_node);
}

void XmlModule::renameTab(int index, QString name)
{
    if (moduleExpandable)
    {
        pugi::xpath_node_set tab_nodes = xmlNode.select_nodes("./tabs/*[self::tab or self::table]");
        for (size_t i = 0; i < tab_nodes.size(); ++i)
            if (i == size_t(index))
                tab_nodes[i].node().attribute("name").set_value(name.toStdString().c_str());
    }
}

void XmlModule::toggleTab(int index, bool enable)
{
    if (moduleExpandable)
    {
        pugi::xpath_node_set tab_nodes = xmlNode.select_nodes("./tabs/*[self::tab or self::table]");
        tab_nodes[size_t(index)].node().attribute("enabled").set_value(int(enable));
    }
}

void XmlModule::newTab(int index)
{
    if (moduleExpandable && xmlNode.child("items"))
    {
        pugi::xml_node tab_node = xmlNode.child("tabs").append_copy(xmlNode.child("items"));
        tab_node.set_name("tab");
        tab_node.append_attribute("name").set_value(tabBar->tabText(index).toStdString().c_str());
        generateTab(index, tab_node);
    }
}

void XmlModule::copyTab(int original, int index)
{
    if (moduleExpandable)
    {
        pugi::xpath_node_set tab_nodes = xmlNode.select_nodes("./tabs/*[self::tab or self::table]");
        pugi::xml_node tab_orig = tab_nodes[size_t(original)].node();
        pugi::xml_node tab_copy = tab_orig.parent().append_copy(tab_orig);
        tab_copy.attribute("name").set_value(tabBar->tabText(index).toStdString().c_str());
        generateTab(index, tab_copy);
        tabBar->setCurrentIndex(index);
    }
}

void XmlModule::closeTab(int index)
{
    if (moduleExpandable)
    {
        pugi::xpath_node_set tab_nodes = xmlNode.select_nodes("./tabs/*[self::tab or self::table]");
        pugi::xml_node tab_node = tab_nodes[size_t(index)].node();
        tab_node.parent().remove_child(tab_node);
    }
}

void XmlModule::generateTab(int index, pugi::xml_node tab_node)
{
    /* Set tab enabled */
    if (moduleExpandable)
    {
        if (!tab_node.attribute("enabled"))
            tab_node.append_attribute("enabled").set_value("1");
        else if (!QString(tab_node.attribute("enabled").value()).toLower().replace("true","1").replace("false","0").toInt())
            tabBar->setTabEnabled(index, false);
    }
    else
        tab_node.remove_attribute("enabled");

    /* Table element */
    if (QString(tab_node.name()) == "table")
    {
        XmlTableWidget *tableWidget = XmlTableWidget::createFromXmlNode(this, tab_node, rowheight);
        tableWidget->setUseViewportSizeHintHeight(false);
        tabBar->setTabText(index, tableWidget->getTitle());
        stackedWidget->insertWidget(index, tableWidget);
    }
    /* Tab element */
    else
    {
        /* Set tab name */
        if (!tab_node.attribute("name"))
            tab_node.prepend_attribute("name").set_value(QString(XmlModule::tr("Tab %1")).arg(index + 1).toStdString().c_str());
        tabBar->setTabText(index, tab_node.attribute("name").value());

        /* Scroll area */
        QScrollArea *scrollArea = new QScrollArea(this);
        scrollArea->setAttribute(Qt::WA_Hover);
        scrollArea->setWidgetResizable(true);
        scrollArea->verticalScrollBar()->setSliderPosition(Qt::ScrollBegin);
        scrollArea->horizontalScrollBar()->setSliderPosition(Qt::ScrollBegin);
        stackedWidget->insertWidget(index, scrollArea);

        /* Main widget for ui elements */
        HdWidget *mainWidget = new HdWidget(scrollArea);
        scrollArea->setWidget(mainWidget);
        connect(this, &XmlModule::dpiScaleChanged, mainWidget, &HdWidget::updateDpiScale);

        /* Layout for main widget */
        HdBoxLayout *boxLayout = new HdBoxLayout(QBoxLayout::TopToBottom, mainWidget);
        boxLayout->setSpacing(0);
        boxLayout->setDynamicMargins(12,12,12,12);
        connect(this, &XmlModule::dpiScaleChanged, boxLayout, &HdBoxLayout::updateDpiScale);

        /* Add child ui elements */
        for (pugi::xml_node node: tab_node.children())
            generateXmlObject(this, boxLayout, node, rowheight, getObjectLabelWidth(tab_node), true);

        for (pugi::xml_node node: tab_node.children())
            generateXmlObject(this, boxLayout, node, rowheight, getObjectLabelWidth(tab_node), false);

        /* Add spacer at bottom */
        QWidget *spacer = new QWidget(mainWidget);
        spacer->setMinimumHeight(0);
        boxLayout->addWidget(spacer,1);
    }

    return;
}

bool XmlModule::generateXmlObject(XmlModule *module, HdBoxLayout *boxlayout, pugi::xml_node node, int rowheight, int labelwidth, bool test)
{
    const QStringList node_types = \
    {"group-box", "table", "check-box", "combo-box", "double-spin-box", "expandable-box", "file-selection",
     "folder-selection", "grid-layout", "horizontal-layout", "vertical-layout", "line-edit", "multi-file-selection",
     "selection-box", "spacer", "spin-box", "text-box"};

    QString node_type = node.name();

    /* Create object */
    if (test && node_types.contains(node_type))
        return true;
    else if (test)
    {
        FramelessMessageBox msg(QMessageBox::Question, settings.getApplicationName(),
                                QString("Unknown XML node [%1].\nDelete element and proceed?").arg(node_type),
                                QMessageBox::Yes | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Yes);

        int reply = msg.exec();

        if (reply == QMessageBox::Cancel)
            return false;
        else
        {
            node.parent().remove_child(node);
            return true;
        }
    }
    else if (node_type == "group-box")
        boxlayout->addWidget(XmlGroupBox::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "table")
        boxlayout->addWidget(XmlTableWidget::createFromXmlNode(module, node, rowheight));
    else if (node_type == "check-box")
        boxlayout->addWidget(XmlCheckBox::createFromXmlNode(module, node, rowheight));
    else if (node_type == "combo-box")
        boxlayout->addWidget(XmlComboBox::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "double-spin-box")
        boxlayout->addWidget(XmlDoubleSpinBox::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "expandable-box")
        boxlayout->addWidget(XmlExpandableBox::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "file-selection")
        boxlayout->addWidget(XmlFileSelection::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "folder-selection")
        boxlayout->addWidget(XmlFolderSelection::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "grid-layout")
        boxlayout->addWidget(XmlGridLayout::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "horizontal-layout" || node_type == "vertical-layout")
        boxlayout->addWidget(XmlBoxLayout::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "line-edit")
        boxlayout->addWidget(XmlLineEdit::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "multi-file-selection")
        boxlayout->addWidget(XmlMultiFileSelection::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "selection-box")
        boxlayout->addWidget(XmlSelectionBox::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "spacer")
        boxlayout->addWidget(XmlSpacer::createFromXmlNode(module, node),1);
    else if (node_type == "spin-box")
        boxlayout->addWidget(XmlSpinBox::createFromXmlNode(module, node, rowheight, labelwidth));
    else if (node_type == "text-box")
        boxlayout->addWidget(XmlTextBox::createFromXmlNode(module, node));

    return true;
}
