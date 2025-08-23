#ifndef FRAMELESSDOCKBUTTONS_H
#define FRAMELESSDOCKBUTTONS_H

#include <QWindow>
#include <QPixmap>
#include <QPainter>
#include <HdWidgets.h>

#include <Settings.h>


class FramelessDockButton : public HdAbstractButton
{
    Q_OBJECT

public:

    enum ButtonType
    {
        BUTTON_RUN, BUTTON_DOCK
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
    QColor highlight, darken, brush;

public:

    FramelessDockButton(ButtonType type, QWidget *parent) : HdAbstractButton(parent), buttonType(type), buttonState(STATE_NORMAL), pixmapNormal(Q_NULLPTR), pixmapHovered(Q_NULLPTR), pixmapClicked(Q_NULLPTR)
    {
        setFixedSize(32, 32);
        setCheckable(true);
        connect(this, &FramelessDockButton::toggled, this, &FramelessDockButton::initPixmaps);

        highlight = settings.getColor("color/blend-lighter");
        darken  = settings.getColor("color/blend-darker");
        brush = QColor(255,255,255);

    }

    ~FramelessDockButton() override
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
        case BUTTON_RUN:
            InitRun();
            break;
        case BUTTON_DOCK:
            InitDock();
            break;
        default:
            break;
        }
    }

    void InitPixmap(QPixmap **pixmap)
    {
        delete *pixmap;

        *pixmap = new QPixmap(size());

        (*pixmap)->fill(Qt::transparent);
    }

    void InitRun()
    {
        /* Button's symbol */
        QPolygon symbol1, symbol2;
        double w = width();
        double h = height();

        if (!isChecked())
        {
            symbol1 << QPoint(qRound(0.2*w), qRound(0.2*h))
                    << QPoint(width()-qRound(0.2*w), qRound(0.50*h))
                    << QPoint(qRound(0.2*w), height()-qRound(0.2*h));
        }
        else
        {
            symbol1 << QPoint(qRound(0.2*w), qRound(0.2*h))
                    << QPoint(qRound(0.4*w), qRound(0.2*h))
                    << QPoint(qRound(0.4*w), height()-1-qRound(0.2*h))
                    << QPoint(qRound(0.2*w), height()-1-qRound(0.2*h));

            symbol2 << QPoint(width()-1-qRound(0.4*w), qRound(0.2*h))
                    << QPoint(width()-1-qRound(0.2*w), qRound(0.2*h))
                    << QPoint(width()-1-qRound(0.2*w), height()-1-qRound(0.2*h))
                    << QPoint(width()-1-qRound(0.4*w), height()-1-qRound(0.2*h));
        }

        QPainter painter;

        /* Normal */
        painter.begin(pixmapNormal);

        if (!isChecked())
        {
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(QBrush(brush), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        }
        else
        {
            painter.setRenderHint(QPainter::Antialiasing, false);
            painter.setPen(QPen(QBrush(brush), 0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        }

        painter.setBrush(brush);
        painter.drawPolygon(symbol1, Qt::WindingFill);

        if (isChecked())
            painter.drawPolygon(symbol2);

        painter.end();

        /* Hovered */
        painter.begin(pixmapHovered);
        painter.fillRect(rect(), QBrush(highlight));

        if (!isChecked())
        {
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(QBrush(brush), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        }
        else
        {
            painter.setRenderHint(QPainter::Antialiasing, false);
            painter.setPen(QPen(QBrush(brush), 0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        }

        painter.setBrush(brush);
        painter.drawPolygon(symbol1, Qt::WindingFill);

        if (isChecked())
            painter.drawPolygon(symbol2, Qt::WindingFill);

        painter.end();

        /* Clicked */
        painter.begin(pixmapClicked);
        painter.fillRect(rect(), QBrush(darken));

        if (!isChecked())
        {
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(QBrush(brush), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        }
        else
        {
            painter.setRenderHint(QPainter::Antialiasing, false);
            painter.setPen(QPen(QBrush(brush), 0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        }

        painter.setBrush(brush);
        painter.drawPolygon(symbol1, Qt::WindingFill);

        if (isChecked())
            painter.drawPolygon(symbol2, Qt::WindingFill);
        painter.end();
    }

    void InitDock()
    {
        /* Button's symbol */
        QPolygon symbol1, symbol2;
        double sf = height() / 32.0;

        if (!isChecked())
        {
            symbol1 << QPoint(qRound(sf*19), qRound(sf*19))
                    << QPoint(qRound(sf*19), qRound(sf*22))
                    << QPoint(qRound(sf*10), qRound(sf*22))
                    << QPoint(qRound(sf*10), qRound(sf*13))
                    << QPoint(qRound(sf*13), qRound(sf*13))
                    << QPoint(qRound(sf*13), qRound(sf*19));

            symbol2 << QPoint(qRound(sf*13), qRound(sf*10))
                    << QPoint(qRound(sf*22), qRound(sf*10))
                    << QPoint(qRound(sf*22), qRound(sf*19))
                    << QPoint(qRound(sf*13), qRound(sf*19));
        }
        else
        {
            symbol1 << QPoint(qRound(sf*10), qRound(sf*10))
                    << QPoint(qRound(sf*22), qRound(sf*10))
                    << QPoint(qRound(sf*22), qRound(sf*22))
                    << QPoint(qRound(sf*10), qRound(sf*22));

            symbol2 << QPoint(qRound(sf*10), qRound(sf*10));
        }

        QPainter painter;

        /* Normal */
        painter.begin(pixmapNormal);
        painter.setPen(QPen(QBrush(brush), qRound(sf * 1.0), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);
        painter.drawPolygon(symbol2);
        painter.end();

        /* Hovered */
        painter.begin(pixmapHovered);
        painter.fillRect(rect(), QBrush(highlight));
        painter.setPen(QPen(QBrush(brush), qRound(sf * 1.0), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);
        painter.drawPolygon(symbol2);
        painter.end();

        /* Clicked */
        painter.begin(pixmapClicked);
        painter.fillRect(rect(), QBrush(darken));
        painter.setPen(QPen(QBrush(brush), qRound(sf * 1.0), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.drawPolygon(symbol1);
        painter.drawPolygon(symbol2);
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
