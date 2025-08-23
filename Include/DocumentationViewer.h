#ifndef DOCUMENTATIONVIEWER_H
#define DOCUMENTATIONVIEWER_H

#include <QDir>
#include <QFileInfo>

#include <HdShadowEffect.h>
#include <HdWidgets.h>
#include <FramelessMainWindow.h>


class DocumentationViewer : public FramelessMainWindow
{
    Q_OBJECT

private:
    HdToolBar *navigationBar;
    HdTextBrowser *textViewer;
    HdPushButton *homeButton, *backwardButton, *forwardButton;

public:

    explicit DocumentationViewer(QWidget *parent = Q_NULLPTR) : FramelessMainWindow(parent, FramelessWindowTitleBar::WINDOW)
    {
        setAttribute(Qt::WA_Hover,true);

        /* Create background widget with layout */
        QWidget *background = new QWidget(this);
        setCentralWidget(background);

        QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom, background);
        layout->setSpacing(0);
        layout->setContentsMargins(0,0,0,0);

        /* Create navigation bar widget */
        navigationBar = new HdToolBar(this);
        connect(this, &DocumentationViewer::dpiScaleChanged, navigationBar, &HdToolBar::updateDpiScale);
        layout->addWidget(navigationBar);

        /* Add shadow to navigation bar */
        HdShadowEffect *effect = new HdShadowEffect(navigationBar, qreal(1.0), qreal(12.0), settings.getColor("drop-shadow/color"));
        effect->setDistance(1);
        connect(this, &DocumentationViewer::dpiScaleChanged, effect, &HdShadowEffect::updateDpiScale);
        navigationBar->setGraphicsEffect(effect);

        /* Dynamic sizes */
        int barheight = settings.getTabBarHeight(this);
        int iconwidth = barheight-2;
        QSize iconsize = QSize(iconwidth,iconwidth);
        navigationBar->setDynamicHeight(barheight);

        /* Create a text browser to view documentation */
        textViewer = new HdTextBrowser(this);
        textViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(this, &DocumentationViewer::dpiScaleChanged, textViewer, &HdTextBrowser::updateDpiScale);

        HdBoxLayout *marginlayout = new HdBoxLayout(QBoxLayout::LeftToRight);
        marginlayout->setDynamicMargins(8,0,0,0);
        connect(this, &DocumentationViewer::dpiScaleChanged, textViewer, &HdTextBrowser::updateDpiScale);
        marginlayout->addWidget(textViewer);
        layout->addLayout(marginlayout);

        /* Create home button */
        homeButton = new HdPushButton(this);
        homeButton->setFixedSize(barheight, barheight);
        QIcon homeIcon;
        homeIcon.addFile(QDir::currentPath() + "/icons/home.svg", iconsize);
        homeButton->setIcon(homeIcon);
        homeButton->setIconSize(iconsize);
        homeButton->setCheckable(false);
        connect(homeButton, &HdPushButton::clicked, textViewer, &HdTextBrowser::home);
        connect(this, &DocumentationViewer::dpiScaleChanged, homeButton, &HdPushButton::updateDpiScale);
        navigationBar->addWidget(homeButton);

        /* Create backward button */
        backwardButton = new HdPushButton(this);
        backwardButton->setFixedSize(barheight, barheight);
        QIcon backwardIcon;
        backwardIcon.addFile(QDir::currentPath() + "/icons/arrow-backward.svg", iconsize);
        backwardButton->setIcon(backwardIcon);
        backwardButton->setIconSize(iconsize);
        connect(backwardButton, &HdPushButton::clicked, textViewer, &HdTextBrowser::backward);
        connect(this, &DocumentationViewer::dpiScaleChanged, backwardButton, &HdPushButton::updateDpiScale);
        navigationBar->addWidget(backwardButton);

        /* Create forward button */
        forwardButton = new HdPushButton(this);
        forwardButton->setFixedSize(barheight, barheight);
        QIcon forwardIcon;
        forwardIcon.addFile(QDir::currentPath() + "/icons/arrow-forward.svg", iconsize);
        forwardButton->setIcon(forwardIcon);
        forwardButton->setIconSize(iconsize);
        connect(forwardButton, &HdPushButton::clicked, textViewer, &HdTextBrowser::forward);
        connect(this, &DocumentationViewer::dpiScaleChanged, forwardButton, &HdPushButton::updateDpiScale);
        navigationBar->addWidget(forwardButton);

        QWidget *spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        spacer->setFixedHeight(0);
        navigationBar->addWidget(spacer);

        navigationBar->raise();
        connect(this, &DocumentationViewer::screenChanged, this, [this](){ setDynamicNavigationBarHeight(settings.getTabBarHeight(this)); });
    }

    void setNavigationBarHeight(int h)
    {
        h = std::max(12, h);

        navigationBar->setFixedHeight(h);
        homeButton->setFixedSize(h,h);
        homeButton->setIconSize(QSize(h-2, h-2));
        backwardButton->setFixedSize(h,h);
        backwardButton->setIconSize(QSize(h-2, h-2));
        forwardButton->setFixedSize(h,h);
        forwardButton->setIconSize(QSize(h-2, h-2));
    }

    void setDynamicNavigationBarHeight(int h)
    {
        if (h >= 0)
            h = std::max(12, h);

        navigationBar->setDynamicHeight(h);
        homeButton->setDynamicSize(h,h);
        homeButton->setDynamicIconSize(h-2, h-2);
        backwardButton->setDynamicSize(h,h);
        backwardButton->setDynamicIconSize(h-2, h-2);
        forwardButton->setDynamicSize(h,h);
        forwardButton->setDynamicIconSize(h-2, h-2);
    }

    void setSource(QString path)
    {
        if (QFileInfo(path).isFile())
            textViewer->setSource(QUrl::fromLocalFile(path));
    }

    static DocumentationViewer* openDocumentation(QString source)
    {
        DocumentationViewer *viewer = new DocumentationViewer();
        viewer->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(&settings, &Settings::aboutToQuit, viewer, &DocumentationViewer::close);

        viewer->setWindowTitle(DocumentationViewer::tr("Documentation"));
        viewer->setSource(QFileInfo(source).absoluteFilePath());
        viewer->show();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        viewer->setDpiScale(settings.getGlobalDpiScale());
        viewer->setFocus();
        return viewer;
    }

};

#endif
