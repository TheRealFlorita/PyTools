#ifndef PYTOOLS_H
#define PYTOOLS_H

#include <QTranslator>
#include <HdWidgets.h>

#include <FramelessMainWindow.h>
#include <FramelessFileDialog.h>
#include <FramelessMessageBox.h>

#include <PyAction.h>
#include <XmlModule.h>
#include <XmlApplication.h>
#include <Settings.h>

class PyDock;

class PyTools : public FramelessMainWindow
{
    Q_OBJECT

signals:
    void languageChanged();
    void moduleChanged();

public:
    explicit PyTools(QWidget *parent = Q_NULLPTR);
    ~PyTools() override;

protected:
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent* event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:

    void createSessionMenu();
    void createModulesMenu();
    void createActionsMenu();
    void createHelpMenu();

    void loadXmlFile(QString filepath, QString name = "");
    void createFileDialog();

    QTranslator qtTranslator, ptTranslator;
    QMenu *sessionMenu, *modulesMenu, *actionsMenu;
    FramelessFileDialog *fileDialog = Q_NULLPTR;
    XmlApplication *xmlApp;
    PyDock* pyDock;
    HdToolBar *statusWidget;
    HdProgressBar *progressBar;
    HdStatusBar *statusBar;

public:
    void initializeModules();
    void loadActions();

public slots:
    void resetSessionTriggered();
    void openSessionTriggered();
    void saveSessionTriggered();
    void openSession(QString filepath);
    void saveSession(QString filepath);
    void installModuleTriggered();
    void installModule(QString archive);
    void startModule();
    void stopModule();
    void pyModuleTriggered();
    void pyActionTriggered();
    void runButtonClicked(bool checked);
    void openQtSourceWindow();
    void aboutMe();
    void onScreenChanged();

};

#endif
