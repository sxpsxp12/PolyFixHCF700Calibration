#ifndef DBOPERATE_H
#define DBOPERATE_H

#include <QThread>
#include "customframe/tableviewmodel.h"
#include <QSqlDatabase>
#include "sensorglobal.h"
#include <QMutex>
#include "nodeparam.h"

struct DB_Info{
    DB_Info(){}
    DB_Info(QString sn){
        db = QSqlDatabase::addDatabase("QSQLITE",sn);
    }
    QSqlDatabase db;
    QString serial_number;
    quint16 addr;
    quint16 version;
    quint16 range;
};

struct DB_OPInstruct{
    QString sn;
    QString query_str;
 };

class DBOperate : public QThread
{
    Q_OBJECT
public:
    explicit DBOperate(QObject *parent = Q_NULLPTR);
    ~DBOperate(){}
    static QString DBFileDir;

    void setStartThread(bool run);
    void init_DBs(QList<TableDataOfNodeList> db_list);
    void close_DBs();
    void addDBTableOperate(QString sn, QString op);
    QString getDBTableStrFromPayload(nodePayload payload);
    void commitTransaction(bool commit);

    void getNodeDatasTimeRangeFromDB(QSharedPointer<NodeParam> node);
    void getNodeDatasFromStartTimeToEndTime(QSharedPointer<NodeParam> node, uint start_time, uint end_time, nodeDataType data_type=DATA_NONE);

protected:
    virtual void run() Q_DECL_OVERRIDE;

private:
    bool m_is_run;
    QMap<QString,DB_Info> m_mapDataBaseNameAndDBInfo;
    QList<DB_OPInstruct> m_op_instructList;
    QMutex m_op_mutex;
    bool m_commit_transaction;

    void _getDBOfDBDir();
    void _createTable(DB_Info &db_info);
};

#endif // DBOPERATE_H
