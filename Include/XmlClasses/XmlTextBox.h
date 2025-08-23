#ifndef XMLTEXTBOX_H
#define XMLTEXTBOX_H

#include <QTimer>
#include <QFileInfo>

#include <XmlAbstractObject.h>
#include <XmlModule.h>


class XmlTextBox : public HdTextBrowser, public XmlAbstractObject
{
    Q_OBJECT

public:
    explicit XmlTextBox(QWidget *parent, pugi::xml_node node) : HdTextBrowser(parent), XmlAbstractObject(node)
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        if (getAttribute("file-path"))
            setHtmlSource(getAttributeValue("file-path", QString()));
        else
            setText(xmlNode.text().get());

        document()->setDocumentMargin(0.0);

        /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());

        connect(this, &XmlTextBox::textChanged, this, &XmlTextBox::updateText);
    }

    void setHtmlSource(QString path)
    {
        if (!QFileInfo(path).isFile())
            return;

        clear();
        setReadOnly(true);
        setOpenLinks(true);
        setOpenExternalLinks(true);
        deleteAttribute("read-only");
        setSource(QUrl::fromLocalFile(path));
    }

    void setText(QString text)
    {
        clear();
        setReadOnly(getAttributeValue("read-only", true));
        setOpenLinks(false);
        setOpenExternalLinks(false);
        setPlainText(text.trimmed());
        updateText();
    }

    void updateText()
    {
        xmlNode.text().set(toPlainText().trimmed().toStdString().c_str());
    }

    static XmlTextBox* createFromXmlNode(XmlModule *parent, pugi::xml_node node)
    {
        XmlTextBox *textBox = new XmlTextBox(parent, node);
        textBox->setUseSizeHintHeight(true);
        connect(parent, &XmlModule::dpiScaleChanged, textBox, &XmlTextBox::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            textBox->updateDpiScale(parent->getDpiScale());

        return textBox;
    }

};

#endif
