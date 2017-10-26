#ifndef TABLEVIEWMODEL_H
#define TABLEVIEWMODEL_H

#include <QAbstractTableModel>
#include "sensorglobal.h"

class TableViewModel:public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TableViewModel(TableType type=TABLE_NONE,QObject *parent=NULL);

    void setColumnCount(int count);
    void setHeaderLabels(QStringList labels);
    void updateNodeListData(QList<TableDataOfNodeList> list);
    void updateSampleDataList(QList<node_Data> list,bool is_clear=true);
    void clear();
    void removeNodeOfTable(QSerialPortInfo info,quint16 addr);
    node_Data getSampleDataByRow(int row,bool &is_ok);

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;

signals:
    void stateChanged(Qt::CheckState state);
    void signal_changeNodeSelectedStatus(QSerialPortInfo port_info,quint16 addr,bool status);
private slots:
    void slot_stateChanged(Qt::CheckState state);
private:
    QList<TableDataOfNodeList> m_nodelist;
    QList<node_Data> m_sampledatalist;
    int m_column_count;
    QStringList m_headerLabels;
    TableType m_table_type;

    void onStateChanged();

    enum{
        CHECK_BOX_COLUMN = 0
    };
};

#endif // TABLEVIEWMODEL_H
