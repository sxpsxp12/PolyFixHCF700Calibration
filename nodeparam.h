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
    ~NodeParam(){}
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
    QMap<nodePayload,QList<node_Data>> getNodedatas() const;
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
    void appendnodeData(nodePayload payload,node_Data data);

    void clearUpData();
    void clearDownData();
    void clearNodeDatas();
    void clearPolyFactor();

    void removeUpPayloadLastData(nodePayload payload);
    void removeDownPayloadLastData(nodePayload payload);
    void removeNodePayloadLastData(nodePayload payload);

    void setNodeSoftVersion(quint16 version);
    quint16 getNodeSoftVersion() const;

    void setNodeSelectedStatus(bool is_selected);
    bool getNodeSelectedStatus() const;

    void setNodeDataMinTime(uint time);
    uint getNodeDataMinTime() const;

    void setNodeDataMaxTime(uint time);
    uint getNodeDataMaxTime() const;

    void setSerialNumber(QString sn);
    QString getSerialNumber() const;

private:
    QSerialPortInfo m_port;                 //该设备连接的port
    QString m_serialNumber;         //SN号
    quint16 m_node_addr;          //设备地址
    quint16 m_softversion;       //软件版本
    quint16 m_node_range;        //设备量程
    double m_node_calibrationTemp;  //设备校准温度值
    QByteArray m_node_receiveData;

    QMap<nodePayload,QList<node_Data>> m_upData;   //升温数据(共6种负载)
    QMap<nodePayload,QList<node_Data>> m_downData; //降温数据(共6种负载)
    QMap<nodePayload,QList<node_Data>> m_node_datas; //节点数据(共6种负载)
    QMap<nodePayload,std::vector<double>> m_mapPayloadAndpolyFactor;   //多项式系数

    Status m_status;        //当前节点状态
    bool m_is_selected;

    uint m_minDataTime;     //记录数据库数据的最大最小值
    uint m_maxDataTime;
};

typedef QList<QSharedPointer<NodeParam> > nodeShareedPtrParamList;


#endif // NODEPARAM_H
