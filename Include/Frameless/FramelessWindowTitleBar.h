#ifndef FRAMELESSWINDOWTITLEBAR_H
#define FRAMELESSWINDOWTITLEBAR_H

#include <HdWidgets.h>
#include <HdLayouts.h>
#include <FramelessWindowButtons.h>
#include <QWKWidgets/widgetwindowagent.h>
#include <Settings.h>

class FramelessWindowTitleBar : public HdWidget
{
Q_OBJECT

public:

    enum WindowType
    {
        MAINWINDOW, MAINWINDOWTITLE, WINDOW, DOCK, DIALOG
    };

private:
    HdLabel *titleLabel = Q_NULLPTR;
    QMenuBar *menuBar;
    HdBoxLayout *boxLayout;
    QMenu *mainMenu, *titleMenu, *helpMenu;
    WindowType windowType;
    FramelessWindowButton *menuButton = Q_NULLPTR;
    FramelessWindowButton *titleButton = Q_NULLPTR;
    FramelessWindowButton *helpButton = Q_NULLPTR;
    FramelessWindowButton *minimizeButoon = Q_NULLPTR;
    FramelessWindowButton *maximizeButton = Q_NULLPTR;
    FramelessWindowButton *closeButton;

signals:
    void mainMenuButtonClicked();
    void titleMenuButtonClicked();
    void helpMenuButtonClicked();
    void closeButtonClicked();

public:

    explicit FramelessWindowTitleBar(QWidget *parent, WindowType windowtype = MAINWINDOW) : HdWidget(parent)
    {
        windowType = windowtype;

        QWK::WidgetWindowAgent *agent = new QWK::WidgetWindowAgent(this);
        agent->setup(parent);
        agent->setTitleBar(this);
        setWindowTitle(parent->windowTitle());

        connect(window(), &QWidget::windowTitleChanged, this, &FramelessWindowTitleBar::setLabel);

        int height = getWindowTitleSizeHintHeight();

        boxLayout = new HdBoxLayout(QBoxLayout::LeftToRight, this);
        boxLayout->setDynamicSpacing(1);
        boxLayout->setContentsMargins(0,0,0,0);
        connect(this , &FramelessWindowTitleBar::dpiScaleChanged, boxLayout, &HdBoxLayout::updateDpiScale);

        if ((windowType == MAINWINDOW) or (windowType == MAINWINDOWTITLE))
        {
            menuButton = new FramelessWindowButton(FramelessWindowButton::ButtonType::BUTTON_MENU, this);
            menuButton->setDynamicSize(height, height);
            boxLayout->addWidget(menuButton,1,Qt::AlignTop);
            connect(this , &FramelessWindowTitleBar::dpiScaleChanged, menuButton, &FramelessWindowButton::updateDpiScale);
            connect(menuButton, &QAbstractButton::clicked, this, &FramelessWindowTitleBar::mainMenuClicked);
            connect(menuButton, &QAbstractButton::clicked, this, &FramelessWindowTitleBar::mainMenuButtonClicked);
            agent->setHitTestVisible(menuButton, true);

            if (windowType == MAINWINDOWTITLE)
            {
                titleButton = new FramelessWindowButton(FramelessWindowButton::ButtonType::BUTTON_TITLE, this);;
                titleButton->setText(windowTitle());
                titleButton->setDynamicSize(90, height);
                boxLayout->addWidget(titleButton,1,Qt::AlignLeft | Qt::AlignVCenter);
                connect(this , &FramelessWindowTitleBar::dpiScaleChanged, titleButton, &FramelessWindowButton::updateDpiScale);
                connect(titleButton, &QAbstractButton::clicked, this , &FramelessWindowTitleBar::titleMenuClicked);
                connect(titleButton, &QAbstractButton::clicked, this , &FramelessWindowTitleBar::titleMenuButtonClicked);
                agent->setHitTestVisible(titleButton);
            }
            else
            {
                titleLabel = new HdLabel(windowTitle(), this);
                titleLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
                titleLabel->setDynamicHeight(height);
                titleLabel->setIndent(0);
                boxLayout->addWidget(titleLabel,1,Qt::AlignLeft | Qt::AlignVCenter);
                connect(this , &FramelessWindowTitleBar::dpiScaleChanged, titleLabel, &HdLabel::updateDpiScale);
            }
            boxLayout->addStretch(100);

            helpButton = new FramelessWindowButton(FramelessWindowButton::ButtonType::BUTTON_HELP, this);
            helpButton->setDynamicSize(height, height);
            boxLayout->addWidget(helpButton,1,Qt::AlignTop);
            connect(this , &FramelessWindowTitleBar::dpiScaleChanged, helpButton, &FramelessWindowButton::updateDpiScale);
            connect(helpButton, &FramelessWindowButton::clicked, this, &FramelessWindowTitleBar::helpMenuClicked);
            connect(helpButton, &FramelessWindowButton::clicked, this, &FramelessWindowTitleBar::helpMenuButtonClicked);
            agent->setSystemButton(QWK::WindowAgentBase::Help, helpButton);
        }
        else
        {
            titleLabel = new HdLabel(windowTitle(), this);
            titleLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
            titleLabel->setDynamicHeight(height);
            titleLabel->setIndent(0);
            connect(this , &FramelessWindowTitleBar::dpiScaleChanged, titleLabel, &HdLabel::updateDpiScale);

            boxLayout->setDynamicMargins(4,0,0,0);
            boxLayout->addWidget(titleLabel,1,Qt::AlignLeft | Qt::AlignVCenter);
            boxLayout->addStretch(1000);
        }

        if (windowType != DIALOG)
        {
            minimizeButoon = new FramelessWindowButton(FramelessWindowButton::ButtonType::BUTTON_MINIMIZE, this);
            minimizeButoon->setDynamicSize(height, height);
            boxLayout->addWidget(minimizeButoon,1,Qt::AlignRight | Qt::AlignVCenter);
            connect(this , &FramelessWindowTitleBar::dpiScaleChanged, minimizeButoon, &FramelessWindowButton::updateDpiScale);
            connect(minimizeButoon, &FramelessWindowButton::clicked, window(), &QWidget::showMinimized);
            agent->setSystemButton(QWK::WindowAgentBase::Minimize, minimizeButoon);

            maximizeButton = new FramelessWindowButton(FramelessWindowButton::ButtonType::BUTTON_MAXIMIZE, this);
            maximizeButton->setDynamicSize(height, height);
            boxLayout->addWidget(maximizeButton,1,Qt::AlignRight | Qt::AlignVCenter);
            connect(this , &FramelessWindowTitleBar::dpiScaleChanged, maximizeButton, &FramelessWindowButton::updateDpiScale);

            connect(maximizeButton, &FramelessWindowButton::clicked, this, [this]()
                {
                    if (window()->isFullScreen())
                        window()->showNormal();
                    else
                        window()->showFullScreen();
                });

            agent->setSystemButton(QWK::WindowAgentBase::Maximize, maximizeButton);
        }


        closeButton = new FramelessWindowButton(FramelessWindowButton::ButtonType::BUTTON_CLOSE, this);
        closeButton->setDynamicSize(height, height);
        boxLayout->addWidget(closeButton,1,Qt::AlignTop);
        connect(this , &FramelessWindowTitleBar::dpiScaleChanged, closeButton, &FramelessWindowButton::updateDpiScale);
        connect(closeButton, &FramelessWindowButton::clicked, this , &FramelessWindowTitleBar::closeButtonClicked);
        agent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);

        if (windowType != DOCK)
            connect(closeButton, &FramelessWindowButton::clicked, window(), &QWidget::close);

        /* Add menu's and bar (menubar required for shortcuts, do not hide) */
        menuBar = new QMenuBar(this);
        menuBar->setFixedSize(0,0);

        mainMenu = new QMenu(this);
        menuBar->addMenu(mainMenu);
        mainMenu->setWindowFlag(Qt::NoDropShadowWindowHint);

        titleMenu = new QMenu(this);
        menuBar->addMenu(titleMenu);
        titleMenu->setWindowFlag(Qt::NoDropShadowWindowHint);

        helpMenu = new QMenu(this);
        menuBar->addMenu(helpMenu);
        helpMenu->setWindowFlag(Qt::NoDropShadowWindowHint);

        setFocusPolicy(Qt::StrongFocus);

        setDynamicTitleBarHeight(height);
    }

    ~FramelessWindowTitleBar() override
    {}

    void setLabel(QString caption)
    {
        setWindowTitle(caption);

        if (titleLabel != Q_NULLPTR)
            titleLabel->setText(caption);

        if (titleButton != Q_NULLPTR)
            titleButton->setText(caption);
    }

    QMenu* getMainMenu()
    {
        return mainMenu;
    }

    QMenu* getTitleMenu()
    {
        return titleMenu;
    }

    QMenu* getHelpMenu()
    {
        return helpMenu;
    }

    int getWindowTitleSizeHintHeight()
    {
        QLabel dummy("TeXtpad", this);
        dummy.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        dummy.setStyleSheet(settings.getBaseStyleSheet());
        dummy.ensurePolished();

        return dummy.sizeHint().height()+2;
    }

    void setDynamicTitleBarHeight(int height)
    {
        height = std::max(16, height);
        setDynamicHeight(height);

        if (titleLabel != Q_NULLPTR)
            titleLabel->setDynamicHeight(height);

        if (menuButton != Q_NULLPTR)
            menuButton->setDynamicSize(height, height);

        if (titleButton != Q_NULLPTR)
            titleButton->setDynamicHeight(height);

        if (helpButton != Q_NULLPTR)
            helpButton->setDynamicSize(height, height);

        if (minimizeButoon != Q_NULLPTR)
            minimizeButoon->setDynamicSize(height, height);

        if (maximizeButton != Q_NULLPTR)
            maximizeButton->setDynamicSize(height, height);

        closeButton->setDynamicSize(height, height);
    }

    void onScreenChanged()
    {
        int height = getWindowTitleSizeHintHeight();
        setDynamicTitleBarHeight(height);

        if (titleLabel != Q_NULLPTR)
        {
            QString txt = titleLabel->text();
            titleLabel->deleteLater();

            titleLabel = new HdLabel(txt, this);
            titleLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
            titleLabel->setDynamicHeight(height);
            titleLabel->setIndent(0);
            connect(this , &FramelessWindowTitleBar::dpiScaleChanged, titleLabel, &HdLabel::updateDpiScale);

            boxLayout->insertWidget(1,titleLabel,1,Qt::AlignVCenter);
        }

        topLevelWidget()->setStyleSheet(topLevelWidget()->styleSheet());
    }

    WindowType getWindowType()
    {
        return windowType;
    }
private:

    void mainMenuClicked()
    {
        mainMenu->move(window()->geometry().x() + pos().x() + menuButton->pos().x(),
                       window()->geometry().y() + pos().y() + height());
        mainMenu->show();
        mainMenu->raise();
        mainMenu->setFocus();

        emit mainMenuButtonClicked();
        update();
    }

    void titleMenuClicked()
    {
        titleMenu->move(window()->geometry().x() + pos().x() + titleButton->pos().x(),
                        window()->geometry().y() + pos().y() + height());
        titleMenu->show();
        titleMenu->setFocus();

        emit titleMenuButtonClicked();
        update();
    }

    void helpMenuClicked()
    {
        helpMenu->move(window()->geometry().x() + pos().x() + helpButton->pos().x() - helpMenu->sizeHint().width() + helpButton->width(),
                       window()->geometry().y() + pos().y() + height());
        helpMenu->show();
        helpMenu->setFocus();

        emit helpMenuButtonClicked();
        update();
    }

};

#endif
