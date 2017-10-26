#ifndef NOFOCUSSTYLE_H
#define NOFOCUSSTYLE_H

#include <QProxyStyle>

class NoFocusStyle : public QProxyStyle
{
    Q_OBJECT
public:
    NoFocusStyle();

    virtual void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const;

};

#endif // NOFOCUSSTYLE_H
