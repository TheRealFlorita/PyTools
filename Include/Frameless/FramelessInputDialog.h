#ifndef FRAMELESSINPUTDIALOG_H
#define FRAMELESSINPUTDIALOG_H

#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QScreen>

#include <HdLayouts.h>
#include <HdWidgets.h>
#include <FramelessWindowTitleBar.h>
#include <Settings.h>


class FramelessInputDialog : public HdInputDialog
{
Q_OBJECT

protected:
    FramelessWindowTitleBar *titleBar;
    double dpi = 1.0;
    double stepSize = 0.05;
    bool keyScaling = false;

private:
    void init()
    {
        QGridLayout *original = static_cast<QGridLayout*>(layout());

        QBoxLayout *centralLayout = new QBoxLayout(QBoxLayout::TopToBottom);
        centralLayout->setContentsMargins(0,0,0,0);
        centralLayout->setSpacing(0);
        centralLayout->addWidget(titleBar);

        QWidget *centralWiget = new QWidget(this);
        centralLayout->addWidget(centralWiget);
        centralWiget->setLayout(original);
        setLayout(centralLayout);

        setFocusPolicy(Qt::StrongFocus);
    }

public:

    explicit FramelessInputDialog(QWidget *parent = Q_NULLPTR) : HdInputDialog(parent)
    {
        titleBar = new FramelessWindowTitleBar(this, FramelessWindowTitleBar::DIALOG);
        connect(this , &FramelessInputDialog::dpiScaleChanged, titleBar, &FramelessWindowTitleBar::updateDpiScale);

        setDpiScale(settings.getGlobalDpiScale());
    }

    ~FramelessInputDialog() override
    { }

    void resetSize()
    {
        int hscreen = screen()->availableGeometry().height();
        int wscreen = screen()->availableGeometry().width();
        int hwindow = qRound(hscreen/6.0);
        int wwindow = qRound(wscreen/6.0);

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


    FramelessWindowTitleBar* titleBarWidget()
    {
        return titleBar;
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
        HdInputDialog::updateDpiScale(dpi);
    }

    void setEnableKeyScaling(bool enable)
    {
        keyScaling = enable;
    }

protected:

    bool event(QEvent* event) override
    {
        switch(event->type())
        {
        case QEvent::Show:
        {
            init();
            center();
            break;
        }
        case QEvent::KeyPress:
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if ((keyEvent->key() == Qt::Key_Plus) && (keyEvent->modifiers() & Qt::ControlModifier) && keyScaling)
                increaseDpiScale();
            else if((keyEvent->key() == Qt::Key_Minus) && (keyEvent->modifiers() & Qt::ControlModifier) && keyScaling)
                decreaseDpiScale();
            break;
        }
        default:
        {
            break;
        }
        }

        return HdInputDialog::event(event);
    }
};

class FramelessTabBar : public QTabBar, public HiDpiExtensions
{
    Q_OBJECT

private:
    QMenu *menu;
    QAction *actionRename, *actionToggle, *actionNew, *actionCopy, *actionClose;
    bool tabsEditable = true;
    int currentTab = -1;
    int minimumCount = 1;

signals:
    void dpiScaleChanged(double);
    void tabRenamed(int, QString);
    void tabToggled(int, bool);
    void tabCreated(int);
    void tabCopied(int, int);
    void tabClosed(int);
    void tabBarRightClicked(int);

public:
    explicit FramelessTabBar(QWidget *parent = Q_NULLPTR) : QTabBar(parent), HiDpiExtensions(this)
    {
        setDrawBase(false);
        setExpanding(false);
        setMovable(false);

        menu = new QMenu(this);
        menu->setWindowFlag(Qt::NoDropShadowWindowHint);

        actionRename = new QAction(this);
        menu->addAction(actionRename);
        connect(actionRename, &QAction::triggered, this, &FramelessTabBar::renameTab);

        actionToggle = new QAction(this);
        menu->addAction(actionToggle);
        connect(actionToggle, &QAction::triggered, this, &FramelessTabBar::toggleTab);

        actionNew = new QAction(this);
        menu->addAction(actionNew);
        connect(actionNew, &QAction::triggered, this, qOverload<>(&FramelessTabBar::addTab));

        actionCopy = new QAction(this);
        menu->addAction(actionCopy);
        connect(actionCopy, &QAction::triggered, this, &FramelessTabBar::copyTab);

        actionClose = new QAction(this);
        menu->addAction(actionClose);
        connect(actionClose, &QAction::triggered, this, qOverload<>(&FramelessTabBar::removeTab));
    }

    void renameTab()
    {
        if ((currentTab < 0) || !tabsEditable)
            return;

        FramelessInputDialog dlg;
        dlg.setSizeGripEnabled(false);
        dlg.setDpiScale(getDpiScale());
        dlg.setWindowTitle(QTabBar::tr("Rename tab"));
        dlg.setLabelText(QTabBar::tr("Name"));
        dlg.setInputMode(QInputDialog::TextInput);
        dlg.setTextValue(tabText(currentTab));
        dlg.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        if (dlg.exec())
        {
            QString newname = dlg.textValue();
            if (tabText(currentTab) != newname)
            {
                setTabText(currentTab, newname);
                emit tabRenamed(currentTab, newname);
            }
        }
    }

    void toggleTab()
    {
        if ((currentTab < 0) || !tabsEditable)
            return;

        bool enable = !isTabEnabled(currentTab);

        /* Select next enabled tab when disabling current (before disabling current tab!) */
        if (!enable && (currentTab == currentIndex()))
        {
            int closestIndex = currentTab;
            for (int i = 0; i < count(); ++i)
            {
                ++closestIndex;

                if (closestIndex == count())
                    closestIndex = 0;

                if (isTabEnabled(closestIndex))
                {
                    setCurrentIndex(closestIndex);
                    break;
                }
            }

        }

        setTabEnabled(currentTab, enable);

        if (enable && !(currentTab == currentIndex()))
            setCurrentIndex(currentTab);

        emit tabToggled(currentTab, enable);
    }

    int addTab(const QString &text)
    {
        int index = QTabBar::addTab(text);
        emit tabCreated(index);

        return index;
    }

    int addTab()
    {
        if ((currentTab < 0) || !tabsEditable)
            return -1;

        int index = QTabBar::addTab(QString(QTabBar::tr("Tab %1")).arg(count()+1));
        emit tabCreated(index);
        return index;
    }

    int insertTab(int index, const QString &text)
    {
        index = QTabBar::insertTab(index, text);
        emit tabCreated(index);

        return index;
    }

    int insertTab()
    {
        if ((currentTab < 0) || !tabsEditable)
            return -1;

        int index = QTabBar::insertTab(currentTab, QString(QTabBar::tr("Tab %1")).arg(count()+1));
        emit tabCreated(index);

        return index;
    }

    int copyTab()
    {
        if ((currentTab < 0) || !tabsEditable)
            return -1;

        QString subcopy = QTabBar::tr("(copy)").chopped(1);
        QString curname = tabText(currentTab).split(subcopy, Qt::SkipEmptyParts).constFirst();
        subcopy = subcopy.mid(1);

        int q = 1;
        for (int i = 0; i < count(); ++i)
        {
            QString inm = tabText(i);
            if (inm.startsWith(curname))
            {
                QRegularExpression expression(".*\\(" + subcopy + "([\\s]?)([0-9]+)\\)$");
                QRegularExpressionMatch match = expression.match(inm);
                if (match.hasMatch())
                    if (match.capturedLength() >= 3)
                        q = match.captured(2).toInt() + 1;
            }
        }

        int index = QTabBar::addTab(curname + " (" + subcopy + " " + QString::number(q) + ")");
        emit tabCopied(currentTab, index);

        return index;
    }

    void removeTab(int index)
    {
        QTabBar::removeTab(index);
        emit tabClosed(index);
    }

    void removeTab()
    {
        if ((currentTab < 0) || !tabsEditable || (count() == minimumCount))
            return;

        QTabBar::removeTab(currentTab);
        emit tabClosed(currentTab);
    }

    void setTabsEditable(bool enabled)
    {
        tabsEditable = enabled;

        if (tabsEditable)
            connect(this, &FramelessTabBar::tabBarRightClicked, this, &FramelessTabBar::openTabEditMenu, Qt::UniqueConnection);
        else
            disconnect(this, &FramelessTabBar::tabBarRightClicked, this, &FramelessTabBar::openTabEditMenu);
    }

    bool getTabsEditable()
    {
        return tabsEditable;
    }

    void setMinimumTabCount(int minimum)
    {
        minimumCount = minimum;
    }

    int enabledTabsCount()
    {
        int n = 0;

        for (int i = 0; i < count(); ++i)
            if (this->isTabEnabled(i))
                ++n;
        return n;
    }

protected:

    void mousePressEvent(QMouseEvent *event) override
    {
        if(event->button() == Qt::RightButton)
            emit tabBarRightClicked(tabAt(event->pos()));
        else
            QTabBar::mousePressEvent(event);
    }

public slots:

    void openTabEditMenu(int index)
    {
        if ((index < 0) || !tabsEditable)
        {
            currentTab = -1;
            return;
        }

        actionRename->setText(QTabBar::tr("Rename"));
        actionNew->setText(QTabBar::tr("New"));
        actionCopy->setText(QTabBar::tr("Copy"));
        actionClose->setText(QTabBar::tr("Close"));

        currentTab = index;
        if (isTabEnabled(currentTab))
            actionToggle->setText(QTabBar::tr("Disable"));
        else
            actionToggle->setText(QTabBar::tr("Enable"));

        actionToggle->setEnabled( (enabledTabsCount() > minimumCount) || !isTabEnabled(currentTab));
        actionClose->setEnabled( (enabledTabsCount() > minimumCount) || (!isTabEnabled(currentTab) && (count() > minimumCount)) );

        QPoint gpos = QCursor::pos();
        menu->move(gpos.x(), gpos.y() + 1);
        menu->show();
        menu->setFocus();
    }

    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};

#endif
