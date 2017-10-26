#include "standarditemmodel.h"
#include <QDebug>
#include "customframe/standarditem.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

StandardItemModel::StandardItemModel(QObject *parent):
    QStandardItemModel(parent)
{
    setHorizontalHeaderLabels(QStringList() << "选择" << "串口号" << "地址" << "负载压强(KPa)" << "多项式");
}

bool StandardItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.parent().isValid() && index.column() == 0)    //复选框Item
    {
        StandardItem *item = static_cast<StandardItem *>(this->itemFromIndex(index));
        switch (role) {
        case Qt::UserRole:
        case Qt::UserRole+1:
        {
            bool is_checked = (((Qt::CheckState)value.toInt()) == Qt::Checked);
            item->setIsChecked(is_checked);

            emit dataChanged(index,index);
            if(role == Qt::UserRole+1)
            {
                onStateChanged();
            }
            return true;
        }
            break;
        default:
            break;
        }
    }
    return QStandardItemModel::setData(index,value,role);
}

QVariant StandardItemModel::data(const QModelIndex &index, int role) const
{
    int column = index.column();
    switch (role) {
    case Qt::UserRole:
        if(!index.parent().isValid() && column == 0)
        {
            StandardItem * item = static_cast<StandardItem *>(itemFromIndex(index));
            return item->getIsChecked()?Qt::Checked:Qt::Unchecked;
        }
        break;
    case Qt::TextAlignmentRole:
        if(column == 0 || column == 4)
            return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
        else
            return Qt::AlignCenter;
        break;
    case Qt::TextColorRole:
        return QColor(Qt::black);
        break;
    case Qt::SizeHintRole:
            return QSize(110,25);
        break;
    case Qt::FontRole:
        return QFont("SimSun", 10);
        break;
    default:
        break;
    }
    return QStandardItemModel::data(index,role);
}

QVariant StandardItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::FontRole:
        return QFont("SimSun", 10);
        break;
    case Qt::TextAlignmentRole:
        if(section == 0)
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        else
            return Qt::AlignCenter;
        break;
    case Qt::TextColorRole:
        return QColor(Qt::black);
        break;
    case Qt::SizeHintRole:
            return QSize(100,30);
        break;
    case Qt::BackgroundRole:
        return QBrush(Qt::gray);
        break;
    default:
        break;
    }
    return QStandardItemModel::headerData(section,orientation,role);
}

void StandardItemModel::slot_stateChanged(Qt::CheckState state)
{
    for(int i = 0;i < invisibleRootItem()->rowCount();++i)
    {
        setData(index(i,0),state,Qt::UserRole);
    }
}

void StandardItemModel::onStateChanged()
{
    int select_total = 0;
    for(int i = 0;i < invisibleRootItem()->rowCount();++i)
    {
        StandardItem *item = static_cast<StandardItem *>(invisibleRootItem()->child(i));
        if(item->getIsChecked())
            ++select_total;
    }

    if(select_total == 0)
    {
        emit stateChanged(Qt::Unchecked);
    }else if(select_total < rowCount())
    {
        emit stateChanged(Qt::PartiallyChecked);
    }else
    {
        emit stateChanged(Qt::Checked);
    }
}
