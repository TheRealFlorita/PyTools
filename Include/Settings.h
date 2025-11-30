#ifndef SETTINGS_H
#define SETTINGS_H

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QLineEdit>
#include <QLabel>
#include <QTabBar>
#include <QUrl>
#include <QScreen>


class Settings : public QObject
{
    Q_OBJECT

private:
    QList<QUrl> locations;
    QString applicationPath, appDataPath, embeddedPythonPath, pythonPath, dialogPath;
    QString rawStyleSheet, baseStyleSheet;
    QSettings *settings;
    QMap<QString, QVariant> configs;

signals:
    void globalDpiScaleChanged(double);
    void applicationNameChanged(QString);
    void pythonPathChanged(QString);
    void aboutToQuit();

public:

    Settings(QObject* parent = Q_NULLPTR) : QObject(parent)
    { }

    ~Settings() override
    { }

    void load()
    {
        /* Set application path as current directory */
        applicationPath = QApplication::applicationDirPath();
        QDir::setCurrent(applicationPath);

        /* Define default configuration settings */
        configs.clear();
        configs["application/name"] = "PyTools";
        configs["application/documentation"] = "";

        configs["font-type/title"] = "Century Gothic";
        configs["font-type/regular"] = "Segoe UI";
        configs["font-type/monospace"] = "Consolas";

        configs["font-size/small"] = "9.0pt";
        configs["font-size/medium"] = "10.0pt";
        configs["font-size/large"] = "11.0pt";
        configs["font-size/title"] = "11.0pt";
        configs["font-size/monospace"] = "10.0pt";

        configs["color/theme-border"] = "rgb(50,50,50)";
        configs["color/theme-title-bar"] = "rgb(10,10,10)";
        configs["color/theme-highlight"] = "rgb(117,115,215)";
        configs["color/theme-groupbox"] = "rgb(40,40,40)";
        configs["color/theme-light"] = "rgb(50,50,50)";
        configs["color/theme-medium"] = "rgb(30,30,30)";
        configs["color/theme-dark"] = "rgb(10,10,10)";

        configs["color/blend-lighter"] = "rgba(255,255,255,75)";
        configs["color/blend-darker"] = "rgba(0,0,0,75)";

        configs["drop-shadow/color"] = "rgba(0,0,0,50)";
        configs["drop-shadow/blur-radius"] = "12";

        configs["styling/groupbox-border-radius"] = "6px";
        configs["styling/items-border-radius"] = "4px";
        configs["styling/scroll-bar-size"] = "6%px";
        configs["styling/scroll-bar-border-radius"] = "6px";

        /* Load settings from config.ini */
        QSettings config(applicationPath + "/config.ini", QSettings::IniFormat, this);
        for (QString &key : config.allKeys())
            configs[key] = config.value(key);

        /* Load settings from settings.ini */
        QApplication::setApplicationName(getApplicationName());
        appDataPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + getApplicationName();
        dialogPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        settings = new QSettings(appDataPath + "/settings.ini", QSettings::IniFormat, this);

        /* Load stylesheet */
        QFile file(applicationPath + "/styleSheet.css");
        if (file.open(QFile::ReadOnly))
            rawStyleSheet = file.readAll();
        file.close();

        /* Set icon directory */
        rawStyleSheet.replace("%ICONDIR%", applicationPath + "/icons");

        /* Set colors and fonts from configuration settings */
        for (QMap<QString, QVariant>::iterator it = configs.begin(); it != configs.end(); ++it)
            rawStyleSheet.replace(QString("%" + it.key() + "%"), it.value().toString());

        /* Get default heigth of a line edit */
        QLineEdit dummy("TeXtpad");
        QString tmp = rawStyleSheet;
        tmp.replace("%button-height%", "1px");
        dummy.setStyleSheet(tmp);
        int height = dummy.sizeHint().height();

        /* Set default height in stylesheet */
        rawStyleSheet.replace("%button-height%", QString::number(height-2) + "px");
        baseStyleSheet = cssScale(rawStyleSheet, 1.0);

        /* Debug stylesheet */
        int i = 0;
        for (QString &line: baseStyleSheet.split("\r\n"))
        {
            ++i;
            if (line.contains("%"))
                qDebug() << i << ": " << line;
        }

        initializePaths();
    }

    QString getApplicationPath()
    {
        return applicationPath;
    }

    QString getAppDataPath()
    {
        return appDataPath;
    }

    QString getDialogPath()
    {
        return dialogPath;
    }

    void setDialogPath(QString path)
    {
        if (!path.trimmed().isEmpty())
        {
            QFileInfo p(path);
            if (p.exists() && p.isDir())
                dialogPath = p.absoluteFilePath();
            else if (p.exists() && p.isFile())
                dialogPath = p.absolutePath();
            else
                dialogPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        }
        else
            dialogPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }

    QString getEmbeddedPythonPath()
    {
        return embeddedPythonPath;
    }

    bool setPythonPath(QString path)
    {
        QFileInfo exefile(path);
        if (exefile.exists() && exefile.fileName().toLower() == "python.exe")
        {
            if (exefile.absoluteFilePath().toLower() != pythonPath.toLower())
            {
                pythonPath = exefile.absoluteFilePath();
                emit pythonPathChanged(pythonPath);
            }
            return true;
        }
        return false;
    }

    QString getPythonPath()
    {
        return pythonPath;
    }

    QString getApplicationName()
    {
        return getValue("application/name", "PyTools").toString();
    }

    void setApplicationName(QString name)
    {
        setValue("application/name", name);
        emit applicationNameChanged(name);
    }

    QList<QUrl> getFileDialogLocations()
    {
        return locations;
    }

    QString getRawStyleSheet()
    {
        return rawStyleSheet;
    }

    QString getBaseStyleSheet()
    {
        return baseStyleSheet;
    }

    QString getScaledStyleSheet(double scale)
    {
        return cssScale(rawStyleSheet, scale);
    }

    void setGlobalDpiScale(double dpi)
    {
        double globalDpiScale = std::max(0.25, std::min(4.0,dpi));
        setValue("settings/dpi-factor", globalDpiScale);
        emit globalDpiScaleChanged(globalDpiScale);
    }

    double getGlobalDpiScale()
    {
        return getValue("settings/dpi-factor", 1.0).toDouble();
    }

    void save()
    {
        settings->sync();
    }

    QVariant getValue(QString key, QVariant value = QVariant())
    {
        if (settings)
            if (settings->contains(key))
                return settings->value(key);

        if (configs.contains(key))
            return configs[key];

        return value;
    }

    void setValue(QString key, QVariant value, bool sync = true)
    {
        if (sync)
            settings->setValue(key, value);
        else
            configs[key] = value;
    }

    void addSetting(QString key, QVariant value, bool sync=true)
    {
        if(sync)
            settings->setValue(key, value);
        else
            configs[key] = value;
    }

    QColor getColor(QString key)
    {
        QColor color(255,255,255);

        QString value = getValue(key, "rgba(255,0,0,130)").toString();

        if (value.startsWith("rgb("))
        {
            value = value.remove("rgb(").remove(")");
            QStringList rgb = value.split(",");

            if (rgb.length() == 3)
                color = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
        }
        else if (value.startsWith("rgba("))
        {
            value = value.remove("rgba(").remove(")");
            QStringList rgb = value.split(",");

            if (rgb.length() == 4)
                color = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt(), rgb[3].toInt());
        }

        return color;
    }

    QString getFontType(QString key)
    {
        return getValue("font-type/" + key, QVariant("Segoe UI")).toString();
    }

    int getLabelHeight(QWidget *parent = Q_NULLPTR)
    {
        QLabel dummy("TeXtpad", parent);
        dummy.setIndent(0);
        dummy.setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
        dummy.setStyleSheet(baseStyleSheet);
        dummy.ensurePolished();
        return dummy.sizeHint().height();
    }

    int getLineEditHeight(QWidget *parent = Q_NULLPTR)
    {
        QLineEdit dummy("TeXtpad", parent);
        dummy.setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
        dummy.setStyleSheet(baseStyleSheet);
        return dummy.sizeHint().height();
    }

    int getLabelWidth(int length, QWidget *parent = Q_NULLPTR)
    {
        QLabel dummy(QString("X").repeated(length), parent);
        dummy.setIndent(0);
        dummy.setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
        dummy.setStyleSheet(baseStyleSheet);
        dummy.ensurePolished();
        return dummy.sizeHint().width();
    }

    int getLabelWidth(QString text, QWidget *parent = Q_NULLPTR)
    {
        QLabel dummy(text, parent);
        dummy.setIndent(0);
        dummy.setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
        dummy.setStyleSheet(baseStyleSheet);
        dummy.ensurePolished();
        return dummy.sizeHint().width();
    }

    int getLabelWidth(QList<QString> texts, QWidget *parent = Q_NULLPTR)
    {
        int width = 0;

        if (texts.isEmpty())
            return width;

        QLabel dummy(parent);
        dummy.setIndent(0);
        dummy.setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
        dummy.setStyleSheet(baseStyleSheet);
        dummy.ensurePolished();

        for (const QString &text: texts)
        {
            dummy.setText(text);
            width = std::max(width, dummy.sizeHint().width());
        }

        return width;
    }

    int getTabBarHeight(QWidget *parent = Q_NULLPTR)
    {
        QTabBar dummy(parent);
        dummy.setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
        dummy.setStyleSheet(baseStyleSheet);
        dummy.addTab("TeXtpad");
        dummy.ensurePolished();
        return dummy.sizeHint().height();
    }

    QString getEditorFilePath()
    {
        return QFileInfo(getValue("settings/editor-executable", "notepad.exe").toString()).filePath();
    }

    void openSettingsFileInEditor()
    {
        QProcess::startDetached(getEditorFilePath(), {settings->fileName()});
    }

    bool wordWrapIsOn()
    {
        return getValue("settings/word-wrap", "true").toBool();
    }

private:

    void initializePaths()
    {
        /* Locate python executable */
        embeddedPythonPath.clear();
        QDirIterator it(applicationPath, QStringList() << "python*", QDir::Dirs);
        while (it.hasNext())
        {
            QString exefile = it.next() + "/python.exe";
            if (QFile::exists(exefile))
                embeddedPythonPath = QFileInfo(exefile).absoluteFilePath();
        }
        setPythonPath(embeddedPythonPath);

        /* Create locations list */
        locations.clear();
        locations.append(QUrl::fromLocalFile("//"));

        foreach(QFileInfo drive, QDir::drives())
            locations.append(QUrl::fromLocalFile(drive.path()));

        locations.append(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)));
        locations.append(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)));
        locations.append(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
        locations.append(QUrl::fromLocalFile(getAppDataPath()));

        QStringList paths = getValue("settings/file-dialog-locations", "").toString().split(";");

        foreach(QString path, paths)
        {
            if (!path.trimmed().isEmpty())
            {
                QFileInfo pinfo(path);
                if (pinfo.isDir())
                    locations.append(QUrl::fromLocalFile(pinfo.absoluteFilePath()));
            }
        }
    }

    static QString cssScale(QString text, double scale)
    {
        /* Scale pixel sizes "8px" */
        static QRegularExpression expression;
        expression.setPattern("\\b([0-9]+[.,]?[0-9]*)px\\b");
        QRegularExpressionMatch match;
        while ((match = expression.match(text)).hasMatch())
        {
            int offset = int(match.capturedStart(0));
            text.remove(offset, match.capturedLength(0));

            int size = match.captured(1).toInt();
            if (size > 2 || scale > 1.6)
                size = qRound(match.captured(1).toDouble() * scale);
            text.insert(offset, QString::number(size) + "#px");
        }
        text.replace("#px","px");

        /* Scale pixel halfsizes "16%px" */
        expression.setPattern("\\b([0-9]+[.,]?[0-9]*)%px\\b");
        while ((match = expression.match(text)).hasMatch())
        {
            int offset = int(match.capturedStart(0));
            text.remove(offset, match.capturedLength(0));

            int size = 2*qRound(match.captured(1).toDouble() * scale);
            text.insert(offset, QString::number(size) + "px");
        }

        /* Scale fractional point sizes "8.0pt" */
        expression.setPattern("\\b([0-9]+[.,]{1}[0-9]*)pt\\b");
        while ((match = expression.match(text)).hasMatch())
        {
            int offset = int(match.capturedStart(0));
            text.remove(offset, match.capturedLength(0));

            double pts = match.captured(1).toDouble();
            pts = 0.5*qRound(2 * std::min(72.0, std::max(6.0, pts * scale)));
            text.insert(offset, QString::number(qRound(pts*96.0/72.0)) + "px");
        }

        /* Scale integer point sizes "8pt" */
        expression.setPattern("\\b([0-9]+)pt\\b");
        QList<int> points = {8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};
        while ((match = expression.match(text)).hasMatch())
        {
            int offset = int(match.capturedStart(0));
            text.remove(offset, match.capturedLength(0));

            double pts = match.captured(1).toDouble();
            int pt = qRound(std::min(72.0, std::max(8.0, pts * scale)));

            for (int i = 0; i < points.size()-1; ++i)
            {
                if (points[i] <= pt && points[i+1] > pt)
                {
                    if ((pt - points[i]) < (points[i+1] - pt))
                        pt = points[i];
                    else
                        pt = points[i+1];
                }
            }
            text.insert(offset, QString::number(qRound(pt*96.0/72.0)) + "px");
        }

        return text;
    }

};

extern Settings settings;

#endif
