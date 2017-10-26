#include "dboperate.h"
#include <QStandardPaths>
#include <QSqlQuery>
#include <QDir>
#include <QStringList>
#include <QString>
#include <QDebug>
#include <QtConcurrent>
#include <QSqlError>

QString DBOperate::DBFileDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/SensorCamellia/DB";
DBOperate::DBOperate(QObject *parent):
    QThread(parent),m_is_run(false)
{
    m_mapDataBaseNameAndDBInfo.clear();
}

void DBOperate::setStartThread(bool run)
{
    m_is_run = run;
}

void DBOperate::init_DBs(QList<TableDataOfNodeList> db_list)
{
    //获取当前路径下的数据库
    _getDBOfDBDir();

    for(int i = 0;i < db_list.count();++i)
    {
        if(m_mapDataBaseNameAndDBInfo.contains(db_list.at(i).serial_number))
        {
            DB_Info &info = m_mapDataBaseNameAndDBInfo[db_list.at(i).serial_number];
            info.addr = db_list.at(i).node_addr;
            info.serial_number = db_list.at(i).serial_number;
            info.version = db_list.at(i).softversion;
            info.range = db_list.at(i).node_range;
            QFuture<void> future = QtConcurrent::run(this,&DBOperate::_createTable,info);
            while(!future.isFinished())
                QCoreApplication::processEvents();
            info.db.transaction();  //开启事务
        }else
        {
            DB_Info info(db_list.at(i).serial_number);
            info.addr = db_list.at(i).node_addr;
            info.serial_number = db_list.at(i).serial_number;
            info.version = db_list.at(i).softversion;
            info.range = db_list.at(i).node_range;
            info.db.setDatabaseName(QString("%1/%2").arg(DBFileDir).arg(info.serial_number));
            info.db.open();
            QFuture<void> future = QtConcurrent::run(this,&DBOperate::_createTable,info);
            while(!future.isFinished())
                QCoreApplication::processEvents();
            m_mapDataBaseNameAndDBInfo.insert(info.serial_number,info);
            info.db.transaction();  //开启事务
        }
    }
}

void DBOperate::close_DBs()
{
    for(int i =0;i < m_mapDataBaseNameAndDBInfo.count();++i)
    {
        m_mapDataBaseNameAndDBInfo.values()[i].db.commit();
        m_mapDataBaseNameAndDBInfo.values()[i].db.close();
    }
    m_mapDataBaseNameAndDBInfo.clear();
}

void DBOperate::addDBTableOperate(QString sn, QString op)
{
    QMutexLocker locker(&m_op_mutex);

    DB_OPInstruct op_instruct;
    op_instruct.sn = sn;
    op_instruct.query_str = op;
    m_op_instructList.append(op_instruct);
}

//获取操作的数据库表
QString DBOperate::getDBTableStrFromPayload(nodePayload payload)
{
    QString table_name="";
    switch (payload) {
    case FULL_PAYLOAD:
        table_name = QString("full_payload");
        break;
    case FOUR_FIFTH_PAYLOAD:
        table_name = QString("four_fifth_payload");
        break;
    case THREE_FIFTH_PAYLOAD:
        table_name = QString("three_fifth_payload");
        break;
    case TWO_FIFTH_PAYLOAD:
        table_name = QString("two_fifth_payload");
        break;
    case ONE_FIFTH_PAYLOAD:
        table_name = QString("one_fifth_payload");
        break;
    case EMPTY_PAYLOAD:
        table_name = QString("zero_payload");
        break;
    default:
        table_name = QString("");
        break;
    }
    return table_name;
}

void DBOperate::commitTransaction(bool commit)
{
    m_commit_transaction = commit;
}

void DBOperate::getNodeDatasTimeRangeFromDB(QSharedPointer<NodeParam> node)
{
    DB_Info &info = m_mapDataBaseNameAndDBInfo[node->getSerialNumber()];
    QSqlQuery query(info.db);
    uint query_time=0;
    QFuture<bool> future;
    QVector<nodePayload> payloadVec={FULL_PAYLOAD,FOUR_FIFTH_PAYLOAD,THREE_FIFTH_PAYLOAD,TWO_FIFTH_PAYLOAD,ONE_FIFTH_PAYLOAD,EMPTY_PAYLOAD};

    node->setNodeDataMinTime(0);
    node->setNodeDataMaxTime(0);
    node->clearNodeDatas();

    for(int i = 0;i < payloadVec.count();++i)
    {
        //查询最大时间
        query_time = 0;
        future = QtConcurrent::run(&query,&QSqlQuery::exec,QString("select max(unix_time) from `%1`;").arg(getDBTableStrFromPayload(payloadVec[i])));
        while(!future.isFinished())
            QCoreApplication::processEvents();
        if(query.next())
        {
            query_time = query.value(0).toUInt();
        }
        if(query_time != 0)
        {
            if(node->getNodeDataMaxTime() == 0)
            {
                node->setNodeDataMaxTime(query_time);
            }else
            {
                if(node->getNodeDataMaxTime() < query_time)
                    node->setNodeDataMaxTime(query_time);
            }
        }
        query.finish();

        //查询最小时间
        query_time = 0;
        future = QtConcurrent::run(&query,&QSqlQuery::exec,QString("select min(unix_time) from `%1`;").arg(getDBTableStrFromPayload(payloadVec[i])));
        while(!future.isFinished())
            QCoreApplication::processEvents();
        if(query.next())
        {
            query_time = query.value(0).toUInt();
        }
        if(query_time != 0)
        {
            if(node->getNodeDataMinTime() == 0)
            {
                node->setNodeDataMinTime(query_time);
            }else
            {
                if(node->getNodeDataMinTime() > query_time)
                    node->setNodeDataMinTime(query_time);
            }
        }
        query.finish();
    }
}

void DBOperate::getNodeDatasFromStartTimeToEndTime(QSharedPointer<NodeParam> node, uint start_time, uint end_time,nodeDataType data_type)
{
    DB_Info &info = m_mapDataBaseNameAndDBInfo[node->getSerialNumber()];
    QSqlQuery query(info.db);
    node_Data data;
    QVector<nodePayload> payloadVec={FULL_PAYLOAD,FOUR_FIFTH_PAYLOAD,THREE_FIFTH_PAYLOAD,TWO_FIFTH_PAYLOAD,ONE_FIFTH_PAYLOAD,EMPTY_PAYLOAD};

    if(data_type == DATA_UP)
        node->clearUpData();
    else if(data_type == DATA_DOWN)
        node->clearDownData();
    else
        node->clearNodeDatas();

    for(int i = 0;i < payloadVec.count();++i)
    {
        QFuture<bool> future = QtConcurrent::run(&query,&QSqlQuery::exec,QString("select * from `%1` where unix_time >= %2 AND unix_time <= %3;").arg(getDBTableStrFromPayload(payloadVec[i]))
                                                 .arg(start_time).arg(end_time));
        while(!future.isFinished())
            QCoreApplication::processEvents();
        while(query.next())
        {
            data.addr = node->getNodeAddr();
            data.serial_number = node->getSerialNumber();
            data.payload_type = payloadVec[i];
            data.unix_time = query.value(0).toDouble();
            data.temp = query.value(1).toDouble();
            data.pressure = query.value(2).toDouble();
            data.ref_pressure = query.value(3).toDouble();
            data.div_pressure = query.value(4).toDouble();
            if(data_type == DATA_UP)
                node->appendUpData(payloadVec[i],data);
            else if(data_type == DATA_DOWN)
                node->appendDownData(payloadVec[i],data);
            else
                node->appendnodeData(payloadVec[i],data);
        }
        query.finish();
    }
}

void DBOperate::run()
{
    while(m_is_run)
    {
        if(m_op_instructList.isEmpty())
        {
            QThread::msleep(50);
            continue;
        }

        DB_OPInstruct op_instruct;
        {
            QMutexLocker locker(&m_op_mutex);
            op_instruct = m_op_instructList.first();
            m_op_instructList.removeFirst();
        }

        QSqlDatabase &db = m_mapDataBaseNameAndDBInfo[op_instruct.sn].db;
        QSqlQuery query(db);
        query.exec(op_instruct.query_str);

        if(m_commit_transaction)
        {
            m_commit_transaction = false;
            for(int i =0;i < m_mapDataBaseNameAndDBInfo.count();++i)
            {
                m_mapDataBaseNameAndDBInfo.values()[i].db.commit();
                m_mapDataBaseNameAndDBInfo.values()[i].db.transaction();
            }
        }

        QCoreApplication::processEvents();
    }

    close_DBs();
}
//从数据库路径下遍历数据库
void DBOperate::_getDBOfDBDir()
{
    QDir dir(DBFileDir);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    if(!dir.exists(DBFileDir))  //创建目录
        dir.mkdir(DBFileDir);
    QFileInfoList file_list = dir.entryInfoList();
    for(int i = 0;i < file_list.count();)
    {
        QFileInfo info = file_list.at(i);
        if(info.fileName().contains("journal"))
        {
            QFile::remove(info.absoluteFilePath());
            ++i;
            continue;
        }

        DB_Info db_info(info.fileName());
        db_info.db.setDatabaseName(info.absoluteFilePath());
        db_info.db.open();

        if(db_info.db.tables().isEmpty())
        {
            db_info.db.close();
            QFile::remove(info.absoluteFilePath());
            ++i;
            continue;
        }

        QSqlQuery query(db_info.db);
        if(query.exec("select * from `node_info`;") && query.next())
        {
            db_info.serial_number = query.value(0).toString();
            db_info.addr = query.value(1).toUInt();
            db_info.version = query.value(2).toUInt();
            db_info.range = query.value(3).toUInt();
            m_mapDataBaseNameAndDBInfo.insert(info.baseName(),db_info);
        }
        ++i;
    }
}

void DBOperate::_createTable(DB_Info &db_info)
{
    QStringList query_list;
    query_list.clear();
    query_list.append("CREATE TABLE IF NOT EXISTS node_info(    \
                      sn VARCHAR(20) UNIQUE,    \
                      addr int, \
                      version int,  \
                      range int \
                  );");
    query_list.append("CREATE TABLE IF NOT EXISTS full_payload( \
                      unix_time double UNIQUE,  \
                      temp double,  \
                      pressure double,  \
                      ref_pressure double,  \
                      div_pressure double   \
                  );");
    query_list.append("CREATE TABLE IF NOT EXISTS four_fifth_payload(   \
                      unix_time double UNIQUE,  \
                      temp double,  \
                      pressure double,  \
                      ref_pressure double,  \
                      div_pressure double   \
                  ); ");
    query_list.append("CREATE TABLE IF NOT EXISTS three_fifth_payload(  \
                      unix_time double UNIQUE,  \
                      temp double,  \
                      pressure double,  \
                      ref_pressure double,  \
                      div_pressure double   \
                  ); ");
    query_list.append("CREATE TABLE IF NOT EXISTS two_fifth_payload(    \
                      unix_time double UNIQUE,  \
                      temp double,  \
                      pressure double,  \
                      ref_pressure double,  \
                      div_pressure double   \
                  ); ");
    query_list.append("CREATE TABLE IF NOT EXISTS one_fifth_payload(    \
                      unix_time double UNIQUE,  \
                      temp double,  \
                      pressure double,  \
                      ref_pressure double,  \
                      div_pressure double   \
                  ); ");
    query_list.append("CREATE TABLE IF NOT EXISTS zero_payload( \
                      unix_time double UNIQUE,  \
                      temp double,  \
                      pressure double,  \
                      ref_pressure double,  \
                      div_pressure double   \
                  ); ");

    query_list.append(QString("insert or replace into `node_info` values('%1',%2,%3,%4);").arg(db_info.serial_number).arg(db_info.addr).arg(db_info.version).arg(db_info.range));
    db_info.db.transaction();   //开启事务
    QSqlQuery query(db_info.db);
    for(int i = 0;i < query_list.count();++i)
    {
        if(!query.exec(query_list.at(i)))
        {
            qDebug() << "create database fail:" << db_info.db.lastError();
        }
    }
    db_info.db.commit();
}
