#include "ccalendarwnd.h"

#include <QPainter>
#include <QMouseEvent>
#include <QTableView>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QLabel>
#include <QLocale>

CCalendarWnd::CCalendarWnd(QWidget *parent):
    QCalendarWidget(parent)
{
    this->setMouseTracking(true);
    m_pview = this->findChild<QTableView*>("qt_calendar_calendarview");
    m_pview->setMouseTracking(true);
    QLocale locale(QLocale::Chinese,QLocale::China);
    this->setLocale(locale);
}

void CCalendarWnd::paintCell(QPainter *painter, const QRect &rect, const QDate &date) const
{
    QCalendarWidget::paintCell(painter, rect, date);
    if ( date.isValid() )  {
        painter->save();

//        if(date<this->minimumDate() || date>this->maximumDate())
//        {
//            painter->setPen(Qt::NoPen);
//            painter->setBrush(QColor("#e0e0e0"));
//            painter->drawRect(QRect{rect.x()+1,rect.y()+1,rect.width()-2,rect.height()-2});
//            painter->setPen(QPen(QColor("#777777")));
//            painter->drawText(rect, Qt::AlignCenter,  QString::number(date.day() ) );
//        }
//        else if(m_date_hover!=date)
//        {
//            painter->setPen(QPen(QColor("#000000")));
//            painter->drawText(rect, Qt::AlignCenter,  QString::number(date.day() ) );
//            painter->drawRect(QRect{rect.x()+1,rect.y()+1,rect.width()-2,rect.height()-2});
//        }
        if ( m_date_hover!= date) {
            painter->setPen(QPen("#e0e0e0"));
            painter->drawRect(rect);
        }
        //hover
        else {
            if  ( m_date_hover!= this->selectedDate() ) {
                painter->setPen(QPen(QColor("#00b4ff")));
                painter->drawText(rect, Qt::AlignCenter,  QString::number(date.day() ) );
                painter->drawRect(QRect{rect.x()+1,rect.y()+1,rect.width()-2,rect.height()-2});
            }
        }
        if(date==this->selectedDate())
        {
            if(m_date_hover!=this->selectedDate())
            {
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor("#00b4ff"));
                painter->drawRect(QRect{rect.x()+1,rect.y()+1,rect.width()-2,rect.height()-2});
                painter->setPen(QColor("#fdfdfd"));
                painter->drawText(rect, Qt::AlignCenter,  QString::number(date.day() ) );
            }
            else
            {
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor("#00b4ff"));
                painter->drawRect(QRect{rect.x()+1,rect.y()+1,rect.width()-2,rect.height()-2});
                painter->setPen(QColor("#ffcc49"));
                painter->drawText(rect, Qt::AlignCenter,  QString::number(date.day() ) );
            }
        }
        painter->restore();
    }
}

void CCalendarWnd::mouseMoveEvent(QMouseEvent *e)
{
    auto index = m_pview->indexAt(m_pview->mapFromGlobal(e->globalPos()));
    if ( index.isValid() ) {
        if ( m_pview->mapToGlobal(m_pview->visualRect(m_pview->model()->index(2,0)).topLeft()).y() >e->globalPos().y()
             &&  index.data().toInt() > 20 ) {
            // 一页 中出现两个 31 号 等相同的日期. 会出现按照 顶部导航中的月份 设置hover日期.
            // 所以这里判断,  如果鼠标在第3行(周一 周二 etc 算作行0) 以上, 就表面是上一个月的日期
            m_date_hover = QDate();
        }
        else if (m_pview->mapToGlobal(m_pview->visualRect(m_pview->model()->index(4,0)).bottomLeft()).y() <e->globalPos().y()
                 &&  index.data().toInt() < 10 ) {
            m_date_hover = QDate();
        }
        else {
            QDate date_hover( this->yearShown() ,this->monthShown(),index.data().toInt());
            if ( date_hover<=maximumDate()
                 &&  date_hover >=minimumDate()  ) {
                m_date_hover = date_hover;
            }
            else {
                m_date_hover = QDate();
            }
        }
        updateCells();
    }

    QCalendarWidget::mouseMoveEvent(e);
}



