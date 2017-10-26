#ifndef CCALENDARWND_H
#define CCALENDARWND_H

#include <QObject>
#include <QCalendarWidget>

class  QTableView;

class CCalendarWnd : public QCalendarWidget
{
    Q_OBJECT
public:
    CCalendarWnd(QWidget *parent = 0);
    ~CCalendarWnd(){
    }

protected:
    virtual void paintCell(QPainter *painter, const QRect &rect, const QDate &date) const;

    virtual void mouseMoveEvent(QMouseEvent *e);

    QTableView * m_pview;

    QDate m_date_hover;
};

#endif // CCALENDARWND_H
