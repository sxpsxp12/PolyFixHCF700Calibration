#ifndef NODEPARAM_H
#define NODEPARAM_H

#include <qglobal.h>
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#include <QPair>
#include <QSerialPortInfo>
#include "sensorglobal.h"
#include <QMap>
#include <QList>

class NodeParam
{
public:
    NodeParam();
    NodeParam(QSerialPortInfo port);
    NodeParam(const NodeParam &other);
    NodeParam& operator=(const NodeParam &other);

    bool operator==(const NodeParam &other) const;

    QSerialPortInfo getNodePort() const;
    quint16 getNodeAddr() const;
    quint16 getNodeRange() const;
    Status getNodeStatus() const;
    QMap<nodePayload,QList<node_Data>> getUpdata() const;
    QMap<nodePayload,QList<node_Data>> getDowndata() const;
    QMap<nodePayload,std::vector<double>> getPolyFactorMap();
    std::vector<double> getPayloadPolyFactor(nodePayload payload) const;
    QString getPolyFactorStr(nodePayload payload) const;
    double getNodeCalibrationTemp() const;
    QByteArray getNodeReceiveData() const;

    double getFactorComputeY(nodePayload payload,double x);

    void setNodeAddr(quint16 addr);
    void setNodeRange(quint16 range);
    void setNodeStatus(Status status);
    void setPolyFactor(nodePayload payload, std::vector<double> factor);
    void setNodeCalibrationTemp(double temp);
    void setNodeReceiveData(QByteArray data);

    void appendUpData(nodePayload payload,node_Data data);
    void appendDownData(nodePayload payload,node_Data data);

    void clearUpData();
    void clearDownData();
private:
    QSerialPortInfo m_port;                 //该设备连接的port
    quint16 m_node_addr;          //设备地址
    QString m_softversion;       //软件版本
    quint16 m_node_range;        //设备量程
    double m_node_calibrationTemp;  //设备校准温度值
    QByteArray m_node_receiveData;

    QMap<nodePayload,QList<node_Data>> m_upData;   //升温数据(共6种负载)
    QMap<nodePayload,QList<node_Data>> m_downData; //降温数据(共6种负载)
    QMap<nodePayload,std::vector<double>> m_mapPayloadAndpolyFactor;   //多项式系数

    Status m_status;        //当前节点状态
};

typedef QList<NodeParam> nodeParamList;


#endif // NODEPARAM_H
