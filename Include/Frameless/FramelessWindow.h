#ifndef FRAMELESSWINDOW_H
#define FRAMELESSWINDOW_H

#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QScreen>

#include <HdLayouts.h>
#include <HdWidgets.h>
#include <FramelessWindowTitleBar.h>
#include <Settings.h>


class FramelessWindow : public HdWidget
{
Q_OBJECT

protected:
    FramelessWindowTitleBar *titleBar;
    HdWidget *statusBar;
    HdBoxLayout *boxLayout;
    double screenScale = 1.0;
    double dpi = 1.0;
    double stepSize = 0.05;
    bool keyScaling = true;

signals:
    void screenChanged(QScreen*);

public:

    explicit FramelessWindow(QWidget *parent = Q_NULLPTR, FramelessWindowTitleBar::WindowType windowtype = FramelessWindowTitleBar::WINDOW, bool scale = true) : HdWidget(parent)
    {
        titleBar = new FramelessWindowTitleBar(this, windowtype);
        connect(this , &FramelessWindow::dpiScaleChanged, titleBar, &FramelessWindowTitleBar::updateDpiScale);

        setWindowTitle(settings.getApplicationName());

        /* Central layout */
        QBoxLayout *centralLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        centralLayout->setContentsMargins(0,0,0,0);
        centralLayout->setSpacing(0);
        centralLayout->addWidget(titleBar);

        /* Central widget */
        QWidget *central = new QWidget(this);
        centralLayout->addWidget(central,1000);

        boxLayout = new HdBoxLayout(QBoxLayout::TopToBottom, central);

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

    ~FramelessWindow() override
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

    void setCentralWidget(QWidget *widget)
    {
        boxLayout->insertWidget(0, widget);
    }

    HdBoxLayout* layout()
    {
        return boxLayout;
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
        HdWidget::updateDpiScale(dpi);
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
            connect(screen(), &QScreen::availableGeometryChanged, this, &FramelessWindow::onScreenChanged, Qt::UniqueConnection);
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
            connect(windowHandle(), &QWindow::screenChanged, this, &FramelessWindow::screenChanged, Qt::UniqueConnection);
            connect(this, &FramelessWindow::screenChanged, this, &FramelessWindow::onScreenChanged, Qt::UniqueConnection);
            connect(this, &FramelessWindow::screenChanged, titleBar, &FramelessWindowTitleBar::onScreenChanged, Qt::UniqueConnection);
            connect(screen(), &QScreen::availableGeometryChanged, this, &FramelessWindow::onScreenChanged, Qt::UniqueConnection);
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

        return HdWidget::event(event);
    }

};

#endif
