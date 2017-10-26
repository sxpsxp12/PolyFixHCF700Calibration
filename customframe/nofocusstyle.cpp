#include "nofocusstyle.h"

NoFocusStyle::NoFocusStyle():
    QProxyStyle()
{

}

void NoFocusStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if(element == PE_FrameFocusRect)
    {

    }else
    {
        QProxyStyle::drawPrimitive(element, option,painter, widget);
    }
}
