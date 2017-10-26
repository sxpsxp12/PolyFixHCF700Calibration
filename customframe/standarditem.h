#ifndef STANDARDITEM_H
#define STANDARDITEM_H

#include <QStandardItem>

class StandardItem : public QStandardItem
{
public:
    explicit StandardItem(const QString text);
    ~StandardItem() {}

    void setIsChecked(bool is_checked);
    bool getIsChecked() const;

    QStandardItem *portItem() const {return m_portItem;}
    QStandardItem *addrItem() const {return m_addrItem;}
    QStandardItem *pressureItem() const {return m_payloadPressureItem;}
    QStandardItem *factorItem() const {return m_factorItem;}

private:
    bool m_is_checked;
    QStandardItem *m_portItem;
    QStandardItem *m_addrItem;
    QStandardItem *m_payloadPressureItem;
    QStandardItem *m_factorItem;
};

#endif // STANDARDITEM_H
