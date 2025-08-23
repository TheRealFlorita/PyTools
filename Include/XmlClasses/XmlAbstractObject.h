#ifndef XMLABSTRACTOBJECT_H
#define XMLABSTRACTOBJECT_H

#include <pugixml.hpp>
#include <string>
#include <cstring>
#include <QString>
#include <QVariant>


struct xml_string_writer: pugi::xml_writer
{
    std::string result;

    virtual void write(const void* data, size_t size) override
    {
        result.append(static_cast<const char*>(data), size);
    }
};


class XmlAbstractObject
{

protected:
    pugi::xml_node xmlNode;

public:
    XmlAbstractObject()
    { }

    XmlAbstractObject(pugi::xml_node node)
    {
        xmlNode = node;
    }

    virtual ~XmlAbstractObject();

    virtual QString getElementType()
    {
        return QString(xmlNode.name());
    }

    virtual QString getElementName()
    {
        return QString(xmlNode.attribute("name").value());
    }

    virtual pugi::xml_attribute getAttribute(QString attribute)
    {
        return xmlNode.attribute(attribute.toStdString().c_str());
    }

    virtual void deleteAttribute(QString attribute)
    {
        xmlNode.remove_attribute(attribute.toStdString().c_str());
        return;
    }

    virtual bool getAttributeValue(QString attribute, bool default_value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();
        bool value = default_value;

        if (xmlNode.attribute(atrr))
        {
            bool ok = false;
            QString text = xmlNode.attribute(atrr).value();
            text = text.replace("true","1", Qt::CaseInsensitive).replace("false","0", Qt::CaseInsensitive);
            value = bool(text.toInt(&ok));

            if (!ok)
                value = default_value;
        }
        return value;
    }

    virtual int getAttributeValue(QString attribute, int default_value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();
        int value = default_value;

        if (xmlNode.attribute(atrr))
        {
            bool ok = false;
            value = QString(xmlNode.attribute(atrr).value()).toInt(&ok);

            if (!ok)
                value = default_value;
        }
        return value;
    }

    virtual double getAttributeValue(QString attribute, double default_value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();
        double value = default_value;

        if (xmlNode.attribute(atrr))
        {
            bool ok = false;
            value = QString(xmlNode.attribute(atrr).value()).toDouble(&ok);

            if (!ok)
                value = default_value;
        }
        return value;
    }

    virtual QString getAttributeValue(QString attribute, QString default_value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();
        QString value = default_value;

        if (xmlNode.attribute(atrr))
            value = QString(xmlNode.attribute(atrr).value());
        return value;
    }

    virtual QVariant getAttributeValue(QString attribute, QVariant default_value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();
        QVariant value= default_value;

        if (xmlNode.attribute(atrr))
            value = QVariant(xmlNode.attribute(atrr).value());
        return value;
    }

    virtual void setAttributeValue(QString attribute, bool value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();

        if (xmlNode.attribute(atrr))
            xmlNode.attribute(atrr).set_value(int(value));
        else
        {
            xmlNode.append_attribute(atrr);
            xmlNode.attribute(atrr).set_value(int(value));
        }
        return;
    }

    virtual void setAttributeValue(QString attribute, int value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();

        if (xmlNode.attribute(atrr))
            xmlNode.attribute(atrr).set_value(value);
        else
            xmlNode.append_attribute(atrr).set_value(value);
        return;
    }

    virtual void setAttributeValue(QString attribute, double value, int precision = -1)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();
        QString val;

        if (precision >= 0)
            val = QString::number(value, 'f', precision);
        else
            val = QVariant(value).toString();

        if (xmlNode.attribute(atrr))
            xmlNode.attribute(atrr).set_value(val.toStdString().c_str());
        else
            xmlNode.append_attribute(atrr).set_value(val.toStdString().c_str());
        return;
    }

    virtual void setAttributeValue(QString attribute, QString value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();

        if (xmlNode.attribute(atrr))
            xmlNode.attribute(atrr).set_value(value.toStdString().c_str());
        else
            xmlNode.append_attribute(atrr).set_value(value.toStdString().c_str());
        return;
    }

    virtual void setAttributeValue(QString attribute, QVariant value)
    {
        std::string str = attribute.toStdString();
        const char *atrr = str.c_str();

        if (xmlNode.attribute(atrr))
            xmlNode.attribute(atrr).set_value(value.toString().toStdString().c_str());
        else
            xmlNode.append_attribute(atrr).set_value(value.toString().toStdString().c_str());
        return;
    }

    virtual pugi::xml_node node()
    {
        return xmlNode;
    }

    virtual QString printXmlNode()
    {
        xml_string_writer writer;
        xmlNode.print(writer);

        return QString(writer.result.c_str());
    }

};

#endif
