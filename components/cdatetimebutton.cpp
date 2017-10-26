#include "cdatetimebutton.h"

CDateTimeButton::CDateTimeButton(QWidget *parent) :
    QPushButton(parent),m_maximumDate(),m_minimumDate()
{
    this->setText("未定义");
    this->setEnabled(false);
}

void CDateTimeButton::setDateTime(const QDateTime &dt)
{
    if(dt.isValid())
    {
        this->setText(dt.toString("yyyy-MM-dd HH:mm:ss"));
        this->setEnabled(true);
        if(dt>m_maximumDate)
            this->setText(m_maximumDate.toString("yyyy-MM-dd HH:mm:ss"));
        else if(dt<m_minimumDate)
            this->setText(m_minimumDate.toString("yyyy-MM-dd HH:mm:ss"));
//        emit dateTimeChanged(dt);
    }else
    {
        this->setText("未定义");
        this->setEnabled(false);
//        emit dateTimeChanged(QDateTime());
    }
}

void CDateTimeButton::setMaximumDateTime(const QDateTime &d)
{
    m_maximumDate = d;
    if(d.isValid())
    {
        QDateTime t = QDateTime::fromString(this->text(),"yyyy-MM-dd HH:mm:ss");
        if(t.isValid() && t<d)
            this->setText(d.toString("yyyy-MM-dd HH:mm:ss"));
    }
}

void CDateTimeButton::setMinimumDateTime(const QDateTime &d)
{
    m_minimumDate = d;
    if(d.isValid())
    {
        QDateTime t = QDateTime::fromString(this->text(),"yyyy-MM-dd HH:mm:ss");
        if(t.isValid() && t>d)
            this->setText(d.toString("yyyy-MM-dd HH:mm:ss"));
    }
}
