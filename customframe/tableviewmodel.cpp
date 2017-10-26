#include "tableviewmodel.h"
#include <QDateTime>
#include <QStringList>
#include <QString>
#include <QColor>
#include <QSize>
#include <QBrush>
#include <QFont>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

TableViewModel::TableViewModel(TableType type, QObject *parent):
    QAbstractTableModel(parent),m_column_count(0),m_table_type(type)
{
    m_nodelist.clear();
}

void TableViewModel::setColumnCount(int count)
{
    m_column_count = count;
}

void TableViewModel::setHeaderLabels(QStringList labels)
{
    m_headerLabels = labels;
}

void TableViewModel::updateNodeListData(QList<TableDataOfNodeList> list)
{
    m_nodelist = list;

    beginResetModel();
    endResetModel();

    emit stateChanged(Qt::Unchecked);
}

//清空更新和不清空更新两种模式
void TableViewModel::updateSampleDataList(QList<node_Data> list, bool is_clear)
{
    if(is_clear)
        m_sampledatalist = list;
    else
        m_sampledatalist.append(list);

    beginResetModel();
    endResetModel();

    emit stateChanged(Qt::Unchecked);
}

void TableViewModel::clear()
{
    m_nodelist.clear();
    m_sampledatalist.clear();

    beginResetModel();
    endResetModel();

    emit stateChanged(Qt::Unchecked);
}

void TableViewModel::removeNodeOfTable(QSerialPortInfo info, quint16 addr)
{
    int i = 0;
    for(i = 0;i < m_nodelist.count();++i)
    {
        if(m_nodelist.at(i).portinfo.portName() == info.portName()
                && m_nodelist.at(i).node_addr == addr)
            break;
    }
    removeRow(i);
}

node_Data TableViewModel::getSampleDataByRow(int row, bool &is_ok)
{
    if(row >= m_sampledatalist.count())
    {
        is_ok = false;
        return node_Data();
    }

    is_ok = true;
    return m_sampledatalist.at(row);
}

int TableViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    if(m_table_type == TABLE_NODELIST)
    {
        return m_nodelist.count();
    }else if(m_table_type == TABLE_NODEDATA)
    {
        return m_sampledatalist.count();
    }else
        return 0;
}

int TableViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_column_count;
}


QVariant TableViewModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    TableDataOfNodeList index_data;
    node_Data sample_data;
    int row = index.row();
    int column = index.column();
    if(m_table_type == TABLE_NODELIST)
         index_data = m_nodelist.at(row);
    else if(m_table_type == TABLE_NODEDATA)
        sample_data = m_sampledatalist.at(row);

    switch (role) {
    case Qt::DisplayRole:
        if(m_table_type == TABLE_NODELIST)
        {
            if(column == 1)
                return index_data.portinfo.portName();
            else if(column == 2)
                return index_data.node_addr;
            else if(column == 3)
                return index_data.serial_number;
            else if(column == 4)
                return index_data.node_range;
            else if(column == 5)
                return QString("%1.%2.%3").arg(index_data.softversion/100).arg(index_data.softversion%100/10).arg(index_data.softversion%100%10);
        }else if(m_table_type == TABLE_NODEDATA)
        {
            if(column == 1)
                return QDateTime::fromTime_t(sample_data.unix_time).toString("yyyy-MM-dd hh:mm:ss");
            else if(column == 2)
                return getStrPayLoad(sample_data.payload_type);
            else if(column == 3)
                return sample_data.temp;
            else if(column == 4)
                return sample_data.pressure;
            else if(column == 5)
                return sample_data.ref_pressure;
            else if(column == 6)
                return sample_data.div_pressure;
        }

        return QString("");
        break;
    case Qt::UserRole:
        if(m_table_type == TABLE_NODELIST)
        {
            if(column == CHECK_BOX_COLUMN)
                return index_data.is_be_checked?Qt::Checked:Qt::Unchecked;
        }else if(m_table_type == TABLE_NODEDATA)
        {
            if(column == CHECK_BOX_COLUMN)
                return sample_data.is_be_checked?Qt::Checked:Qt::Unchecked;
        }

        break;
    case Qt::TextAlignmentRole:
        if(column == CHECK_BOX_COLUMN)
            return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
        else
            return Qt::AlignCenter;
        break;
    case Qt::TextColorRole:
        return QColor(Qt::black);
        break;
    case Qt::SizeHintRole:
        if(m_table_type == TABLE_NODELIST)
            return QSize(110,25);
        else if(m_table_type == TABLE_NODEDATA)
            return QSize(160,25);
        else
            return QSize(160,25);
        break;
    case Qt::FontRole:
        return QFont("SimSun", 10);
        break;
    default:
        break;
    }
    return QVariant();
}

bool TableViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    int column = index.column();
    TableDataOfNodeList index_data;
    node_Data sample_data;

    if(m_table_type == TABLE_NODELIST)
         index_data = m_nodelist.at(index.row());
    else if(m_table_type == TABLE_NODEDATA)
        sample_data = m_sampledatalist.at(index.row());

    switch (role) {
    case Qt::UserRole:
    case Qt::UserRole+1:  //根据表头的复选框选择

        if(m_table_type == TABLE_NODELIST && column == CHECK_BOX_COLUMN)
        {
            index_data.is_be_checked = (((Qt::CheckState)value.toInt()) == Qt::Checked);
            m_nodelist.replace(index.row(),index_data);

            emit dataChanged(index,index);
            emit signal_changeNodeSelectedStatus(index_data.portinfo,index_data.node_addr,index_data.is_be_checked);

            if(role == Qt::UserRole+1)  //点击鼠标，更新表头复选框状态
                onStateChanged();

            return true;
        }else if(m_table_type == TABLE_NODEDATA && column == CHECK_BOX_COLUMN)
        {
            sample_data.is_be_checked = (((Qt::CheckState)value.toInt()) == Qt::Checked);
            m_sampledatalist.replace(index.row(),sample_data);

            emit dataChanged(index,index);

            if(role == Qt::UserRole+1)  //点击鼠标，更新表头复选框状态
                onStateChanged();

            return true;
        }
        break;
    default:
        return false;
        break;
    }
    return false;
}

//可选
Qt::ItemFlags TableViewModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (index.isValid())
        flags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    return flags;
}

//表头
QVariant TableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        if(m_headerLabels.count() > section)
            return m_headerLabels.at(section);
        else
            return QString("");

        break;
    case Qt::FontRole:
        return QFont("SimSun", 10);
        break;
    case Qt::TextAlignmentRole:
        if(section == CHECK_BOX_COLUMN)
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        else
            return Qt::AlignCenter;
        break;
    case Qt::TextColorRole:
        return QColor(Qt::black);
        break;
    case Qt::SizeHintRole:
        if(m_table_type == TABLE_NODELIST)
            return QSize(120,30);
        else if(m_table_type == TABLE_NODEDATA)
            return QSize(160,30);
        else
            return QSize(160,30);
        break;
    case Qt::BackgroundRole:
        return QBrush(Qt::gray);
        break;
    default:
        break;
    }
    return QVariant();
}

bool TableViewModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent,row,row+count-1);
    for(int i = 0;i < count;i++)
    {
        if(m_table_type == TABLE_NODELIST)
            m_nodelist.insert(row,TableDataOfNodeList());
        else if(m_table_type == TABLE_NODEDATA)
            m_sampledatalist.insert(row,node_Data());
    }
    endInsertRows();
    return true;
}

bool TableViewModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent,row,row+count-1);
    for(int i = 0;i < count;++i)
    {
        if(m_table_type == TABLE_NODELIST)
            m_nodelist.removeAt(row);
        else if(m_table_type == TABLE_NODEDATA)
            m_sampledatalist.removeAt(row);
    }
    endRemoveRows();
    return true;
}

void TableViewModel::slot_stateChanged(Qt::CheckState state)
{
   for(int i = 0;i < rowCount();++i)
   {
       setData(index(i,CHECK_BOX_COLUMN),state,Qt::UserRole);
   }
}

void TableViewModel::onStateChanged()
{
    int select_total = 0;
    for(int i = 0;i < rowCount();++i)
    {
        if(m_table_type == TABLE_NODELIST)
        {
            if(m_nodelist.at(i).is_be_checked)
                ++select_total;
        }else if(m_table_type == TABLE_NODEDATA)
        {
            if(m_sampledatalist.at(i).is_be_checked)
                ++select_total;
        }
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
