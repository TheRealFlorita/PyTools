#ifndef FRAMELESSFILEDIALOG_H
#define FRAMELESSFILEDIALOG_H

#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QScreen>
#include <QToolButton>

#include <HdLayouts.h>
#include <HdWidgets.h>
#include <FramelessWindowTitleBar.h>
#include <Settings.h>


class FramelessFileDialog : public HdFileDialog
{
Q_OBJECT

protected:
    FramelessWindowTitleBar *titleBar;
    double dpi = 1.0;
    double stepSize = 0.05;
    bool keyScaling = false;

signals:
    void screenChanged();

public:

    explicit FramelessFileDialog(QWidget *parent = Q_NULLPTR, QString caption = FramelessFileDialog::tr("File Dialog")) : HdFileDialog(parent, caption)
    {
        titleBar = new FramelessWindowTitleBar(this, FramelessWindowTitleBar::DIALOG);
        connect(this , &FramelessFileDialog::dpiScaleChanged, titleBar, &FramelessWindowTitleBar::updateDpiScale);

        setWindowTitle(caption);
        setStyleSheet(settings.getBaseStyleSheet());
        setDpiScale(settings.getGlobalDpiScale());

        setSidebarUrls(settings.getFileDialogLocations());

        QGridLayout *original = static_cast<QGridLayout*>(layout());

        QBoxLayout *centralLayout = new QBoxLayout(QBoxLayout::TopToBottom);
        centralLayout->setContentsMargins(0,0,0,0);
        centralLayout->setSpacing(0);
        centralLayout->addWidget(titleBar);

        QWidget *centralWiget = new QWidget(this);
        centralLayout->addWidget(centralWiget);
        centralWiget->setLayout(original);
        setLayout(centralLayout);

        connect(this , &FramelessFileDialog::dpiScaleChanged, this, &FramelessFileDialog::updateSize);

        /* Define window size and position */
        setMinimumSize(400,400);
        resetSize();
        center();

        setFocusPolicy(Qt::StrongFocus);

        const QList<QToolButton *> buttons = this->findChildren<QToolButton *>();
        for (QToolButton *button: buttons)
        {
            if (button->objectName() == "backButton")
            {
                QIcon icon(settings.getApplicationPath() + "/icons/arrow-tail-left.svg");
                button->setIcon(icon);
            }
            else if (button->objectName() == "forwardButton")
            {
                QIcon icon(settings.getApplicationPath() + "/icons/arrow-tail-right.svg");
                button->setIcon(icon);
            }
            else if (button->objectName() == "toParentButton")
            {
                QIcon icon(settings.getApplicationPath() + "/icons/arrow-tail-up.svg");
                button->setIcon(icon);
            }
            else if (button->objectName() == "newFolderButton")
            {
                QIcon icon(settings.getApplicationPath() + "/icons/folder-new.svg");
                button->setIcon(icon);
            }
            else if (button->objectName() == "listModeButton")
            {
                QIcon icon(settings.getApplicationPath() + "/icons/list-view.svg");
                button->setIcon(icon);
            }
            else if (button->objectName() == "detailModeButton")
            {
                QIcon icon(settings.getApplicationPath() + "/icons/table-view.svg");
                button->setIcon(icon);
            }
        }
    }

    ~FramelessFileDialog() override
    { }

    static QString getExistingDirectory(QWidget *parent = Q_NULLPTR, const QString &caption = QString(),
                                        const QString &dir = QString(), QFileDialog::Options options = Options())
    {
        QString title = caption;
        if (caption.isEmpty())
            title = FramelessFileDialog::tr("File Dialog");

        FramelessFileDialog fileDialog(parent, title);
        fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
        fileDialog.setFileMode(QFileDialog::Directory);
        fileDialog.setDirectory(QFileInfo(dir).absolutePath());
        fileDialog.setOptions(options);

        if (fileDialog.exec())
            return (fileDialog.selectedFiles().constFirst());
        return QString();
    }

    static QString getOpenFileName(QWidget *parent = Q_NULLPTR, const QString &caption = QString(),
                                   const QString &dir = QString(), const QString &filter = QString(),
                                   QString *selectedFilter = Q_NULLPTR, QFileDialog::Options options = Options())
    {
        QString title = caption;
        if (caption.isEmpty())
            title = FramelessFileDialog::tr("File Dialog");

        FramelessFileDialog fileDialog(parent, title);
        fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        fileDialog.setDirectory(QFileInfo(dir).absolutePath());
        fileDialog.setNameFilter(filter);
        fileDialog.setOptions(options);

        if (fileDialog.exec())
            return (fileDialog.selectedFiles().constFirst());
        return QString();
    }

    static QStringList getOpenFileNames(QWidget *parent = Q_NULLPTR, const QString &caption = QString(),
                                        const QString &dir = QString(), const QString &filter = QString(),
                                        QString *selectedFilter = Q_NULLPTR, QFileDialog::Options options = Options())
    {
        QString title = caption;
        if (caption.isEmpty())
            title = FramelessFileDialog::tr("File Dialog");

        FramelessFileDialog fileDialog(parent, title);
        fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setDirectory(QFileInfo(dir).absolutePath());
        fileDialog.setNameFilter(filter);
        fileDialog.setOptions(options);

        if (fileDialog.exec())
            return (fileDialog.selectedFiles());
        return QStringList();
    }

    static QString getSaveFileName(QWidget *parent = Q_NULLPTR, const QString &caption = QString(),
                                   const QString &dir = QString(), const QString &filter = QString(),
                                   QString *selectedFilter = Q_NULLPTR, QFileDialog::Options options = Options())
    {
        QString title = caption;
        if (caption.isEmpty())
            title = FramelessFileDialog::tr("File Dialog");

        FramelessFileDialog fileDialog(parent, title);
        fileDialog.setStyleSheet(settings.getBaseStyleSheet());
        fileDialog.setDpiScale(settings.getGlobalDpiScale());
        fileDialog.setAcceptMode(QFileDialog::AcceptSave);
        fileDialog.setFileMode(QFileDialog::AnyFile);
        fileDialog.setDirectory(QFileInfo(dir).absolutePath());
        fileDialog.setNameFilter(filter);
        fileDialog.setOptions(options);

        if (fileDialog.exec())
            return (fileDialog.selectedFiles().constFirst());
        return QString();
    }

    void resetSize()
    {
        int hscreen = screen()->availableGeometry().height();
        int wscreen = screen()->availableGeometry().width();
        int hwindow = qRound(0.5*hscreen);
        int wwindow = qRound(0.5*wscreen);

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

    double getDpiScale()
    {
        return dpi;
    }

    void setDpiScale(double scale)
    {
        dpi = std::min(100, std::max(1, qRound(scale/stepSize))) * stepSize;
        setStyleSheet(settings.getScaledStyleSheet(dpi));
        updateScaling();
        updateSize();
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
        setDpiScale(scale);
        HdFileDialog::updateDpiScale(dpi);
        updateSize();
    }

    void setEnableKeyScaling(bool enable)
    {
        keyScaling = enable;
    }

protected:

    void updateSize()
    {
        QLineEdit *fileNameEdit = findChild<QLineEdit*>("fileNameEdit");
        int h = fileNameEdit->sizeHint().height();

        QList<QToolButton*> list1 = findChildren<QToolButton*>();
        QList<QPushButton*> list2 = findChildren<QPushButton*>();
        QList<QComboBox*> list3 = findChildren<QComboBox*>();

        foreach(QToolButton *button, list1)
            button->setFixedSize(h,h);

        foreach(QPushButton *button, list2)
            button->setFixedHeight(h);

        foreach(QComboBox *box, list3)
            box->setFixedHeight(h);
    }

    bool event(QEvent* event) override
    {
        switch(event->type())
        {
        case QEvent::Show:
            connect(windowHandle(), &QWindow::screenChanged, this, &FramelessFileDialog::screenChanged, Qt::UniqueConnection);
            connect(this, &FramelessFileDialog::screenChanged, titleBar, &FramelessWindowTitleBar::onScreenChanged, Qt::UniqueConnection);
            connect(this, &FramelessFileDialog::screenChanged, this, &FramelessFileDialog::updateSize, Qt::UniqueConnection);
            center();
            updateSize();
            break;
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
            break;
        }

        return HdFileDialog::event(event);
    }
};

#endif
