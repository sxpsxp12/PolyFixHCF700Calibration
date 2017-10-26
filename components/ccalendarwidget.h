#ifndef CCALENDARWIDGET_H
#define CCALENDARWIDGET_H

#include <QWidget>
#include <QDateTime>
#include <QMouseEvent>

namespace Ui {
class CCalendarWidget;
}

class CCalendarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CCalendarWidget(QWidget *parent = 0);
    ~CCalendarWidget();
    void setMaximumDate(const QDate &d);
    void setMinimumDate(const QDate &d);
    void setDateTime(const QDateTime &dt);

private:
    Ui::CCalendarWidget *ui;

protected:
    void mousePressEvent(QMouseEvent *e);

signals:
    void dateTimeSelect(QDateTime t);

private slots:
    void on_pushButtonSubmit_clicked();
};

#endif // CCALENDARWIDGET_H
