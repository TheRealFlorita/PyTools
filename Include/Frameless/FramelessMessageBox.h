#ifndef FRAMELESSMESSAGEBOX_H
#define FRAMELESSMESSAGEBOX_H

#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QScreen>

#include <HdLayouts.h>
#include <HdWidgets.h>
#include <FramelessWindowTitleBar.h>
#include <Settings.h>


class FramelessMessageBox : public HdMessageBox
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
    explicit FramelessMessageBox(QWidget *parent = Q_NULLPTR) : HdMessageBox(parent)
    {
        titleBar = new FramelessWindowTitleBar(this, FramelessWindowTitleBar::DIALOG);
        connect(this , &FramelessMessageBox::dpiScaleChanged, titleBar, &FramelessWindowTitleBar::updateDpiScale);
        setMinimumSize(420,280);
        setDpiScale(settings.getGlobalDpiScale());
    }

    explicit FramelessMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text, QMessageBox::StandardButtons buttons = NoButton, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint) : HdMessageBox(icon, title, text, buttons, parent, flags)
    {
        titleBar = new FramelessWindowTitleBar(this, FramelessWindowTitleBar::DIALOG);
        connect(this , &FramelessMessageBox::dpiScaleChanged, titleBar, &FramelessWindowTitleBar::updateDpiScale);
        setMinimumSize(420,280);
        setDpiScale(settings.getGlobalDpiScale());
    }

    ~FramelessMessageBox() override
    { }

    static void aboutQt(HiDpiExtensions *parent, QString caption)
    {

        QString translatedTextAboutQtCaption;
        translatedTextAboutQtCaption = QString("<h3>About Qt</h3><p>This program uses Qt version %1.</p>").arg(QLatin1String(QT_VERSION_STR));

        const QString translatedTextAboutQtText = QString(
              "<p>Qt is a C++ toolkit for cross-platform application "
              "development.</p>"
              "<p>Qt provides single-source portability across all major desktop "
              "operating systems. It is also available for embedded Linux and other "
              "embedded and mobile operating systems.</p>"
              "<p>Qt is available under multiple licensing options designed "
              "to accommodate the needs of our various users.</p>"
              "<p>Qt licensed under our commercial license agreement is appropriate "
              "for development of proprietary/commercial software where you do not "
              "want to share any source code with third parties or otherwise cannot "
              "comply with the terms of GNU (L)GPL.</p>"
              "<p>Qt licensed under GNU (L)GPL is appropriate for the "
              "development of Qt&nbsp;applications provided you can comply with the terms "
              "and conditions of the respective licenses.</p>"
              "<p>Please see <a href=\"https://%2/\">%2</a> "
              "for an overview of Qt licensing.</p>"
              "<p>Copyright (C) The Qt Company Ltd. and other "
              "contributors.</p>"
              "<p>Qt and the Qt logo are trademarks of The Qt Company Ltd.</p>"
              "<p>Qt is The Qt Company Ltd. product developed as an open source "
              "project. See <a href=\"https://%3/\">%3</a> for more information.</p>"
              ).arg(QStringLiteral("qt.io/licensing"),
                   QStringLiteral("qt.io"));

        FramelessMessageBox msg;
        msg.setWindowTitle(caption);
        msg.setText(translatedTextAboutQtCaption);
        msg.setInformativeText(translatedTextAboutQtText);

        QIcon logo(settings.getApplicationPath() + "/icons/qt-logo.svg");
        msg.setIconPixmap(logo.pixmap(qRound(msg.getDpiScale()*64), qRound(msg.getDpiScale()*64)));
        msg.exec();
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
        HdMessageBox::updateDpiScale(dpi);
    }

    void setEnableKeyScaling(bool enable)
    {
        keyScaling = enable;
    }

    void setAutoDeleteOnClose(bool enable = true)
    {
        setAttribute(Qt::WA_DeleteOnClose, enable);

        if (enable)
            connect(&settings, &Settings::aboutToQuit, this, &FramelessMessageBox::close);
        else
            disconnect(&settings, &Settings::aboutToQuit, this, &FramelessMessageBox::close);
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
            break;
        }

        return HdMessageBox::event(event);
    }
};

#endif
