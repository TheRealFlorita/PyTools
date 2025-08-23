#ifndef PYACTION_H
#define PYACTION_H

#include <QAction>
#include <QWidget>
#include <pugixml.hpp>

class PyAction : public QAction
{
    Q_OBJECT

private:
    QString name, filepath;
    bool runSilent;

public:
    explicit PyAction(QWidget *parent, pugi::xml_node node) : QAction(dynamic_cast<QObject*>(parent))
    {
        name = node.attribute("name").value();
        filepath = node.attribute("file-path").value();
        runSilent = false;

        if (node.attribute("run-silent"))
            runSilent = QString(node.attribute("run-silent").value()).replace("true","1", Qt::CaseInsensitive).replace("false","0", Qt::CaseInsensitive).toInt();

        setText(name);

        /* Set short key */
        if (node.attribute("short-key"))
            setShortcut(QKeySequence(node.attribute("short-key").value()));

        /* Tool tip */
        if (node.attribute("tool-tip"))
            setToolTip(QString(node.attribute("tool-tip").value()).trimmed());
    }

    explicit PyAction(QWidget *parent, QString text, QString file, bool silent = false, QString keysequence = "") : QAction(dynamic_cast<QObject*>(parent))
    {
        name = text;
        filepath = file;
        runSilent = silent;
        setText(name);

        /* Set short key */
        if (!keysequence.trimmed().isEmpty())
            setShortcut(QKeySequence(keysequence));
    }

    QString getFilePath()
    {
        return filepath;
    }

    QString getName()
    {
        return name;
    }

    bool getRunSilent()
    {
        return runSilent;
    }

};

#endif
