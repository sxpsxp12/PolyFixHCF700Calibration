#include "headerview.h"
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

//默认水平表头,无指示标记
HeaderView::HeaderView(Qt::Orientation orientation, bool is_indentation, QWidget *parent):
    QHeaderView(orientation,parent),m_is_indentation(is_indentation)
{
    m_state = Qt::Unchecked;
    m_is_pressed = false;

    setSectionsClickable(true);
}

void HeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    QHeaderView::paintSection(painter,rect,logicalIndex);
    painter->restore();

    painter->save();

    painter->setBrush(QBrush(Qt::gray));
    painter->setPen(Qt::NoPen);
    painter->drawRect(rect);

    painter->setFont(QFont("SimSun", 10));
    painter->setPen(QColor("#000000"));
    painter->drawText(rect, model()->headerData(logicalIndex,Qt::Horizontal,Qt::TextAlignmentRole).toInt(), model()->headerData(logicalIndex,Qt::Horizontal).toString());

    painter->restore();

    if(logicalIndex == 0)
    {
        QStyleOptionButton option;
        option.initFrom(this);
        if(m_state == Qt::Unchecked)
        {
            option.state |= QStyle::State_Off;
        }
        else if(m_state == Qt::PartiallyChecked)
        {
            option.state |= QStyle::State_NoChange;
        }
        else if(m_state == Qt::Checked)
        {
            option.state |= QStyle::State_On;
        }
        option.iconSize = QSize(20, 20);
        if(m_is_indentation)
            option.rect = QRect(QPoint(rect.left()+(rect.width()-20)/2+15,rect.top()+(rect.height()-20)/2),QPoint(rect.left()+(rect.width()-20)/2+20+15,rect.bottom()-(rect.height()-20)/2));
        else
            option.rect = QRect(QPoint(rect.left()+(rect.width()-20)/2,rect.top()+(rect.height()-20)/2),QPoint(rect.left()+(rect.width()-20)/2+20,rect.bottom()-(rect.height()-20)/2));
        style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);
    }
}

void HeaderView::mousePressEvent(QMouseEvent *e)
{
    int nColumn = logicalIndexAt(e->pos());
    if ((e->buttons() & Qt::LeftButton) && (nColumn == 0))
    {
        m_is_pressed = true;

        e->accept();
    }
    e->ignore();
}

void HeaderView::mouseReleaseEvent(QMouseEvent *e)
{
    if(m_is_pressed)
    {
        if(m_state == Qt::Unchecked)
        {
            m_state = Qt::Checked;
        }else
        {
            m_state = Qt::Unchecked;
        }
        updateSection(0);
        emit stateChanged(m_state); //状态改变
    }
    m_is_pressed = false;
    e->accept();
}

void HeaderView::slot_stateChanged(Qt::CheckState state)
{
    m_state = state;

    updateSection(0);
}
