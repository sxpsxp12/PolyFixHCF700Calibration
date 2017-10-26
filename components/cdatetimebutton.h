#ifndef CDATETIMEBUTTON_H
#define CDATETIMEBUTTON_H

#include <QPushButton>
#include <QDateTime>
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

class CDateTimeButton : public QPushButton
{
    Q_OBJECT
public:
    explicit CDateTimeButton(QWidget *parent = 0);
    void setMaximumDateTime(const QDateTime &d);
    void setMinimumDateTime(const QDateTime &d);
    inline const QDateTime maximumDateTime(){return m_maximumDate;}
    inline const QDateTime minimumDateTime(){return m_minimumDate;}

signals:
    void dateTimeChanged(const QDateTime dt);

public slots:
    void setDateTime(const QDateTime &dt);

private:
    QDateTime m_maximumDate;
    QDateTime m_minimumDate;
};

#endif // CDATETIMEBUTTON_H
