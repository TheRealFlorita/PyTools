#ifndef HDWIDGETS_H
#define HDWIDGETS_H

#include <QApplication>
#include <QAbstractButton>
#include <QBoxLayout>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCursor>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollBar>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyleOption>
#include <QTabBar>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>
#include <QWindow>


class MouseWheelWidgetAdjustmentGuard : public QObject
{

public:
    explicit MouseWheelWidgetAdjustmentGuard(QObject *parent) : QObject(parent)
    {}

protected:
    bool eventFilter(QObject *o, QEvent *e) override
    {
        const QWidget* widget = static_cast<QWidget*>(o);
        if (e->type() == QEvent::Wheel && widget && !widget->hasFocus())
        {
            e->ignore();
            return true;
        }

        return QObject::eventFilter(o, e);
    }
};


class HiDpiExtensions
{

private:
    bool bHasDynamicHeight = false;
    bool bHasDynamicWidth = false;
    bool bUseSizeHintHeight = false;
    bool bUseSizeHintWidth = false;
    int  dynamicHeight = 0;
    int  dynamicWidth = 0;
    double dpi = 1.0;
    QWidget *_widget = Q_NULLPTR;

public:
    explicit HiDpiExtensions(QWidget *parent)
    {
        _widget = parent;
        _widget->setAttribute(Qt::WA_Hover, true);
        _widget->setContentsMargins(0,0,0,0);
        _widget->setAutoFillBackground(true);
    }

    QWidget* hdWidget()
    {
        return _widget;
    }

    double getDpiScale()
    {
        return dpi;
    }

    bool setDpiScale(double scale)
    {
        if (scale > 5e-2)
        {
            dpi = scale;
            return true;
        }
        return false;
    }

    void setDynamicWidth(int w)
    {
        if (w>0)
        {
            bHasDynamicWidth = true;
            bUseSizeHintWidth = false;
            dynamicWidth = w;
            _widget->setFixedWidth( std::max(1,int(dpi*dynamicWidth)) );
        }
        else
        {
            bHasDynamicWidth = false;
            bUseSizeHintWidth = false;
            _widget->setMinimumWidth(0);
            _widget->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    }

    bool hasDynamicWidth()
    {
        return bHasDynamicWidth;
    }

    int getDynamicWidth()
    {
        return dynamicWidth;
    }

    void setDynamicHeight(int h)
    {
        if (h>0)
        {
            bHasDynamicHeight = true;
            bUseSizeHintHeight = false;
            dynamicHeight = h;
            _widget->setFixedHeight( std::max(1,int(dpi*dynamicHeight)) );
        }
        else
        {
            bHasDynamicHeight = false;
            bUseSizeHintHeight = false;
            _widget->setMinimumHeight(0);
            _widget->setMaximumHeight(QWIDGETSIZE_MAX);
        }
    }

    bool hasDynamicHeight()
    {
        return bHasDynamicHeight;
    }

    int getDynamicHeight()
    {
        return dynamicHeight;
    }

    void setDynamicSize(int w, int h)
    {
        setDynamicWidth(w);
        setDynamicHeight(h);
    }

    void setUseSizeHintWidth(bool enable)
    {
        bUseSizeHintWidth = enable;

        if (enable)
            bHasDynamicWidth = false;
    }

    bool usesSizeHintWidth()
    {
        return bUseSizeHintWidth;
    }

    void setUseSizeHintHeight(bool enable)
    {
        bUseSizeHintHeight = enable;

        if (enable)
            bHasDynamicHeight = false;
    }

    bool usesSizeHintHeight()
    {
        return bUseSizeHintHeight;
    }

    void setUseSizeHint(bool enable)
    {
        bUseSizeHintHeight = enable;
        bUseSizeHintWidth = enable;

        if (enable)
        {
            bHasDynamicHeight = false;
            bHasDynamicWidth = false;
        }
    }

    void updateScaling()
    {
        if (bHasDynamicWidth)
            _widget->setFixedWidth( std::max(1,int(dpi*dynamicWidth)) );
        else if (bUseSizeHintWidth)
            _widget->setFixedWidth( _widget->sizeHint().width() );

        if (bHasDynamicHeight)
            _widget->setFixedHeight( std::max(1,int(dpi*dynamicHeight)) );
        else if (bUseSizeHintHeight)
            _widget->setFixedHeight( _widget->sizeHint().height() );
    }

};


class HdWidget : public QWidget, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdWidget(QWidget *parent = Q_NULLPTR) : QWidget(parent), HiDpiExtensions(this)
    {
        setAttribute(Qt::WA_Hover, true);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QStyleOption opt;
        opt.initFrom(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        QWidget::paintEvent(event);
    }

};


class HdAbstractButton : public QAbstractButton, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdAbstractButton(QWidget *parent = Q_NULLPTR) : QAbstractButton(parent), HiDpiExtensions(this)
    {
        setAttribute(Qt::WA_Hover,true);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdCheckBox : public QCheckBox, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdCheckBox(QWidget *parent = Q_NULLPTR) : QCheckBox(parent), HiDpiExtensions(this)
    { }

    explicit HdCheckBox(QString text, QWidget *parent = Q_NULLPTR) : QCheckBox(parent), HiDpiExtensions(this)
    {
        setText(text);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdComboBox : public QComboBox, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdComboBox(QWidget *parent = Q_NULLPTR) : QComboBox(parent), HiDpiExtensions(this)
    {
        setFocusPolicy(Qt::StrongFocus);
        installEventFilter(new MouseWheelWidgetAdjustmentGuard(this));
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdDialog : public QDialog, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdDialog(QWidget *parent = Q_NULLPTR) : QDialog(parent), HiDpiExtensions(this)
    { }


public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};



class HdDockWidget : public QDockWidget, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdDockWidget(QWidget *parent = Q_NULLPTR) : QDockWidget(parent), HiDpiExtensions(this)
    {
        setAttribute(Qt::WA_Hover);
        setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        setFloating(false);
        setFocusPolicy(Qt::StrongFocus);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdDoubleSpinBox : public QDoubleSpinBox, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdDoubleSpinBox(QWidget *parent = Q_NULLPTR) : QDoubleSpinBox(parent), HiDpiExtensions(this)
    {
        setFocusPolicy(Qt::StrongFocus);
        installEventFilter(new MouseWheelWidgetAdjustmentGuard(this));
        setRange(0.00, 99.99);
        setDecimals(2);
        setAutoFillBackground(false);
    }

    explicit HdDoubleSpinBox(int value, QWidget *parent = Q_NULLPTR) : QDoubleSpinBox(parent), HiDpiExtensions(this)
    {
        setFocusPolicy(Qt::StrongFocus);
        installEventFilter(new MouseWheelWidgetAdjustmentGuard(this));
        setRange(std::min(value, 0), std::max(value, 9999));
        setValue(value);
        setAutoFillBackground(false);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdFileDialog : public QFileDialog, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdFileDialog(QWidget *parent = Q_NULLPTR, QString caption = QFileDialog::tr("File Dialog")) : QFileDialog(parent, caption), HiDpiExtensions(this)
    {
        setOptions(QFileDialog::DontUseNativeDialog | QFileDialog::DontUseCustomDirectoryIcons);
        setSizeGripEnabled(false);
        setWindowTitle(QFileDialog::tr("File Dialog"));

        QScreen *screen;
        if (parent == Q_NULLPTR)
            screen = QApplication::primaryScreen();
        else
        {
            screen = parent->window()->windowHandle()->screen();
            connect(parent->window()->windowHandle(), &QWindow::screenChanged, this, [this](QScreen* screen) {
                int w = screen->availableGeometry().width()/2;
                int h = screen->availableGeometry().height()/2;
                resize(w,h); });
        }

        int w = screen->availableGeometry().width()/2;
        int h = screen->availableGeometry().height()/2;
        resize(w,h);
    }

    void setNameFilter(const QString &filter)
    {
        QString defaultsuffix;

        static QRegularExpression expression;
        expression.setPattern("\\((.*)\\)");
        QRegularExpressionMatch match = expression.match(filter);

        if(match.hasMatch())
        {
            QStringList exts = match.captured(1).remove("*").remove(",").remove(".").remove(";").split(" ", Qt::SkipEmptyParts);
            if (exts.length() > 0)
                defaultsuffix = exts.first();
        }

        if (!defaultsuffix.isEmpty())
            setDefaultSuffix(defaultsuffix);

        QFileDialog::setNameFilter(filter);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdGroupBox : public QGroupBox, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdGroupBox(QWidget *parent = Q_NULLPTR) : QGroupBox(parent), HiDpiExtensions(this)
    {
        setCheckable(false);
    }

    explicit HdGroupBox(QString title, QWidget *parent = Q_NULLPTR) : QGroupBox(title, parent), HiDpiExtensions(this)
    {
        setCheckable(false);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdInputDialog : public QInputDialog, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdInputDialog(QWidget *parent = Q_NULLPTR) : QInputDialog(parent), HiDpiExtensions(this)
    {
        setSizeGripEnabled(true);

        QScreen *screen = this->screen();
        if (parent != Q_NULLPTR)
        {
            screen = parent->window()->windowHandle()->screen();
            connect(parent->window()->windowHandle(), &QWindow::screenChanged, this, [this](QScreen* screen) {
                int w = screen->availableGeometry().width()/6;
                int h = screen->availableGeometry().height()/6;
                resize(w,h); });
        }

        int w = screen->availableGeometry().width()/6;
        int h = screen->availableGeometry().height()/6;
        resize(w,h);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdLabel : public QLabel, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);


public:
    explicit HdLabel(QWidget *parent = Q_NULLPTR) : QLabel(parent), HiDpiExtensions(this)
    {
        setIndent(0);
    }

    HdLabel(QString text, QWidget *parent = Q_NULLPTR) : QLabel(parent), HiDpiExtensions(this)
    {
        setIndent(0);
        setText(text);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdLineEdit : public QLineEdit, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdLineEdit(QWidget *parent = Q_NULLPTR) : QLineEdit(parent), HiDpiExtensions(this)
    { }

    HdLineEdit(QString text, QWidget *parent = Q_NULLPTR) : QLineEdit(parent), HiDpiExtensions(this)
    {
        setText(text);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdMainWindow : public QMainWindow, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdMainWindow(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags()) : QMainWindow(parent, flags), HiDpiExtensions(this)
    {
        setWindowFlags(flags);
        setAttribute(Qt::WA_Hover, true);
        setAttribute(Qt::WA_StyledBackground, true);
        setFocusPolicy(Qt::StrongFocus);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdMenuBar : public QMenuBar, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdMenuBar(QWidget *parent = Q_NULLPTR) : QMenuBar(parent), HiDpiExtensions(this)
    { }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdMessageBox : public QMessageBox, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdMessageBox(QWidget *parent = Q_NULLPTR) : QMessageBox(parent), HiDpiExtensions(this)
    { }

    explicit HdMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text, QMessageBox::StandardButtons buttons = NoButton, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::Dialog) : QMessageBox(icon, title, text, buttons, parent, flags), HiDpiExtensions(this)
    { }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdProgressBar : public QProgressBar, public HiDpiExtensions
{
    Q_OBJECT

private:
    HdLabel *time;
    QElapsedTimer timer;
    bool isrunning = false;

signals:
    void dpiScaleChanged(double);

public:
    explicit HdProgressBar(QWidget *parent = Q_NULLPTR) : QProgressBar(parent), HiDpiExtensions(this)
    {
        setTextVisible(false);

        QBoxLayout *lyt = new QBoxLayout(QBoxLayout::LeftToRight, this);
        lyt->setContentsMargins(0,0,0,0);
        lyt->setSpacing(0);
        this->setLayout(lyt);
        time = new HdLabel(this);
        lyt->addWidget(time, 0, Qt::AlignCenter);
        timer.start();
    }

    void reset()
    {
        stopTimer();
        QProgressBar::reset();
    }

    void setRange(int min, int max)
    {
        if (min == 0 && max == 0)
            startTimer();
        else
            stopTimer();

        QProgressBar::setRange(min, max);
    }

    void startTimer()
    {
        isrunning = true;
        timer.restart();
    }

    void stopTimer()
    {
        isrunning = false;
        time->setText("");
    }

    void paintEvent(QPaintEvent *event) override
    {
        if (isrunning)
            time->setText(QDateTime::fromMSecsSinceEpoch(timer.elapsed()).toUTC().toString("hh:mm:ss"));

        QProgressBar::paintEvent(event);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdPushButton : public QPushButton, public HiDpiExtensions
{
    Q_OBJECT

private:
    bool hasDynamicIconSize = false;
    int  dynamicIconHeight = 0;
    int  dynamicIconWidth = 0;

signals:
    void dpiScaleChanged(double);

public:
    explicit HdPushButton(QWidget *parent = Q_NULLPTR) : QPushButton(parent), HiDpiExtensions(this)
    { }

    void setDynamicIconSize(int w, int h)
    {
        if (h > 0 && w > 0)
        {
            hasDynamicIconSize = true;
            dynamicIconWidth = w;
            dynamicIconHeight = h;
            setIconSize(QSize(std::max(1,int(getDpiScale()*dynamicIconWidth)),
                              std::max(1,int(getDpiScale()*dynamicIconHeight)) ));
        }
        else
        {
            hasDynamicIconSize = false;
            setIconSize(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
        }
    }

public slots:

    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();

        if (hasDynamicIconSize)
            setIconSize(QSize(std::max(1,int(getDpiScale()*dynamicIconHeight)),
                              std::max(1,int(getDpiScale()*dynamicIconWidth)) ));
    }
};


class HdSpinBox : public QSpinBox, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdSpinBox(QWidget *parent = Q_NULLPTR) : QSpinBox(parent), HiDpiExtensions(this)
    {
        setFocusPolicy(Qt::StrongFocus);
        installEventFilter(new MouseWheelWidgetAdjustmentGuard(this));
        setRange(0, 9999);
        setAutoFillBackground(false);
    }

    explicit HdSpinBox(int value, QWidget *parent = Q_NULLPTR) : QSpinBox(parent), HiDpiExtensions(this)
    {
        setFocusPolicy(Qt::StrongFocus);
        installEventFilter(new MouseWheelWidgetAdjustmentGuard(this));
        setRange(std::min(value, 0), std::max(value, 9999));
        setValue(value);
        setAutoFillBackground(false);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdStackedWidget : public QStackedWidget, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdStackedWidget(QWidget *parent = Q_NULLPTR) : QStackedWidget(parent), HiDpiExtensions(this)
    { }

    void removeWidget(QWidget *widget)
    {
        QStackedWidget::removeWidget(widget);
        widget->deleteLater();
    }

    void removeWidget(int index)
    {
        if (index < count())
        {
            QWidget *wdg = QStackedWidget::widget(index);
            QStackedWidget::removeWidget(wdg);
            wdg->deleteLater();
        }
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdStatusBar : public QStatusBar, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdStatusBar(QWidget *parent = Q_NULLPTR) : QStatusBar(parent), HiDpiExtensions(this)
    {
        setContentsMargins(0,0,0,0);
        setSizeGripEnabled(false);
    }

    void showMessage(QString text, int timeout=15000)
    {
        QStatusBar::showMessage(text, timeout);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdTabBar : public QTabBar, public HiDpiExtensions
{
    Q_OBJECT

private:
    QMenu *menu;
    QAction *actionRename, *actionToggle, *actionNew, *actionCopy, *actionClose;
    bool _tabsEditable = true;
    int _currentIndex = -1;
    int _minimumCount = 1;

signals:
    void dpiScaleChanged(double);
    void tabRenamed(int, QString);
    void tabToggled(int, bool);
    void tabCreated(int);
    void tabCopied(int, int);
    void tabClosed(int);
    void tabBarRightClicked(int);

public:
    explicit HdTabBar(QWidget *parent = Q_NULLPTR) : QTabBar(parent), HiDpiExtensions(this)
    {
        setDrawBase(false);
        setExpanding(false);
        setMovable(false);

        menu = new QMenu(this);

        actionRename = new QAction(this);
        menu->addAction(actionRename);
        connect(actionRename, &QAction::triggered, this, &HdTabBar::renameTab);

        actionToggle = new QAction(this);
        menu->addAction(actionToggle);
        connect(actionToggle, &QAction::triggered, this, &HdTabBar::toggleTab);

        actionNew = new QAction(this);
        menu->addAction(actionNew);
        connect(actionNew, &QAction::triggered, this, qOverload<>(&HdTabBar::addTab));

        actionCopy = new QAction(this);
        menu->addAction(actionCopy);
        connect(actionCopy, &QAction::triggered, this, &HdTabBar::copyTab);

        actionClose = new QAction(this);
        menu->addAction(actionClose);
        connect(actionClose, &QAction::triggered, this, qOverload<>(&HdTabBar::removeTab));
    }

    void renameTab()
    {
        if ((_currentIndex < 0) || !_tabsEditable)
            return;

        HdInputDialog dlg(this);
        dlg.setWindowTitle(QTabBar::tr("Rename tab"));
        dlg.setLabelText(QTabBar::tr("Name"));
        dlg.setInputMode(QInputDialog::TextInput);
        dlg.setTextValue(tabText(_currentIndex));

        if (dlg.exec())
        {
            QString newname = dlg.textValue();
            setTabText(_currentIndex, newname);
            emit tabRenamed(_currentIndex, newname);
        }
    }

    void toggleTab()
    {
        if ((_currentIndex < 0) || !_tabsEditable)
            return;

        bool enable = !isTabEnabled(_currentIndex);

        /* Select next enabled tab when disabling current (before disabling current tab!) */
        if (!enable && (_currentIndex == currentIndex()))
        {
            int closestIndex = _currentIndex;
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

        setTabEnabled(_currentIndex, enable);

        if (enable && !(_currentIndex == currentIndex()))
            setCurrentIndex(_currentIndex);

        emit tabToggled(_currentIndex, enable);
    }

    int addTab(const QString &text)
    {
        int index = QTabBar::addTab(text);
        emit tabCreated(index);

        return index;
    }

    int addTab()
    {
        if ((_currentIndex < 0) || !_tabsEditable)
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
        if ((_currentIndex < 0) || !_tabsEditable)
            return -1;

        int index = QTabBar::insertTab(_currentIndex, QString(QTabBar::tr("Tab %1")).arg(count()+1));
        emit tabCreated(index);

        return index;
    }

    int copyTab()
    {
        if ((_currentIndex < 0) || !_tabsEditable)
            return -1;

        QString subcopy = QTabBar::tr("(copy)").chopped(1);
        QString curname = tabText(_currentIndex).split(subcopy, Qt::SkipEmptyParts).constFirst();
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
        emit tabCopied(_currentIndex, index);

        return index;
    }

    void removeTab(int index)
    {
        QTabBar::removeTab(index);
        emit tabClosed(index);
    }

    void removeTab()
    {
        if ((_currentIndex < 0) || !_tabsEditable || (count() == _minimumCount))
            return;

        QTabBar::removeTab(_currentIndex);
        emit tabClosed(_currentIndex);
    }

    void setTabsEditable(bool enabled)
    {
        _tabsEditable = enabled;

        if (_tabsEditable)
            connect(this, &HdTabBar::tabBarRightClicked, this, &HdTabBar::openTabEditMenu, Qt::UniqueConnection);
        else
            disconnect(this, &HdTabBar::tabBarRightClicked, this, &HdTabBar::openTabEditMenu);
    }

    bool tabsEditable()
    {
        return _tabsEditable;
    }

    void setMinimumTabCount(int minimum)
    {
        _minimumCount = minimum;
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
        if ((index < 0) || !_tabsEditable)
        {
            _currentIndex = -1;
            return;
        }

        actionRename->setText(QTabBar::tr("Rename"));
        actionNew->setText(QTabBar::tr("New"));
        actionCopy->setText(QTabBar::tr("Copy"));
        actionClose->setText(QTabBar::tr("Close"));

        _currentIndex = index;
        if (isTabEnabled(_currentIndex))
            actionToggle->setText(QTabBar::tr("Disable"));
        else
            actionToggle->setText(QTabBar::tr("Enable"));

        actionToggle->setEnabled( (enabledTabsCount() > _minimumCount) || !isTabEnabled(_currentIndex));
        actionClose->setEnabled( (enabledTabsCount() > _minimumCount) || (!isTabEnabled(_currentIndex) && (count() > _minimumCount)) );

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


class HdTableWidget : public QTableWidget, public HiDpiExtensions
{
    Q_OBJECT

protected:
    bool hasDynamicRowHeight = false;
    bool hasDynamicColumnWidth = false;
    bool hasDynamicMinimumColumnWidth = false;
    int  dynamicRowHeight = 0;
    int  dynamicColumnWidth = 0;
    int  dynamicMinimumColumnWidth = 0;

    bool rowsResizable = true;
    bool columnsResizable = true;
    bool useViewportSizeHintHeight = false;
    bool useViewportSizeHintWidth = false;

signals:
    void dpiScaleChanged(double);
    void rowInserted(int);
    void columnInserted(int);
    void rowRemoved(int);
    void columnRemoved(int);

public:

    explicit HdTableWidget(QWidget *parent = Q_NULLPTR) : QTableWidget(parent), HiDpiExtensions(this)
    {
        setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::AnyKeyPressed);

        horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        setAlternatingRowColors(false);
        setShowGrid(true);
    }

    explicit HdTableWidget(int rows, int columns, QWidget *parent = Q_NULLPTR) : QTableWidget(rows, columns, parent), HiDpiExtensions(this)
    {
        setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::AnyKeyPressed);

        horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        setAlternatingRowColors(false);
        setShowGrid(true);
    }

    void setDynamicColumnWidth(int w)
    {
        if (w>0)
        {
            hasDynamicColumnWidth = true;
            hasDynamicMinimumColumnWidth = false;
            dynamicColumnWidth = w;
            horizontalHeader()->setDefaultSectionSize( std::max(1, int(getDpiScale()*dynamicColumnWidth)) );
        }
        else
        {
            hasDynamicColumnWidth = false;
        }
    }

    void setDynamicMinimumColumnWidth(int w)
    {
        if (w>0)
        {
            hasDynamicColumnWidth = false;
            hasDynamicMinimumColumnWidth = true;
            dynamicMinimumColumnWidth = w;
            QTableWidget::resizeColumnsToContents();

            for (int i = 0; i < columnCount(); ++i)
                if (horizontalHeader()->sectionSize(i) < std::max(1, int(getDpiScale()*dynamicMinimumColumnWidth)) )
                    horizontalHeader()->resizeSection(i, std::max(1, int(getDpiScale()*dynamicMinimumColumnWidth)) );
        }
        else
        {
            hasDynamicMinimumColumnWidth = false;
        }
    }

    void setDynamicRowHeight(int h)
    {
        if (h>0)
        {
            hasDynamicRowHeight = true;
            dynamicRowHeight = h;
            horizontalHeader()->setFixedHeight( std::max(1,int(getDpiScale()*dynamicRowHeight)) );
            verticalHeader()->setDefaultSectionSize( std::max(1,int(getDpiScale()*dynamicRowHeight)) );
        }
        else
        {
            hasDynamicRowHeight = false;
        }
    }

    void setUseViewportSizeHintHeight(bool enable)
    {
        useViewportSizeHintHeight = enable;

        if (enable)
        {
            setUseSizeHintHeight(false);
            setDynamicHeight(-1);

            int h = 0;
            if (horizontalScrollBar()->isVisible())
                h = horizontalScrollBar()->sizeHint().height();
            setFixedHeight(viewportSizeHint().height() + h);
        }
        else
        {
            setMinimumHeight(0);
            setMaximumHeight(QWIDGETSIZE_MAX);
        }
    }

    void setUseViewportSizeHintWidth(bool enable)
    {
        useViewportSizeHintWidth = enable;

        if (enable)
        {
            setUseSizeHintWidth(false);
            setDynamicWidth(-1);

            int w = 0;
            if (verticalScrollBar()->isVisible())
                w = verticalScrollBar()->sizeHint().width();
            setFixedWidth(viewportSizeHint().width() + w);

        }
        else
        {
            setMinimumWidth(0);
            setMaximumWidth(QWIDGETSIZE_MAX);
        }
    }

    void setUseViewportSizeHint(bool enable)
    {
        setUseViewportSizeHintHeight(enable);
        setUseViewportSizeHintWidth(enable);
    }

    void setRowsResizable(bool resizable)
    {
        rowsResizable = resizable;
    }

    bool isRowsResizable()
    {
        return rowsResizable;
    }

    void setColumnsResizable(bool resizable)
    {
        columnsResizable = resizable;
    }

    bool isColumnsResizable()
    {
        return columnsResizable;
    }

    void setResizable(bool rows, bool columns)
    {
        rowsResizable = rows;
        columnsResizable = columns;
    }

    QPair<bool, bool> isResizable()
    {
        return QPair<bool, bool>(rowsResizable, columnsResizable);
    }

    void setItemText(int row, int col, const QString &text)
    {
        if ((row >= rowCount() && !rowsResizable) || (col >= columnCount() && !columnsResizable))
            return;
        else if (row >= rowCount())
            setRowCount(row+1);
        else if (col >= columnCount())
            setColumnCount(col+1);

        if (item(row, col) != Q_NULLPTR)
        {
            if (!(item(row, col)->flags() & Qt::ItemIsEditable))
                return;
            else if (text.isEmpty())
                setItem(row, col, Q_NULLPTR);
            else
                item(row, col)->setText(text);
        }
        else if (!text.isEmpty())
            setItem(row, col, new QTableWidgetItem(text));
    }

    void insertRow(int row)
    {
        if (!rowsResizable)
            return;

        row = std::min(row, rowCount());
        QTableWidget::insertRow(row);
        emit rowInserted(row);
    }

    void insertColumn(int column)
    {
        if (!columnsResizable)
            return;

        column = std::min(column, columnCount());
        QTableWidget::insertColumn(column);
        emit columnInserted(column);
    }

    void removeRow(int row)
    {
        if (!rowsResizable)
            return;

        QTableWidget::removeRow(row);
        emit rowRemoved(row);
    }

    void removeColumn(int column)
    {
        if (!columnsResizable)
            return;

        QTableWidget::removeColumn(column);
        emit columnRemoved(column);
    }

public slots:

    void resizeColumnsToContents()
    {
        if (hasDynamicColumnWidth)
            return;

        QTableWidget::resizeColumnsToContents();

        if (hasDynamicMinimumColumnWidth)
        {
            for (int i = 0; i < columnCount(); ++i)
                if (horizontalHeader()->sectionSize(i) < std::max(1, int(getDpiScale()*dynamicMinimumColumnWidth)) )
                    horizontalHeader()->resizeSection(i, std::max(1, int(getDpiScale()*dynamicMinimumColumnWidth)) );
        }
    }

    void updateViewportSizeHintHeight()
    {
        if (useViewportSizeHintHeight)
        {
            int h = 0;
            if (horizontalScrollBar()->isVisible())
                h = horizontalScrollBar()->sizeHint().height();
            setFixedHeight(viewportSizeHint().height() + h);
        }
    }

    void updateViewportSizeHintWidth()
    {
        if (useViewportSizeHintWidth)
        {
            int w = 0;
            if (verticalScrollBar()->isVisible())
                w = verticalScrollBar()->sizeHint().width();
            setFixedWidth(viewportSizeHint().width() + w);
        }
    }

    void updateViewportSizeHint()
    {
        updateViewportSizeHintHeight();
        updateViewportSizeHintWidth();
    }

    void updateViewport()
    {
        resizeColumnsToContents();
        updateViewportSizeHint();
    }

    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();

        if (hasDynamicColumnWidth)
            horizontalHeader()->setDefaultSectionSize(std::max(1, int(getDpiScale()*dynamicColumnWidth)) );

        if (hasDynamicRowHeight)
        {
            horizontalHeader()->setFixedHeight( std::max(1, int(getDpiScale()*dynamicRowHeight)) );
            verticalHeader()->setDefaultSectionSize( std::max(1, int(getDpiScale()*dynamicRowHeight)) );
        }

        if (hasDynamicMinimumColumnWidth)
        {
            for (int i = 0; i < columnCount(); ++i)
                if (horizontalHeader()->sectionSize(i) < std::max(1, int(getDpiScale()*dynamicMinimumColumnWidth)) )
                    horizontalHeader()->resizeSection(i, std::max(1, int(getDpiScale()*dynamicMinimumColumnWidth)) );
        }

        updateViewportSizeHint();
    }

protected:

    void insert()
    {
        if (!selectionModel()->hasSelection())
            return;

        QModelIndexList selected_rows = selectionModel()->selectedRows();
        QModelIndexList selected_cols = selectionModel()->selectedColumns();

        if(!selected_rows.isEmpty() && rowsResizable)
        {
            for (int i = 0; i < selected_rows.length(); ++i)
                insertRow(selected_rows.first().row());

            updateViewportSizeHintHeight();
        }

        else if(!selected_cols.isEmpty() && columnsResizable)
        {
            for (int j = 0; j < selected_cols.length(); ++j)
                insertColumn(selected_cols.first().column());

            updateViewportSizeHintWidth();
        }
    }

    void append()
    {
        if (!selectionModel()->hasSelection())
            return;

        QModelIndexList selected_rows = selectionModel()->selectedRows();
        QModelIndexList selected_cols = selectionModel()->selectedColumns();

        if(!selected_rows.isEmpty() && rowsResizable)
        {
            for (int i = 0; i < selected_rows.length(); ++i)
                insertRow(selected_rows.last().row()+1);

            updateViewportSizeHintHeight();
        }

        else if(!selected_cols.isEmpty() && columnsResizable)
        {
            for (int j = 0; j < selected_cols.length(); ++j)
                insertColumn(selected_cols.last().column()+1);

            updateViewportSizeHintWidth();
        }
    }

    void del()
    {
        if (!selectionModel()->hasSelection())
            return;

        QModelIndexList selected_rows = selectionModel()->selectedRows();
        QModelIndexList selected_cols = selectionModel()->selectedColumns();

        /* Delete selected rows */
        if (!selected_rows.isEmpty() && rowsResizable)
        {
            for (int n = 0; n < selected_rows.length(); ++n)
            {
                int i = selected_rows.at(n).row();
                for (int j = 0; j < rowCount(); ++j)
                    if (item(i,j) != Q_NULLPTR)
                        if (!(item(i,j)->flags() & Qt::ItemIsEditable))
                            return;
            }

            for (int n = 0; n < selected_rows.length(); ++n)
                removeRow(selected_rows.first().row());

            updateViewportSizeHintHeight();
        }
        /* Or delete selected columns */
        else if (!selected_cols.isEmpty() && columnsResizable)
        {
            for (int n = 0; n < selected_cols.length(); ++n)
            {
                int j = selected_cols.at(n).column();
                for (int i = 0; i < columnCount(); ++i)
                    if (item(i,j) != Q_NULLPTR)
                        if (!(item(i,j)->flags() & Qt::ItemIsEditable))
                            return;
            }

            for (int n = 0; n < selected_cols.length(); ++n)
                removeColumn(selected_cols.first().column());

            updateViewportSizeHintWidth();
        }
        /* Or clear selected cells */
        else
        {
            QTableWidgetSelectionRange start = selectedRanges().constFirst();
            QTableWidgetSelectionRange stop = selectedRanges().constLast();

            for (int i = start.topRow(); i <= stop.bottomRow(); ++i)
                for (int j = start.leftColumn(); j <= stop.rightColumn(); ++j)
                    if (item(i,j) != Q_NULLPTR)
                        if (item(i,j)->flags() & Qt::ItemIsEditable)
                            setItem(i,j,Q_NULLPTR);

            updateViewportSizeHint();
        }
    }

    void copy()
    {
        if (!selectionModel()->hasSelection())
            return;

        QTableWidgetSelectionRange start = selectedRanges().constFirst();
        QTableWidgetSelectionRange stop = selectedRanges().constLast();
        QString text;

        for (int i = start.topRow(); i <= stop.bottomRow(); ++i)
        {
            if (i > start.topRow())
                text += "\n";

            for (int j = start.leftColumn(); j <= stop.rightColumn(); ++j)
            {
                if (j > start.leftColumn())
                    text += "\t";

                if (item(i,j) != Q_NULLPTR)
                    text += this->item(i,j)->text().replace("\t","  ");
            }
        }

        QApplication::clipboard()->setText(text);
    }

    void cut()
    {
        if (!selectionModel()->hasSelection())
            return;

        QTableWidgetSelectionRange start = selectedRanges().constFirst();
        QTableWidgetSelectionRange stop = selectedRanges().constLast();
        QString text;

        for (int i = start.topRow(); i <= stop.bottomRow(); ++i)
        {
            if (i > start.topRow())
                text += "\n";

            for (int j = start.leftColumn(); j <= stop.rightColumn(); ++j)
            {
                if (j > start.leftColumn())
                    text += "\t";

                if (item(i,j) != Q_NULLPTR)
                {
                    text += item(i,j)->text().replace("\t","  ");

                    if (item(i,j)->flags() & Qt::ItemIsEditable)
                        setItem(i,j,Q_NULLPTR);
                }
            }
        }

        QApplication::clipboard()->setText(text);
        updateViewportSizeHint();
    }

    void paste()
    {
        if (!selectionModel()->hasSelection())
            return;

        QTableWidgetSelectionRange start = selectedRanges().constFirst();
        QTableWidgetSelectionRange stop = selectedRanges().constLast();

        int top = start.topRow();
        int bottom = stop.bottomRow();
        int left = start.leftColumn();
        int right = stop.rightColumn();

        QString clipboardtext = QApplication::clipboard()->text();

        while (clipboardtext.startsWith("\r") || clipboardtext.startsWith("\n"))
            clipboardtext.remove(0, 1);

        while (clipboardtext.endsWith("\r") || clipboardtext.endsWith("\n"))
            clipboardtext.remove(clipboardtext.length()-1, 1);

        QStringList rowContents = clipboardtext.split("\n");

        int j_ext = std::max(0, bottom - top - int(rowContents.size()) + 1);
        for (int e = 0; e < j_ext; ++e)
            rowContents.append(rowContents[e]);

        for (int i = 0; i < rowContents.size(); ++i)
        {
            QStringList columnContents = rowContents[i].split("\t");

            int i_ext = std::max(0, right - left - int(columnContents.size()) + 1);
            for (int e = 0; e < i_ext; ++e)
                columnContents.append(columnContents[e]);

            for (int j = 0; j < columnContents.size(); ++j)
                setItemText(top+i,left+j,columnContents[j]);
        }

        updateViewportSizeHint();
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Z && (event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier))
            updateViewport();
        else if (event->key() == Qt::Key_Insert && !(event->modifiers() & Qt::ShiftModifier))
            insert();
        else if (event->key() == Qt::Key_Insert && event->modifiers() == Qt::ShiftModifier)
            append();
        else if (event->key() == Qt::Key_Delete)
            del();
        else if (event->key() == Qt::Key_X && event->modifiers() == Qt::ControlModifier)
            cut();
        else if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier)
            copy();
        else if (event->key() == Qt::Key_V && event->modifiers() == Qt::ControlModifier)
            paste();
        else if (event->key() == Qt::Key_Escape)
            clearSelection();
        else if (event->key() == Qt::Key_D && event->modifiers() == Qt::ShiftModifier)
            clearSelection();
        else
            QTableWidget::keyPressEvent(event);
    }
};


class HdTextBrowser : public QTextBrowser, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdTextBrowser(QWidget *parent = Q_NULLPTR) : QTextBrowser(parent), HiDpiExtensions(this)
    {
        setMouseTracking(true);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        setContentsMargins(0,0,0,0);

        /* Set tab width */
        ensurePolished();
        QFontMetrics metrics(font());
        setTabStopDistance(2*metrics.horizontalAdvance(" "));

        connect(this, &HdTextBrowser::anchorClicked, this, &HdTextBrowser::onAnchorClicked);

        setReadOnly(true);
        setOpenLinks(false);
        setOpenExternalLinks(false);
    }

private:

    void resizeEvent(QResizeEvent* event) override
    {
        QTextBrowser::resizeEvent(event);
        updateDpiScale(getDpiScale());
    }

public slots:
    void onAnchorClicked(const QUrl &link)
    {
        if (link.scheme().contains("http"))
            QDesktopServices::openUrl(link);
        else
            setSource(link);
    }

    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        if (hasDynamicWidth())
            setFixedWidth( std::max(1,int(getDpiScale()*getDynamicWidth())) );
        else if (usesSizeHintWidth())
            setFixedWidth(int(document()->size().width()));

        if (hasDynamicHeight())
            setFixedHeight( std::max(1,int(getDpiScale()*getDynamicHeight())) );
        else if (usesSizeHintHeight())
            setFixedHeight(int(document()->size().height()+getDpiScale()*4));
    }
};


class HdTextEdit : public QTextEdit, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdTextEdit(QWidget *parent = Q_NULLPTR) : QTextEdit(parent), HiDpiExtensions(this)
    {
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        setContentsMargins(0,0,0,0);

        /* Set tab width */
        ensurePolished();
        QFontMetrics metrics(font());
        setTabStopDistance(2*metrics.horizontalAdvance(" "));
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdToolBar : public QToolBar, public HiDpiExtensions
{
    Q_OBJECT

signals:
    void dpiScaleChanged(double);

public:
    explicit HdToolBar(QWidget *parent = Q_NULLPTR) : QToolBar(parent), HiDpiExtensions(this)
    {
        setContentsMargins(0,0,0,0);
    }

public slots:
    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();
    }
};


class HdToolButton : public QToolButton, public HiDpiExtensions
{
    Q_OBJECT

private:
    bool hasDynamicIconSize = false;
    int  dynamicIconHeight = 0;
    int  dynamicIconWidth = 0;

signals:
    void dpiScaleChanged(double);

public:
    explicit HdToolButton(QWidget *parent = Q_NULLPTR) : QToolButton(parent), HiDpiExtensions(this)
    { }

    void setDynamicIconSize(int w, int h)
    {
        if (h > 0 && w > 0)
        {
            hasDynamicIconSize = true;
            dynamicIconWidth = w;
            dynamicIconHeight = h;
            setIconSize(QSize(std::max(1,int(getDpiScale()*dynamicIconWidth)),
                              std::max(1,int(getDpiScale()*dynamicIconHeight)) ));
        }
        else
        {
            hasDynamicIconSize = false;
            setIconSize(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
        }
    }

public slots:

    void updateDpiScale(double scale)
    {
        if (HiDpiExtensions::setDpiScale(scale))
            emit dpiScaleChanged(getDpiScale());

        HiDpiExtensions::updateScaling();

        if (hasDynamicIconSize)
            setIconSize(QSize(std::max(1,int(getDpiScale()*dynamicIconHeight)),
                              std::max(1,int(getDpiScale()*dynamicIconWidth)) ));
    }
};

#endif
