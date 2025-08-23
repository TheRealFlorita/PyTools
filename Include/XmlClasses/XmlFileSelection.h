#ifndef XMLFILESELECTION_H
#define XMLFILESELECTION_H

#include <HdWidgets.h>
#include <XmlAbstractObject.h>
#include <XmlModule.h>
#include <Settings.h>
#include <FramelessFileDialog.h>


class XmlFileSelection : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    HdBoxLayout *layout;
    HdLineEdit *lineEdit;
    HdLabel *label;
    HdPushButton *pushButton;
    QString nameFilter, suffix;

public:
    explicit XmlFileSelection(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
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
        connect(this, &XmlFileSelection::dpiScaleChanged, layout, &HdBoxLayout::updateDpiScale);

        /* Label */
        label = new HdLabel(this);
        setLabelText(getAttributeValue("name", QString()));
        connect(this, &XmlFileSelection::dpiScaleChanged, label, &HdLabel::updateDpiScale);
        layout->addWidget(label);

        /* Line edit */
        lineEdit = new HdLineEdit(this);
        connect(lineEdit, &HdLineEdit::editingFinished, this, &XmlFileSelection::updateText);
        connect(this, &XmlFileSelection::dpiScaleChanged, lineEdit, &HdLineEdit::updateDpiScale);

        /* Dynamic icon size */
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
        connect(this, &XmlFileSelection::dpiScaleChanged, pushButton, &HdPushButton::updateDpiScale);
        connect(pushButton, &HdPushButton::clicked, this, &XmlFileSelection::showFileDialog);

        HdWidget *wdg = new HdWidget(this);
        wdg->setObjectName("Background");
        layout->addWidget(wdg);

        HdBoxLayout *lyt = new HdBoxLayout(QBoxLayout::LeftToRight,wdg);
        lyt->setContentsMargins(0,0,0,0);
        lyt->setSpacing(0);
        lyt->addWidget(lineEdit);
        lyt->addWidget(pushButton);

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());

        setText(getAttributeValue("value", QString()));
    }

    void setDynamicHeight(int h)
    {
        lineEdit->setDynamicHeight(h);
        pushButton->setDynamicSize(h,h);
        pushButton->setDynamicIconSize(h-8, h-8);
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

    void setText(QString txt)
    {
        txt.replace("\\","/");

        while (txt.contains("//"))
            txt.replace("//","/");

        lineEdit->setText(txt);
        setAttributeValue("value", txt);
    }

    static XmlFileSelection* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlFileSelection *fileSelection = new XmlFileSelection(parent, node);

        /* Label width attribute */
        if (QString(node.attribute("label-width").value()).toInt() > 0)
            fileSelection->setDynamicLabelWidth(QString(node.attribute("label-width").value()).toInt());
        else if (QString(node.attribute("label-width").value()) != "local")
            fileSelection->setDynamicLabelWidth(labelwidth);

        fileSelection->setDynamicHeight(rowheight);
        connect(parent, &XmlModule::dpiScaleChanged, fileSelection, &XmlFileSelection::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            fileSelection->updateDpiScale(parent->getDpiScale());

        return fileSelection;
    }


public slots:

    void updateText()
    {
        if (lineEdit->text().contains("\\"))
            lineEdit->setText(lineEdit->text().replace("\\","/"));

        while (lineEdit->text().contains("//"))
            lineEdit->setText(lineEdit->text().replace("//","/"));

        setAttributeValue("value", lineEdit->text());
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

        QString filepath = lineEdit->text().trimmed();
        QString directory;

        if (filepath.isEmpty())
        {
            directory = settings.getDialogPath();
            filepath = QFileInfo(directory + "/" + FramelessFileDialog::tr("Untitled")).absoluteFilePath();
        }
        else
            directory = QFileInfo(filepath).absolutePath();

        fileDialog->setDirectory(directory);
        fileDialog->selectFile(filepath);

        /* Execute QFileDialog */
        if(fileDialog->exec())
            if (!fileDialog->selectedFiles().isEmpty())
            {
                setText(fileDialog->selectedFiles().constFirst());
                settings.setDialogPath(fileDialog->selectedFiles().constFirst());
            }

        fileDialog->deleteLater();
        pushButton->update();
    }

};

#endif
