#ifndef PYDOCK_H
#define PYDOCK_H

#include <QTextEdit>
#include <HdWidgets.h>
#include <FramelessDockWidget.h>
#include <FramelessDockButtons.h>

class PyProcess;
class PyTools;

class PyDock : public FramelessDockWidget
{
    Q_OBJECT

private:
    PyTools *pyTools;
    HdToolBar *toolBar;
    FramelessDockButton *runButton, *dockingButton;
    QTextEdit *terminal;
    PyProcess *pyProcess;
    QColor textColor;
    qreal indent = 0.0;
    bool bold = false;
    bool cursive = false;
    int barHeight = 30;
    bool dynamicBarHeight = false;
    bool printsEnabled = true;

signals:
    void buttonSizeChange(int, int);
    void iconSizeChange(int, int);
    void runButtonClicked(bool checked);
    void terminalCleared();

public:

    explicit PyDock(PyTools *parent = Q_NULLPTR);
    ~PyDock() override;

    PyProcess* process();
    void setTitleBarHeight(int h);
    void setDynamicTitleBarHeight(int h);
    void setStatusBar(HdStatusBar *statusbar);
    void setProgressBar(HdProgressBar *progressbar);

    void refreshRunButton();
    void clearTerminal();    
    void setTextColor(QColor color);
    void setBlockFormat();
    void printRegular();
    void printBold();
    void printCursive();
    void printBoldCursive();
    void increaseIndent();
    void decreaseIndent();
    void resetIndent();
    void terminalPrint(QString text);
    void terminalErrorPrint(QString text);
    void onScreenChanged();

private:

    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

};

#endif
