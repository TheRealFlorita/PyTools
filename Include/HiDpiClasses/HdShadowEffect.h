#ifndef HDSHADOWEFFECT_H
#define HDSHADOWEFFECT_H

#include <QGraphicsDropShadowEffect>
#include <QGraphicsEffect>
#include <QPainter>

extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);


class HdShadowEffect : public QGraphicsEffect
{
    Q_OBJECT

private:
    qreal  _offsetX;
    qreal  _offsetY;
    qreal  _distance;
    qreal  _blurRadius;
    QColor _color;
    qreal _dpi = 1.0;
    QImage _blurred;

signals:
    void dpiScaleChanged(double);

public:
    explicit HdShadowEffect(QObject *parent = Q_NULLPTR, qreal dpi = 1.0, qreal blur = 10.0, QColor shadow = QColor(0,0,0,50)) : QGraphicsEffect(parent)
    {
        _offsetX = 0.0;
        _offsetY = 0.0;
        _distance = 0.0;
        _blurRadius = blur;
        _color = shadow;
        _dpi = std::max(dpi, 0.1);
    }

    void draw(QPainter* painter) override
    {
        /* If nothing to show outside the item, just draw source */
        if (((blurRadius() + distance()) <= 0) || (_color.alpha() == 0))
        {
            drawSource(painter);
            return;
        }

        PixmapPadMode mode = QGraphicsEffect::PadToEffectiveBoundingRect;
        QPoint offset;
        const QPixmap px = sourcePixmap(Qt::DeviceCoordinates, &offset, mode);

        /* Return if no source */
        if (px.isNull())
            return;

        /* Save world transform */
        QTransform trans = painter->worldTransform();
        painter->setWorldTransform(QTransform());

        /* Calculate size for the background image */
        QSize sz(int(px.size().width() + 2 * distance()), int(px.size().height() + 2 * distance()));

        if (_blurred.isNull() || sz != _blurred.size())
        {
            QImage tmp(sz, QImage::Format_ARGB32_Premultiplied);
            tmp.setOffset(QPoint(int(offsetX()), int(offsetY())));
            tmp.fill(0);
            QPainter tmpPainter(&tmp);
            tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
            tmpPainter.drawPixmap(QPointF(-distance()+offsetX(), -distance()+offsetY()), px.scaled(sz));
            tmpPainter.end();

            /* Blur alpha channel */
            QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
            blurred.fill(0);
            QPainter blurPainter(&blurred);
            qt_blurImage(&blurPainter, tmp, blurRadius(), false, true);
            blurPainter.end();
            _blurred = blurred;

            /* Blacken image */
            tmpPainter.begin(&_blurred);
            tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tmpPainter.fillRect(_blurred.rect(), color());
            tmpPainter.end();
        }

        /* Draw blurred shadow */
        painter->drawImage(offset, _blurred);

        /* Draw pixmap */
        painter->drawPixmap(offset, px, QRectF());

        /* Restore world transform */
        painter->setWorldTransform(trans);
    }

    QRectF boundingRectFor(const QRectF& rect) const override
    {
        qreal delta = blurRadius() + distance();
        return rect.united(rect.adjusted(std::min(0.0, -delta+offsetX()),
                                         std::min(0.0, -delta+offsetY()),
                                         std::max(0.0, delta+offsetX()),
                                         std::max(0.0, delta+offsetY())));
    }

    inline void setOffsetX(qreal offsetX)
    {
        _offsetX = offsetX;
        updateBoundingRect();
    }

    inline qreal offsetX() const
    {
        return _dpi*_offsetX;
    }

    inline void setOffsetY(qreal offsetY)
    {
        _offsetY = offsetY;
        updateBoundingRect();
    }

    inline qreal offsetY() const
    {
        return _dpi*_offsetY;
    }

    inline void setDistance(qreal distance)
    {
        _distance = distance;
        updateBoundingRect();
    }

    inline qreal distance() const
    {
        return _dpi*_distance;
    }

    inline void setBlurRadius(qreal blurRadius)
    {
        _blurRadius = blurRadius;
        updateBoundingRect();
    }

    inline qreal blurRadius() const
    {
        return _dpi*_blurRadius;
    }

    inline void setColor(const QColor& color)
    {
        _color = color;
    }

    inline QColor color() const
    {
        return _color;
    }

    void setDpiScale(double scale)
    {
        if (scale > 0.1)
        {
            _dpi = scale;
            updateBoundingRect();
            emit dpiScaleChanged(_dpi);
        }
    }

    double getDpiScale()
    {
        return _dpi;
    }


public slots:

    void updateDpiScale(double scale)
    {
        if ((scale > 0.1) && (std::abs(_dpi-scale) > 1e-3))
            setDpiScale(scale);
    }

};

#endif
