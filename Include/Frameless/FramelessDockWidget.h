#ifndef FRAMELESSDOCKWIDGET_H
#define FRAMELESSDOCKWIDGET_H

#include <HdWidgets.h>
#include <FramelessMainWindow.h>
#include <FramelessWindow.h>
#include <QTimer>


class FramelessDockWidget : public HdDockWidget
{
    Q_OBJECT

private:
    FramelessMainWindow *parentMainWindow;
    FramelessMainWindow *floatingMainWindow;
    QPair<double, double> sizeRatio;
    bool floating;
    Qt::DockWidgetArea dockArea;

signals:
    void screenChanged(QScreen*);

public:

    explicit FramelessDockWidget(FramelessMainWindow *parent) : HdDockWidget(parent)
    {
        setFeatures(QDockWidget::DockWidgetMovable);
        HdDockWidget::setFloating(false);

        parentMainWindow = parent;
        floating = false;
        dockArea = Qt::RightDockWidgetArea;
        floatingMainWindow = new FramelessMainWindow(Q_NULLPTR, FramelessWindowTitleBar::DOCK);
        floatingMainWindow->setEnableKeyScaling(false);
        floatingMainWindow->hide();
        sizeRatio = {0.5, 0.2};

        connect(this, &FramelessDockWidget::dpiScaleChanged, floatingMainWindow, &FramelessMainWindow::updateDpiScale);
        connect(floatingMainWindow->getTitleBarWidget(), &FramelessWindowTitleBar::closeButtonClicked, this, [this](){ setFloating(false); });
        connect(parentMainWindow, &FramelessMainWindow::screenChanged, this, &FramelessDockWidget::screenChanged);
        connect(floatingMainWindow, &FramelessMainWindow::screenChanged, this, &FramelessDockWidget::screenChanged);
        setFocusPolicy(Qt::StrongFocus);
    }

    ~FramelessDockWidget() override
    {
        if (isFloating())
            setFloating(false);
        resizeToRatio();

        floatingMainWindow->close();
        floatingMainWindow->deleteLater();
    }

    FramelessMainWindow* getParentMainWindow()
    {
        return parentMainWindow;
    }

    FramelessMainWindow* getFramelessMainWindow()
    {
        return floatingMainWindow;
    }

    void setWindowTitle(QString title)
    {
        HdDockWidget::setWindowTitle(title);
        floatingMainWindow->setWindowTitle(title);
    }

    void setDpiScale(double scale)
    {
        HdDockWidget::setDpiScale(scale);
        floatingMainWindow->setDpiScale(getDpiScale());
    }

    void updateDpiScale(double scale)
    {
        HdDockWidget::updateDpiScale(scale);
        floatingMainWindow->updateDpiScale(getDpiScale());
    }

    QPair<double, double> getRatio()
    {
        return sizeRatio;
    }

    void setRatio(double width, double height)
    {
        sizeRatio = {width, height};
    }

    void setRatio(QPair<double, double> screenratio)
    {
        sizeRatio = screenratio;
    }

    void updateRatio()
    {
        if (!isFloating())
            sizeRatio = {double(width())/parentMainWindow->width(), double(height())/parentMainWindow->height()};
    }

    void resizeToRatio()
    {
        if (!isFloating())
        {
            parentMainWindow->resizeDocks({this}, {qRound(parentMainWindow->height()*sizeRatio.second)}, Qt::Vertical);
            parentMainWindow->resizeDocks({this}, {qRound(parentMainWindow->width()*sizeRatio.first)}, Qt::Horizontal);
        }
    }

    void resetDock()
    {
        setFloating(false);
        parentMainWindow->resizeDocks({this}, {qRound(parentMainWindow->height()*0.2)}, Qt::Vertical);
        parentMainWindow->resizeDocks({this}, {parentMainWindow->width()/2-qRound(getDpiScale()*1.0)}, Qt::Horizontal);
        QTimer::singleShot(0, this, [this](){updateRatio();});
    }

    void showEvent(QShowEvent *event) override
    {
        HdDockWidget::showEvent(event);
        updateRatio();
    }

    void resizeEvent(QResizeEvent* event) override
    {
        if (!isFloating())
        {
            if (cursor().shape() == 11 || cursor().shape() == 12)
                updateRatio();
        }

        HdDockWidget::resizeEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (isFloating())
        {
            QApplication::sendEvent(parentMainWindow, static_cast<QEvent*>(event));
            event->accept();
        }
        else
            HdDockWidget::keyPressEvent(event);
    }

    bool isFloating()
    {
        return floating;
    }

    void setFloating(bool enable)
    {
        if (!isFloating())
        {
            Qt::DockWidgetArea area = parentMainWindow->dockWidgetArea(this);

            if (area != Qt::NoDockWidgetArea)
                dockArea = area;
        }

        if (enable == isFloating())
            return;
        else if (enable)
        {
            floatingMainWindow->setDpiScale(parentMainWindow->getDpiScale());
            floatingMainWindow->showNormal();
            floatingMainWindow->raise();
            floatingMainWindow->setCentralWidget(this);
        }
        else
        {
            if (floatingMainWindow->isMinimized())
                floatingMainWindow->showNormal();

            parentMainWindow->addDockWidget(dockArea, this);
            floatingMainWindow->close();
        }

        floating = enable;
        emit topLevelChanged(isFloating());

        QTimer::singleShot(0, this, [this](){setFocus();});
    }
};

#endif
