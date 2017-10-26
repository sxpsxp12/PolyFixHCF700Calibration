#include "standarditem.h"

StandardItem::StandardItem(const QString text):
    QStandardItem(text),m_is_checked(false)
{
    setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);

    m_portItem = new QStandardItem("");
    m_addrItem = new QStandardItem("");
    m_payloadPressureItem = new QStandardItem("");
    m_factorItem = new QStandardItem("");

    m_portItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemNeverHasChildren);
    m_addrItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemNeverHasChildren);
    m_payloadPressureItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemNeverHasChildren);
    m_factorItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemNeverHasChildren);
}

void StandardItem::setIsChecked(bool is_checked)
{
    m_is_checked = is_checked;
}

bool StandardItem::getIsChecked() const
{
    return m_is_checked;
}
