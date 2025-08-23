#ifndef XMLAPPLICATION_H
#define XMLAPPLICATION_H

#include <QFile>
#include <QMap>
#include <QMessageBox>
#include <QStringDecoder>
#include <HdWidgets.h>
#include <Settings.h>
#include <XmlModule.h>
#include <FramelessMessageBox.h>


class XmlApplication : public HdWidget
{
    Q_OBJECT

private:
    pugi::xml_document document;
    XmlModule *module = Q_NULLPTR;
    QBoxLayout *layout;

public:
    explicit XmlApplication(QWidget *parent) : HdWidget(parent)
    {
        layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        layout->setContentsMargins(0,0,0,0);
        layout->setSpacing(0);
    }

    ~XmlApplication() override
    {
        saveCurrentModule(settings.getAppDataPath() + "/LastSession.xml");
    }

    bool isValid()
    {
        return !(module == Q_NULLPTR);
    }

    bool loadXmlFile(QString filepath)
    {
        QFile file(filepath);
        QString data;

        if (file.open(QIODevice::ReadOnly))
        {
            data = file.readAll();
            file.close();

            /* Get %THISDIR% with correct case */
            QFileInfo thisfile(filepath);
            QString thisdir = thisfile.absolutePath();
            if (data.contains("%THISDIR%", Qt::CaseInsensitive))
            {
                QDirIterator it(thisdir, QStringList() << thisfile.fileName(), QDir::Files);
                while (it.hasNext())
                {
                    QString xmlfile = it.next();
                    if (thisfile.absoluteFilePath().toLower() == xmlfile.toLower())
                        thisdir = QFileInfo(xmlfile).absolutePath();
                }
            }

            /* Replace %<PATH>% with absolute path */
            data.replace("%APPROOT%", settings.getApplicationPath(), Qt::CaseInsensitive);
            data.replace("%USERROOT%", settings.getAppDataPath(), Qt::CaseInsensitive);
            data.replace("%THISDIR%", thisdir, Qt::CaseInsensitive);
            data.replace("%DESKTOP%", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), Qt::CaseInsensitive);
            data.replace("%DOCUMENTS%", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), Qt::CaseInsensitive);
            data.replace("%HOME%", QStandardPaths::writableLocation(QStandardPaths::HomeLocation), Qt::CaseInsensitive);

            QString applicationName = settings.getApplicationName();

            /* Load document (must keep it alive for nodes to remain valid) */
            pugi::xml_document test;
            pugi::xml_parse_result result = test.load_string(data.toStdString().c_str());

            /* Check parsing status */
            if (!result)
            {
                QString txt = "Error description:  " + QString(result.description()) + "\n\n"
                              "Error offset:  " + QString::number(result.offset) + "\n\n"
                              "Error at:  ..." + QString(data.toStdString().c_str() + result.offset).trimmed();

                FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(), "XML parsed with errors.", QMessageBox::Ok);
                msg.setDpiScale(getDpiScale());
                msg.setDetailedText(txt);
                msg.exec();
                return false;
            }

            /* Check if application node exists */
            pugi::xml_node application_node = test.child("application");
            if (!application_node)
            {
                FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(), "No application node found in XML.", QMessageBox::Ok);
                msg.setDpiScale(getDpiScale());
                msg.exec();
                return false;
            }

            pugi::xpath_node_set module_nodes = application_node.select_nodes("./modules/*[self::module or self::expandable-module]");
            if (module_nodes.size() < 1)
            {
                FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(), "No module nodes found in XML.", QMessageBox::Ok);
                msg.setDpiScale(getDpiScale());
                msg.exec();
                return false;
            }

            /* Load new xml document */
            document.load_string(data.toStdString().c_str());
            application_node = document.child("application");

            /* Set application name */
            if (!application_node.attribute("name"))
                application_node.prepend_attribute("name").set_value(applicationName.toStdString().c_str());
            else
                application_node.attribute("name").set_value(applicationName.toStdString().c_str());

            /* Fix compatibility */
            pugi::xpath_node_set all_nodes = document.select_nodes(".//*");
            for (size_t i = 0; i < all_nodes.size(); ++i)
            {
                pugi::xml_node node = all_nodes[i].node();
                QString node_type = node.name();

                if (!node_type.isLower())
                {
                    node_type = node_type.toLower();
                    node.set_name(node_type.toStdString().c_str());
                }

                if (node_type == "checkable-group-box")
                {
                    node.set_name("group-box");
                    pugi::xml_attribute attr = node.attribute("enabled");

                    if (attr)
                        node.insert_attribute_before("check-state", attr).set_value(attr.value());
                    else
                        node.append_attribute("check-state").set_value(1);

                    node.remove_attribute("enabled");
                }
                else if (node_type == "checkable-expandable-box")
                {
                    node.set_name("expandable-box");
                    pugi::xml_attribute attr = node.attribute("enabled");

                    if (attr)
                        node.insert_attribute_before("check-state", attr).set_value(attr.value());
                    else
                        node.append_attribute("check-state").set_value(1);

                    node.remove_attribute("enabled");
                }
                else if (node_type == "horizontal")
                    node.set_name("horizontal-layout");
                else if (node_type == "vertical")
                    node.set_name("vertical-layout");
                else if (node_type == "table-box")
                    node.set_name("table");
            }
        }
        else
        {
            FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(), "Cannot read XML file", QMessageBox::Ok);
            msg.setInformativeText(QFileInfo(filepath).absoluteFilePath());
            msg.setDpiScale(getDpiScale());
            msg.exec();
            return false;
        }
        return true;
    }

    void saveCurrentModule(QString path)
    {
        QString data = currentModule()->toString();

        /* Replace absolute path with %<PATH>% */
        data.replace(settings.getApplicationPath(), "%APPROOT%", Qt::CaseInsensitive);
        data.replace(settings.getAppDataPath(), "%USERROOT%", Qt::CaseInsensitive);
        data.replace(QFileInfo(path).absolutePath(), "%THISDIR%", Qt::CaseInsensitive);
        data.replace(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), "%DESKTOP%", Qt::CaseInsensitive);
        data.replace(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "%DOCUMENTS%", Qt::CaseInsensitive);
        data.replace(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), "%HOME%", Qt::CaseInsensitive);      

        QFile file(path);
        if (file.open(QIODevice::ReadWrite))
        {
            QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            stream.setCodec("UTF-8");
#else
            stream.setEncoding(QStringDecoder::Utf8);
#endif
            stream << data;
            file.resize(file.pos());
            file.close();
        }
        else
        {
            FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(), "Cannot write XML file", QMessageBox::Ok);
            msg.setInformativeText(QFileInfo(path).absoluteFilePath());
            msg.setDpiScale(getDpiScale());
            msg.exec();
        }

    }

    QString getLanguageCode()
    {
        return QString(document.child("application").attribute("language").value()).trimmed();
    }

    pugi::xml_node getModuleNode(QString name)
    {
        pugi::xml_node module_node(nullptr);
        QMap<QString, pugi::xml_node> modules = getModuleNodes();

        if (modules.contains(name))
            module_node = modules[name];
        else if (modules.size() > 0)
            module_node = modules.first();

        /* Check if module was found */
        if (!module_node)
        {
            FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(), QString("Module [%1] not found in XML.").arg(name), QMessageBox::Ok);
            msg.setDpiScale(getDpiScale());
            msg.exec();
        }

        return module_node;
    }

    QMap<QString, pugi::xml_node> getModuleNodes()
    {
        QMap<QString, pugi::xml_node> modules;
        pugi::xpath_node_set module_nodes = document.select_nodes("./application/modules/*[self::module or self::expandable-module]");
        for (size_t i = 0; i < module_nodes.size(); ++i)
            modules.insert(module_nodes[i].node().attribute("name").value(), module_nodes[i].node());

        return modules;
    }

    void selectModule(QString name = "")
    {
        if (module != Q_NULLPTR)
        {
            module->disconnect();
            module->close();
            module->deleteLater();
        }

        module = new XmlModule(getModuleNode(name), this);
        layout->addWidget(module);
        connect(this, &XmlApplication::dpiScaleChanged, module, &XmlModule::updateDpiScale);

        if (currentModule()->getName().trimmed().isEmpty())
            setWindowTitle(settings.getApplicationName());
        else
            setWindowTitle(settings.getApplicationName() + "  -  " + module->getName());

        module->initialize();

        if (getDpiScale() != 1.0)
            module->updateDpiScale(getDpiScale());
    }

    XmlModule* currentModule()
    {
        if (module == Q_NULLPTR)
            selectModule();
        return module;
    }

    QMap<QString, pugi::xml_node> getActionNodes()
    {
        QMap<QString, pugi::xml_node> actions;
        pugi::xpath_node_set action_nodes = document.select_nodes("./application/actions/action");
        for (size_t i = 0; i < action_nodes.size(); ++i)
            actions.insert(action_nodes[i].node().attribute("name").value(), action_nodes[i].node());

        action_nodes = currentModule()->node().select_nodes("./actions/action");
        for (size_t i = 0; i < action_nodes.size(); ++i)
            actions.insert(action_nodes[i].node().attribute("name").value(), action_nodes[i].node());
        return actions;
    }

    QMap<QString, QString> getActions()
    {
        QMap<QString, pugi::xml_node> action_nodes = getActionNodes();
        QMap<QString, QString> actions;

        QList<QString> keys = actions.keys();
        foreach (QString key, keys)
            actions.insert(key, action_nodes[key].attribute("file-path").value());
        return actions;
    }

    QString getPythonPath()
    {
        pugi::xml_node application_node = document.child("application");
        QList<pugi::xml_node> python_nodes = {currentModule()->node().child("python"), application_node.child("python")};
        for (pugi::xml_node python_node: python_nodes)
        {
            if (python_node.attribute("file-path"))
            {
                QFileInfo pyinfo(application_node.child("python").attribute("file-path").value());

                if (pyinfo.exists() && pyinfo.isFile() && pyinfo.isExecutable())
                    return pyinfo.absoluteFilePath();
            }
        }
        return settings.getEmbeddedPythonPath();
    }
};

#endif
