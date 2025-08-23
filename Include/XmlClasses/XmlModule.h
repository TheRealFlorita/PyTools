#ifndef XMLMODULE_H
#define XMLMODULE_H

#include <QDebug>
#include <HdLayouts.h>
#include <HdWidgets.h>

#include <XmlAbstractObject.h>
#include <Settings.h>
#include <FramelessInputDialog.h>


class XmlModule : public HdWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    FramelessTabBar *tabBar;
    HdStackedWidget *stackedWidget;
    int rowheight;
    bool moduleExpandable;

public:
    explicit XmlModule(pugi::xml_node node, QWidget *parent = Q_NULLPTR);
    void initialize();
    QString toString();
    QString getName();
    QString getFilePath();
    bool hasDocumentation();
    QString getDocumentationFilePath();
    int getObjectLabelWidth(pugi::xml_node node);
    int getRowHeight();
    void reset();
    static bool generateXmlObject(XmlModule *module, HdBoxLayout *boxlayout, pugi::xml_node node, int rowheight, int labelwidth, bool test);

private:
    void moveTab(int from, int to);
    void renameTab(int index, QString name);
    void toggleTab(int index, bool enable);
    void newTab(int index);
    void copyTab(int original, int index);
    void closeTab(int index);
    void changeCurrentTab(int index);
    void generateTab(int index, pugi::xml_node tab_node);

};

#endif
