#ifndef FRAMELESSWINDOWBUTTONS_H
#define FRAMELESSWINDOWBUTTONS_H

#include <QWindow>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <HdWidgets.h>

#include <Settings.h>


class FramelessWindowButton : public HdAbstractButton
{
    Q_OBJECT

public:
    enum ButtonType
    {
        BUTTON_MENU, BUTTON_TITLE, BUTTON_HELP, BUTTON_MINIMIZE, BUTTON_MAXIMIZE, BUTTON_CLOSE
    };

private:
    enum ButtonState
    {
        STATE_NORMAL, STATE_HOVERED, STATE_CLICKED
    };

    ButtonType buttonType;
    ButtonState buttonState;
    QPixmap *pixmapNormal;
    QPixmap *pixmapHovered;
    QPixmap *pixmapClicked;
    bool isWindowMaximized;
    double lineWidth;
    QString buttonText;
    QColor highlight, darken, brush;

public:

    FramelessWindowButton(ButtonType type, QWidget *parent) : HdAbstractButton(parent), buttonType(type), buttonState(STATE_NORMAL), pixmapNormal(Q_NULLPTR), pixmapHovered(Q_NULLPTR), pixmapClicked(Q_NULLPTR)
    {
        isWindowMaximized = window()->isMaximized();
        lineWidth = 1.0;
        setFixedSize(32, 32);
        highlight = settings.getColor("color/blend-lighter");
        darken  = settings.getColor("color/blend-darker");
        brush = QColor(255,255,255);
    }

    void setText(QString text)
    {
        buttonText = text;
    }

    ~FramelessWindowButton() override
    {
        delete pixmapNormal;
        delete pixmapHovered;
        delete pixmapClicked;
    }

private:

    void initPixmaps()
    {
        /* Delete previous button */
        InitPixmap(&pixmapNormal);
        InitPixmap(&pixmapHovered);
        InitPixmap(&pixmapClicked);

        switch(buttonType)
        {
        case BUTTON_MENU:
            InitMenu();
            break;
        case BUTTON_TITLE:
            InitTitle();
            break;
        case BUTTON_HELP:
            InitHelp();
            break;
        case BUTTON_MINIMIZE:
            InitMinimize();
            break;
        case BUTTON_MAXIMIZE:
            InitMaximize();
            break;
        case BUTTON_CLOSE:
            InitClose();
            break;
        }
    }

    void InitPixmap(QPixmap **pixmap)
    {
        delete *pixmap;
        *pixmap = new QPixmap(size());
        (*pixmap)->fill(Qt::transparent);
    }

    void InitMenu()
    {
        /* Button's symbol */
        QPolygon symbol1, symbol2, symbol3;
        double dpi = getDpiScale();
        double w = width();
        double h = height();

        symbol1 << QPoint(qRound(0.3*w), qRound(0.333*h))
                << QPoint(qRound(0.7*w), qRound(0.333*h));

        symbol2 << QPoint(qRound(0.3*w), qRound(0.5*h))
                << QPoint(qRound(0.7*w), qRound(0.5*h));

        symbol3 << QPoint(qRound(0.3*w), qRound(0.667*h))
                << QPoint(qRound(0.7*w), qRound(0.667*h));

        QPainter painter;

        /* Normal */
        painter.begin(pixmapNormal);
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);
        painter.drawPolygon(symbol2);
        painter.drawPolygon(symbol3);
        painter.end();

        /* Hovered */
        painter.begin(pixmapHovered);
        painter.fillRect(rect(), QBrush(highlight));
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);
        painter.drawPolygon(symbol2);
        painter.drawPolygon(symbol3);
        painter.end();

        /* Clicked */
        painter.begin(pixmapClicked);
        painter.fillRect(rect(), QBrush(darken));
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);
        painter.drawPolygon(symbol2);
        painter.drawPolygon(symbol3);
        painter.end();
    }

    void InitTitle()
    {
        double dpi = getDpiScale();
        int dx = qRound(dpi * 3);
        lineWidth = 2.0;

        /* Button's symbol */
        QPolygon symbol;

        symbol << QPoint(width() - 4*dx, int(height()/2) - 0)
               << QPoint(width() - 3*dx, int(height()/2) + dx)
               << QPoint(width() - 2*dx, int(height()/2) - 0);

        QPainter painter;
        painter.begin(pixmapNormal);
        QFont font = painter.font();
        font.setPointSize(qRound(dpi * 9.0));
        font.setFamily("Segoe UI");
        font.setWeight(QFont::DemiBold);
        QFontMetrics metrics = painter.fontMetrics();
        painter.end();

        if (hasDynamicWidth() || usesSizeHintWidth())
        {
            int width = metrics.horizontalAdvance(buttonText);
            if (hasDynamicWidth())
                setDynamicWidth(qRound(dpi * 4)+width+5*dx);
            else
                setFixedWidth(qRound(dpi * 4)+width+5*dx);
        }

        /* Normal */
        painter.begin(pixmapNormal);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.setBrush(QBrush(brush));
        painter.drawPolyline(symbol);
        painter.setFont(font);
        painter.drawText(QRectF(qRound(dpi * 4), 0, width(), height()), (Qt::AlignLeft | Qt::AlignVCenter), buttonText);
        painter.end();

        /* Hovered */
        painter.begin(pixmapHovered);
        painter.fillRect(rect(), QBrush(highlight));
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.setBrush(QBrush(brush));
        painter.drawPolyline(symbol);
        painter.setFont(font);
        painter.drawText(QRectF(qRound(dpi * 4), 0, width(), height()), (Qt::AlignLeft | Qt::AlignVCenter), buttonText);
        painter.end();

        /* Clicked */
        painter.begin(pixmapClicked);
        painter.fillRect(rect(), QBrush(darken));
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.setBrush(QBrush(brush));
        painter.drawPolyline(symbol);
        painter.setFont(font);
        painter.drawText(QRectF(qRound(dpi * 4), 0, width(), height()), (Qt::AlignLeft | Qt::AlignVCenter), buttonText);
        painter.end();
    }

    void InitHelp()
    {
        QPainter painter;

        painter.begin(pixmapNormal);
        QFont font = painter.font();
        font.setPixelSize(qRound(0.52*double(height())));
        font.setFamily("Segoe UI");
        font.setWeight(QFont::Normal);
        painter.end();

        /* Normal */
        painter.begin(pixmapNormal);
        painter.setPen(QPen(QBrush(brush), 1.0));
        painter.setFont(font);
        painter.drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, "?");
        painter.end();

        /* Hovered */
        painter.begin(pixmapHovered);
        painter.fillRect(rect(), QBrush(highlight));
        painter.setPen(QPen(QBrush(brush), 1.0));
        painter.setFont(font);
        painter.drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, "?");
        painter.end();

        /* Clicked */
        painter.begin(pixmapClicked);
        painter.fillRect(rect(), QBrush(darken));
        painter.setPen(QPen(QBrush(brush), 1.0));
        painter.setFont(font);
        painter.drawText(QRectF(0, 0, width(), height()), Qt::AlignCenter, "?");
        painter.end();
    }


    void InitMinimize()
    {
        /* Button's symbol */
        QPolygon symbol;
        double dpi = getDpiScale();
        double w = width();
        double h = height();

        symbol << QPoint(qRound(0.333*w), qRound(0.5*h))
               << QPoint(qRound(0.667*w), qRound(0.5*h));


        QPainter painter;

        /* Normal */
        painter.begin(pixmapNormal);
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol);
        painter.end();

        /* Hovered */
        painter.begin(pixmapHovered);
        painter.fillRect(rect(), QBrush(highlight));
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol);
        painter.end();

        /* Clicked */
        painter.begin(pixmapClicked);
        painter.fillRect(rect(), QBrush(darken));
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol);
        painter.end();
    }

    void InitMaximize()
    {
        /* Button's symbol */
        QPolygon symbol1, symbol2;
        double dpi = getDpiScale();
        double w = width();
        double h = height();

        if (!window()->isMaximized())
        {
            symbol1 << QPoint(qRound(0.333*w), qRound(0.333*h))
                    << QPoint(qRound(0.667*w), qRound(0.333*h))
                    << QPoint(qRound(0.667*w), qRound(0.667*h))
                    << QPoint(qRound(0.333*w), qRound(0.667*h));
        }
        else
        {
            symbol1 << QPoint(qRound(0.3*w), qRound(0.4*h))
                    << QPoint(qRound(0.6*w), qRound(0.4*w))
                    << QPoint(qRound(0.6*w), qRound(0.7*h))
                    << QPoint(qRound(0.3*w), qRound(0.7*h));

            symbol2 << QPoint(qRound(0.4*w), qRound(0.4*w))
                    << QPoint(qRound(0.4*w), qRound(0.3*h))
                    << QPoint(qRound(0.7*w), qRound(0.3*h))
                    << QPoint(qRound(0.7*w), qRound(0.6*h))
                    << QPoint(qRound(0.6*w), qRound(0.6*h))
                    << QPoint(qRound(0.6*w), qRound(0.4*w));
        }

        QPainter painter;

        /* Normal */
        painter.begin(pixmapNormal);
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);

        if (window()->isMaximized())
            painter.drawPolygon(symbol2);

        painter.end();

        /* Hovered */
        painter.begin(pixmapHovered);
        painter.fillRect(rect(), QBrush(highlight));
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);

        if (window()->isMaximized())
            painter.drawPolygon(symbol2);

        painter.end();

        /* Clicked */
        painter.begin(pixmapClicked);
        painter.fillRect(rect(), QBrush(darken));
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);

        if (window()->isMaximized())
            painter.drawPolygon(symbol2);

        painter.end();
    }

    void InitClose()
    {
        /* Button's symbol */
        double dpi = getDpiScale();
        double w = width();
        double h = height();

        QLine symbol1(QPoint(qRound(0.333*w), qRound(0.333*h)),
                      QPoint(qRound(0.667*w), qRound(0.667*h)));
        QLine symbol2(QPoint(qRound(0.667*w), qRound(0.333*h)),
                      QPoint(qRound(0.333*w), qRound(0.667*h)));

        QPainter painter;

        /* Normal */
        painter.begin(pixmapNormal);
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawLine(symbol1);
        painter.drawLine(symbol2);
        painter.end();

        /* Hovered */
        painter.begin(pixmapHovered);
        painter.fillRect(rect(), QBrush(Qt::red));
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawLine(symbol1);
        painter.drawLine(symbol2);
        painter.end();

        /* Clicked */
        painter.begin(pixmapClicked);
        painter.fillRect(rect(), QBrush(Qt::red));
        painter.setPen(QPen(QBrush(brush), int(dpi * lineWidth), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawLine(symbol1);
        painter.drawLine(symbol2);
        painter.end();
    }

    void enterEvent(QEnterEvent *event) override
    {
        Q_UNUSED(event)
        buttonState = STATE_HOVERED;
        update();
    }

    void leaveEvent(QEvent *event) override
    {
        Q_UNUSED(event)
        buttonState = STATE_NORMAL;
        update();
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        buttonState = STATE_CLICKED;
        update();
        HdAbstractButton::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (underMouse())
            buttonState = STATE_HOVERED;
        else
            buttonState = STATE_NORMAL;

        update();
        HdAbstractButton::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (underMouse())
            buttonState = STATE_HOVERED;
        else
            buttonState = STATE_NORMAL;

        update();
        HdAbstractButton::mouseMoveEvent(event);
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        Q_UNUSED(event)
        buttonState = STATE_NORMAL;
        update();
    }

protected:

    void resizeEvent(QResizeEvent *event) override
    {
        initPixmaps();
        update();
        HdAbstractButton::resizeEvent(event);
    }

    void paintEvent (QPaintEvent  *event) override
    {
        Q_UNUSED(event)

        if (isWindowMaximized != window()->isMaximized())
        {
            initPixmaps();
            isWindowMaximized = window()->isMaximized();
            QTimer::singleShot(0, this, [this](){update();});
        }

        QPainter painter(this);

        if (isEnabled())
        {
            switch(buttonState)
            {
            case STATE_NORMAL:
                if(pixmapNormal  != Q_NULLPTR) painter.drawPixmap(0, 0, *pixmapNormal);
                break;
            case STATE_HOVERED:
                if(pixmapHovered != Q_NULLPTR) painter.drawPixmap(0, 0, *pixmapHovered);
                break;
            case STATE_CLICKED:
                if(pixmapClicked != Q_NULLPTR) painter.drawPixmap(0, 0, *pixmapClicked);
                break;
            }
        }
        else
        {
            if(pixmapNormal != Q_NULLPTR)
                painter.drawPixmap(0, 0, *pixmapNormal);
        }
    }

};

#endif
