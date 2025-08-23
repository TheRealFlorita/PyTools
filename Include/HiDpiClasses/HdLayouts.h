#ifndef HDLAYOUTS_H
#define HDLAYOUTS_H

#include <QBoxLayout>
#include <QGridLayout>


class HdBoxLayout : public QBoxLayout
{
    Q_OBJECT

private:
    bool hasDynamicSpacing = false;
    bool hasDynamicMargins = false;
    int  dynamicSpacing = 0;
    int  dynamicMarginLeft = 0;
    int  dynamicMarginTop = 0;
    int  dynamicMarginRight = 0;
    int  dynamicMarginBottom = 0;
    double dpi = 1.0;

signals:
    void dpiScaleChanged(double);

public:
    explicit HdBoxLayout(QBoxLayout::Direction dir, QWidget *parent = Q_NULLPTR) : QBoxLayout(dir, parent)
    {
        setContentsMargins(0,0,0,0);
        setSpacing(0);
    }

    void setDynamicSpacing(int spacing)
    {
        hasDynamicSpacing = true;
        dynamicSpacing = spacing;
        setSpacing(int(dpi*dynamicSpacing));
    }

    void setDynamicMargins(int left, int top, int right, int bottom)
    {
        hasDynamicMargins = true;
        dynamicMarginLeft = left;
        dynamicMarginTop = top;
        dynamicMarginRight = right;
        dynamicMarginBottom = bottom;
        setContentsMargins(int(dpi*dynamicMarginLeft),int(dpi*dynamicMarginTop),int(dpi*dynamicMarginRight),int(dpi*dynamicMarginBottom));
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
            emit dpiScaleChanged(dpi);
            return true;
        }
        return false;
    }

public slots:

    void updateDpiScale(double scale)
    {
        if (std::abs(dpi-scale) > 1e-3)
        {
            setDpiScale(scale);

            if (hasDynamicSpacing)
                setSpacing(int(dpi*dynamicSpacing));

            if (hasDynamicMargins)
                setContentsMargins(int(dpi*dynamicMarginLeft),int(dpi*dynamicMarginTop),int(dpi*dynamicMarginRight),int(dpi*dynamicMarginBottom));
        }
    }

};


class HdGridLayout : public QGridLayout
{
    Q_OBJECT

private:
    bool hasDynamicHorizontalSpacing = false;
    bool hasDynamicVerticalSpacing = false;
    bool hasDynamicMargins = false;
    int  dynamicHorizontalSpacing = 0;
    int  dynamicVerticalSpacing = 0;
    int  dynamicMarginLeft = 0;
    int  dynamicMarginTop = 0;
    int  dynamicMarginRight = 0;
    int  dynamicMarginBottom = 0;
    double dpi = 1.0;

signals:
    void dpiScaleChanged(double);

public:
    explicit HdGridLayout(QWidget *parent = Q_NULLPTR) : QGridLayout(parent)
    {
        setContentsMargins(0,0,0,0);
        setSpacing(0);
    }

    void setDynamicHorizontalSpacing(int spacing)
    {
        hasDynamicHorizontalSpacing = true;
        dynamicHorizontalSpacing = spacing;
        setHorizontalSpacing(int(dpi*dynamicHorizontalSpacing));
    }

    void setDynamicVerticalSpacing(int spacing)
    {
        hasDynamicVerticalSpacing = true;
        dynamicVerticalSpacing = spacing;
        setVerticalSpacing(int(dpi*dynamicVerticalSpacing));
    }

    void setDynamicSpacing(int spacing)
    {
        setDynamicHorizontalSpacing(spacing);
        setDynamicVerticalSpacing(spacing);
    }

    void setDynamicSpacing(int horizontalSpacing, int verticalSpacing)
    {
        setDynamicHorizontalSpacing(horizontalSpacing);
        setDynamicVerticalSpacing(verticalSpacing);
    }

    void setDynamicMargins(int left, int top, int right, int bottom)
    {
        hasDynamicMargins = true;
        dynamicMarginLeft = left;
        dynamicMarginTop = top;
        dynamicMarginRight = right;
        dynamicMarginBottom = bottom;
        setContentsMargins(int(dpi*dynamicMarginLeft),int(dpi*dynamicMarginTop),int(dpi*dynamicMarginRight),int(dpi*dynamicMarginBottom));
    }

    double getDpiScale()
    {
        return dpi;
    }

    bool setDpiScale(double scale)
    {
        if (std::abs(dpi-scale) > 1e-3)
        {
            dpi = scale;
            emit dpiScaleChanged(dpi);
            return true;
        }
        return false;
    }

public slots:

    void updateDpiScale(double scale)
    {
        setDpiScale(scale);

        if (hasDynamicHorizontalSpacing)
            setHorizontalSpacing(int(dpi*dynamicHorizontalSpacing));

        if (hasDynamicVerticalSpacing)
            setVerticalSpacing(int(dpi*dynamicVerticalSpacing));

        if (hasDynamicMargins)
            setContentsMargins(int(dpi*dynamicMarginLeft),int(dpi*dynamicMarginTop),int(dpi*dynamicMarginRight),int(dpi*dynamicMarginBottom));
    }

};

#endif
