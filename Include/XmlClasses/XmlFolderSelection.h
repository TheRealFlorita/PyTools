#ifndef XMLFOLDERSELECTION_H
#define XMLFOLDERSELECTION_H

#include <HdWidgets.h>
#include <XmlAbstractObject.h>
#include <XmlModule.h>
#include <Settings.h>
#include <FramelessFileDialog.h>


class XmlFolderSelection : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    HdBoxLayout *layout;
    HdLineEdit *lineEdit;
    HdLabel *label;
    HdPushButton *pushButton;

public:
    explicit XmlFolderSelection(QWidget *parent, pugi::xml_node node) : HdWidget(parent), XmlAbstractObject(node)
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
        connect(this, &XmlFolderSelection::dpiScaleChanged, layout, &HdBoxLayout::updateDpiScale);

        /* Label */
        label = new HdLabel(this);
        setLabelText(getAttributeValue("name", QString()));
        connect(this, &XmlFolderSelection::dpiScaleChanged, label, &HdLabel::updateDpiScale);
        layout->addWidget(label);

        /* Line edit */
        lineEdit = new HdLineEdit(this);
        connect(lineEdit, &HdLineEdit::editingFinished, this, &XmlFolderSelection::updateText);
        connect(this, &XmlFolderSelection::dpiScaleChanged, lineEdit, &HdLineEdit::updateDpiScale);

        /* Dynamic sizes */
        int lineHeigth = settings.getLineEditHeight(this);
        int iconwidth = lineHeigth-8;
        QSize iconsize = QSize(iconwidth,iconwidth);
        QIcon folderIcon;
        folderIcon.addFile(QDir::currentPath() + "/icons/folder-search.svg", iconsize, QIcon::Normal, QIcon::On);
        folderIcon.addFile(":/Icons/empty.svg", iconsize, QIcon::Disabled, QIcon::Off);

        /* Add pushbutton to access open file dialog */
        pushButton = new HdPushButton(this);
        pushButton->setIcon(folderIcon);
        pushButton->setCheckable(false);
        connect(this, &XmlFolderSelection::dpiScaleChanged, pushButton, &HdPushButton::updateDpiScale);
        connect(pushButton, &HdPushButton::clicked, this, &XmlFolderSelection::showFileDialog);

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

        setText(getAttributeValue("value", QString()).trimmed());
    }

    void setDynamicHeight(int h)
    {
        lineEdit->setDynamicHeight(h);
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

    void setText(QString txt)
    {
        txt.replace("\\","/");

        while (txt.contains("//"))
            txt.replace("//","/");

        lineEdit->setText(txt);
        setAttributeValue("value", txt);
    }

    static XmlFolderSelection* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight, int labelwidth)
    {
        XmlFolderSelection *folderSelection = new XmlFolderSelection(parent, node);

        /* Label width attribute */
        if (QString(node.attribute("label-width").value()).toInt() > 0)
            folderSelection->setDynamicLabelWidth(QString(node.attribute("label-width").value()).toInt());
        else if (QString(node.attribute("label-width").value()) != "local")
            folderSelection->setDynamicLabelWidth(labelwidth);

        folderSelection->setDynamicHeight(rowheight);
        connect(parent, &XmlModule::dpiScaleChanged, folderSelection, &XmlFolderSelection::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            folderSelection->updateDpiScale(parent->getDpiScale());

        return folderSelection;
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

        fileDialog->setFileMode(QFileDialog::Directory);
        fileDialog->setOption(QFileDialog::ShowDirsOnly, true);
        fileDialog->setAcceptMode(QFileDialog::AcceptSave);

        QString txt = lineEdit->text().trimmed();

        if (!txt.isEmpty())
            fileDialog->setDirectory(QDir(txt).absolutePath());
        else
            fileDialog->setDirectory(settings.getDialogPath());

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
