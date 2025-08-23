#ifndef XMLMULTIFILESELECTION_H
#define XMLMULTIFILESELECTION_H

#include <HdWidgets.h>
#include <XmlAbstractObject.h>
#include <XmlModule.h>
#include <Settings.h>
#include <FramelessFileDialog.h>


class XmlMultiFileSelection : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    HdBoxLayout *layout;
    HdComboBox *comboBox;
    HdLabel *label;
    HdPushButton *pushButton;
    QFileDialog::AcceptMode acceptMode = QFileDialog::AcceptOpen;
    QString nameFilter, suffix;

public:
    explicit XmlMultiFileSelection(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
    {
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);

        if (!xmlNode.child("items"))
            xmlNode.append_child("items");

        /* Create layout */
        if (getAttributeValue("alignment", QString()) == "vertical")
            layout = new HdBoxLayout(QBoxLayout::TopToBottom, this);
        else
            layout = new HdBoxLayout(QBoxLayout::LeftToRight, this);

        layout->setContentsMargins(0,0,0,0);
        layout->setDynamicSpacing(3);
        connect(this, &XmlMultiFileSelection::dpiScaleChanged, layout, &HdBoxLayout::updateDpiScale);

        /* Label */
        label = new HdLabel(this);
        setLabelText(getAttributeValue("name", QString()));
        connect(this, &XmlMultiFileSelection::dpiScaleChanged, label, &HdLabel::updateDpiScale);
        layout->addWidget(label);

        /* Read items */
        QStringList files;
        pugi::xpath_node_set item_nodes = xmlNode.select_nodes("./items/item");
        for (size_t i = 0; i < item_nodes.size(); ++i)
        {
            pugi::xml_node item_node = item_nodes[i].node();

            QString text = item_node.attribute("value").value();
            if (!text.trimmed().isEmpty() && !files.contains(text))
                files.append(text);
            else
                item_node.parent().remove_child(item_node);

            item_node.remove_attribute("index");
        }

        /* Combo box */
        comboBox = new HdComboBox(this);
        comboBox->addItems(files);
        comboBox->setCurrentText(getAttributeValue("value", QString()));
        connect(comboBox, QOverload<int>::of(&HdComboBox::currentIndexChanged), this, &XmlMultiFileSelection::updateIndex);
        connect(this, &XmlMultiFileSelection::dpiScaleChanged, comboBox, &HdComboBox::updateDpiScale);

        /* Dynamic sizes */
        int lineHeigth = settings.getLineEditHeight(this);
        int iconwidth = lineHeigth-8;
        QSize iconsize = QSize(iconwidth,iconwidth);
        QIcon fileIcon;
        fileIcon.addFile(QDir::currentPath() + "/icons/file-search.svg", iconsize, QIcon::Normal, QIcon::On);
        fileIcon.addFile(":/Icons/empty.svg", iconsize, QIcon::Disabled, QIcon::Off);

        /* Add pushbutton to access open file dialog */
        pushButton = new HdPushButton(this);
        pushButton->setIcon(fileIcon);
        pushButton->setCheckable(false);
        connect(this, &XmlMultiFileSelection::dpiScaleChanged, pushButton, &HdPushButton::updateDpiScale);
        connect(pushButton, &HdPushButton::clicked, this, &XmlMultiFileSelection::showFileDialog);

        HdWidget *wdg = new HdWidget(this);
        wdg->setObjectName("Background");
        layout->addWidget(wdg);

        HdBoxLayout *lyt = new HdBoxLayout(QBoxLayout::LeftToRight,wdg);
        lyt->setContentsMargins(0,0,0,0);
        lyt->setSpacing(0);
        lyt->addWidget(comboBox);
        lyt->addWidget(pushButton);

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());
    }

    void setDynamicHeight(int h)
    {
        comboBox->setDynamicHeight(h);
        pushButton->setDynamicSize(h,h);
        pushButton->setDynamicIconSize(h-8,h-8);
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

    static XmlMultiFileSelection* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlMultiFileSelection *fileSelection = new XmlMultiFileSelection(parent, node);

        /* Label width attribute */
        if (QString(node.attribute("label-width").value()).toInt() > 0)
            fileSelection->setDynamicLabelWidth(QString(node.attribute("label-width").value()).toInt());
        else if (QString(node.attribute("label-width").value()) != "local")
            fileSelection->setDynamicLabelWidth(labelwidth);

        fileSelection->setDynamicHeight(rowheight);
        connect(parent, &XmlModule::dpiScaleChanged, fileSelection, &XmlMultiFileSelection::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            fileSelection->updateDpiScale(parent->getDpiScale());

        return fileSelection;
    }

public slots:

    void updateIndex(int index)
    {
        Q_UNUSED(index)
        setAttributeValue("value", comboBox->currentText());
    }

    void showFileDialog()
    {
        FramelessFileDialog *fileDialog = new FramelessFileDialog();

        if (getAttribute("window-title"))
            fileDialog->setWindowTitle(getAttributeValue("window-title", QString()).trimmed());
        else
            fileDialog->setWindowTitle(FramelessFileDialog::tr("File Dialog"));

        if (getAttributeValue("value", QString("save")).toLower() == "save")
            fileDialog->setAcceptMode(QFileDialog::AcceptSave);
        else
            fileDialog->setAcceptMode(QFileDialog::AcceptOpen);

        QString namefilter = getAttributeValue("filter", QString());
        if (!namefilter.isEmpty())
            fileDialog->setNameFilter(namefilter);

        QString defaultsuffix = getAttributeValue("suffix", QString());
        if (!defaultsuffix.isEmpty())
            fileDialog->setDefaultSuffix(defaultsuffix);

        /* Read items */
        QStringList files;
        QString directory = settings.getDialogPath();
        pugi::xpath_node_set item_nodes = xmlNode.select_nodes("./items/item");
        for (size_t i = 0; i < item_nodes.size(); ++i)
        {
            pugi::xml_node item_node = item_nodes[i].node();

            if (i == 0)
                directory = QFileInfo(item_node.attribute("value").value()).absolutePath();

            QString text = QFileInfo(item_node.attribute("value").value()).fileName();
            if (!text.trimmed().isEmpty() && !files.contains(text))
                files.append(text);
            else
                item_node.parent().remove_child(item_node);

            item_node.remove_attribute("index");
        }

        fileDialog->setDirectory(directory);

        if (files.length() > 0)
        {
            QString filepath = "\"" + files.join("\" \"") + "\"";
            fileDialog->selectFile(filepath);
        }
        else
            fileDialog->selectFile("");

        fileDialog->setFileMode(QFileDialog::ExistingFiles);

        /* Execute QFileDialog */
        if(fileDialog->exec())
        {
            comboBox->clear();
            xmlNode.child("items").remove_children();

            files = fileDialog->selectedFiles();
            if (!files.isEmpty())
            {
                settings.setDialogPath(files.constFirst());
                comboBox->addItems(files);

                foreach (QString file, files)
                    xmlNode.child("items").append_child("item").append_attribute("value").set_value(file.toStdString().c_str());
            }
        }

        fileDialog->deleteLater();
        pushButton->update();
    }

};

#endif
