#ifndef FRAMELESSMAINWINDOW_H
#define FRAMELESSMAINWINDOW_H

#include <HdWidgets.h>
#include <FramelessWindowTitleBar.h>
#include <Settings.h>


class FramelessMainWindow : public HdMainWindow
{
    Q_OBJECT

protected:
    FramelessWindowTitleBar *titleBar;
    HdBoxLayout *boxLayout;
    double screenScale = 1.0;
    double dpi = 1.0;
    double stepSize = 0.05;
    bool keyScaling = true;

signals:
    void screenChanged(QScreen*);

public:
    explicit FramelessMainWindow(QWidget *parent = Q_NULLPTR, FramelessWindowTitleBar::WindowType windowtype = FramelessWindowTitleBar::MAINWINDOW, bool scale = true) : HdMainWindow(parent)
    {
        titleBar = new FramelessWindowTitleBar(this, windowtype);
        connect(this , &FramelessMainWindow::dpiScaleChanged, titleBar, &FramelessWindowTitleBar::updateDpiScale);

        setWindowTitle(settings.getApplicationName());

        QWidget *menuWidget = new QWidget(this);
        HdMainWindow::setMenuWidget(menuWidget);

        boxLayout = new HdBoxLayout(QBoxLayout::TopToBottom, menuWidget);
        boxLayout->addWidget(titleBar);

        /* Define window size and position */
        setMinimumSize(400,400);
        resetSize();

        setFocusPolicy(Qt::StrongFocus);
        setFocus();

        /* DPI settings */
        screenScale = screen()->logicalDotsPerInchX() / 96.0;
        if (scale)
            QTimer::singleShot(0, this, [this](){ setDpiScale(settings.getGlobalDpiScale() * screenScale); });
        else
            QTimer::singleShot(0, this, [this](){ setDpiScale(settings.getGlobalDpiScale()); });
    }

    ~FramelessMainWindow() override
    { }

    void resetSize()
    {
        int hscreen = screen()->availableGeometry().height();
        int wscreen = screen()->availableGeometry().width();
        int hwindow = qRound(0.8*hscreen);
        int wwindow = qRound(0.8*wscreen);

        resize(wwindow, hwindow);
        center();
    }

    void center()
    {
        int hscreen = screen()->availableGeometry().height();
        int wscreen = screen()->availableGeometry().width();

        move((wscreen - width())/2 + screen()->availableGeometry().left()-1,
             (hscreen - height())/2 + screen()->availableGeometry().top()-1);
    }

    FramelessWindowTitleBar* getTitleBarWidget()
    {
        return titleBar;
    }

    void setMenuBar(QMenuBar *menuBar)
    {
        boxLayout->insertWidget(1, menuBar);
    }

    void setMenuWidget(QWidget *menuWidget)
    {
        boxLayout->insertWidget(1, menuWidget);
    }

    QMenu* mainMenu()
    {
        return titleBar->getMainMenu();
    }

    QMenu* helpMenu()
    {
        return titleBar->getHelpMenu();
    }

    double getDpiScale()
    {
        return dpi;
    }

    void setDpiScale(double scale)
    {
        dpi = std::min(100, std::max(1, qRound(scale/stepSize))) * stepSize;
        setStyleSheet(settings.getScaledStyleSheet(dpi));
        updateScaling();
        emit dpiScaleChanged(dpi);
     }

    void increaseDpiScale()
    {
        updateDpiScale(dpi + stepSize);
    }

    void decreaseDpiScale()
    {
        updateDpiScale(dpi - stepSize);
    }

    double getDpiStepSize()
    {
        return stepSize;
    }

    void updateDpiScale(double scale)
    {
        if (std::abs(dpi-scale) > 1e-3)
            setDpiScale(scale);
        HdMainWindow::updateDpiScale(dpi);
    }

    void setEnableKeyScaling(bool enable)
    {
        keyScaling = enable;
    }

    void onScreenChanged()
    {
        double oldScreenScale = screenScale;

        if (screen() != nullptr)
        {
            screenScale = screen()->logicalDotsPerInchX() / 96.0;
            connect(screen(), &QScreen::availableGeometryChanged, this, &FramelessMainWindow::onScreenChanged, Qt::UniqueConnection);
        }
        else
            screenScale = 1.0;

        setDpiScale(dpi * screenScale / oldScreenScale);
    }

protected:

    bool event(QEvent* event) override
    {
        switch(event->type())
        {
        case QEvent::Show:
            connect(windowHandle(), &QWindow::screenChanged, this, &FramelessMainWindow::screenChanged, Qt::UniqueConnection);
            connect(this, &FramelessMainWindow::screenChanged, this, &FramelessMainWindow::onScreenChanged, Qt::UniqueConnection);
            connect(this, &FramelessMainWindow::screenChanged, titleBar, &FramelessWindowTitleBar::onScreenChanged, Qt::UniqueConnection);
            connect(screen(), &QScreen::availableGeometryChanged, this, &FramelessMainWindow::onScreenChanged, Qt::UniqueConnection);
            break;
        case QEvent::KeyPress:
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if ((keyEvent->key() == Qt::Key_Plus) && (keyEvent->modifiers() & Qt::ControlModifier) && keyScaling)
                increaseDpiScale();
            else if((keyEvent->key() == Qt::Key_Minus) && (keyEvent->modifiers() & Qt::ControlModifier) && keyScaling)
                decreaseDpiScale();
            else if((keyEvent->key() == Qt::Key_0) && (keyEvent->modifiers() & Qt::ControlModifier) && keyScaling)
                setDpiScale(screen()->logicalDotsPerInchX() / 96.0);
            break;
        }
        default:
            break;
        }

        return HdMainWindow::event(event);
    }

};

#endif
