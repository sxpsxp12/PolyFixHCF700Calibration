#include "nodeparam.h"
#include <QDebug>

NodeParam::NodeParam()
{

}

NodeParam::NodeParam(QSerialPortInfo port)
{
    m_node_addr = 0;
    m_node_calibrationTemp = 0;
    m_port = port;
    m_status = Status_None;
    m_node_receiveData.clear();
    m_softversion = 0;
    m_is_selected = false;
    m_serialNumber.clear();

    m_minDataTime = 0;
    m_maxDataTime = 0;
}

NodeParam::NodeParam(const NodeParam &other)
{
    m_port = other.m_port;
    m_node_addr = other.m_node_addr;
    m_softversion = other.m_softversion;
    m_node_range = other.m_node_range;
    m_status = other.m_status;
    m_node_calibrationTemp = other.m_node_calibrationTemp;
    m_node_receiveData = other.m_node_receiveData;
    m_is_selected = other.m_is_selected;
    m_minDataTime = other.m_minDataTime;
    m_maxDataTime = other.m_maxDataTime;
    m_serialNumber = other.m_serialNumber;

    m_upData = other.m_upData;
    m_downData = other.m_downData;
    m_node_datas = other.m_node_datas;
    m_mapPayloadAndpolyFactor = other.m_mapPayloadAndpolyFactor;
}

bool NodeParam::operator ==(const NodeParam &other) const
{
    if(m_serialNumber == other.m_serialNumber)
        return true;
    return false;
}

QSerialPortInfo NodeParam::getNodePort() const
{
    return m_port;
}

quint16 NodeParam::getNodeAddr() const
{
    return m_node_addr;
}

quint16 NodeParam::getNodeRange() const
{
    return m_node_range;
}

Status NodeParam::getNodeStatus() const
{
    return m_status;
}

QMap<nodePayload, QList<node_Data> > NodeParam::getUpdata() const
{
    return m_upData;
}

QMap<nodePayload, QList<node_Data> > NodeParam::getDowndata() const
{
    return m_downData;
}

QMap<nodePayload, QList<node_Data> > NodeParam::getNodedatas() const
{
    return m_node_datas;
}

QMap<nodePayload, std::vector<double> > NodeParam::getPolyFactorMap()
{
    return m_mapPayloadAndpolyFactor;
}

std::vector<double> NodeParam::getPayloadPolyFactor(nodePayload payload) const
{
    return m_mapPayloadAndpolyFactor[payload];
}

QString NodeParam::getPolyFactorStr(nodePayload payload) const
{
    std::vector<double> factor = m_mapPayloadAndpolyFactor.value(payload);

    QString strFun("y="),strTemp("");

    for(int i = 0;i < factor.size();++i)
    {
        if (0 == i)
        {
            strTemp = QString("%1").arg(factor.at(i));
        }
        else
        {

            double fac = factor.at(i);
            if(fac < 0)
                strTemp = QString("%1x^%2").arg(fac).arg(i);
            else
                strTemp = QString("+%1x^%2").arg(fac).arg(i);
        }
        strFun += strTemp;
    }
    return strFun;
}

double NodeParam::getNodeCalibrationTemp() const
{
    return m_node_calibrationTemp;
}

QByteArray NodeParam::getNodeReceiveData() const
{
    return m_node_receiveData;
}

double NodeParam::getFactorComputeY(nodePayload payload, double x)
{
    double ans=0;
    for (int i=0;i<m_mapPayloadAndpolyFactor[payload].size();++i)
    {
        ans += m_mapPayloadAndpolyFactor[payload][i]*pow(x,i);
    }
    return ans;
}

void NodeParam::setNodeAddr(quint16 addr)
{
    m_node_addr = addr;
}

void NodeParam::setNodeRange(quint16 range)
{
    m_node_range = range;
}

void NodeParam::setNodeStatus(Status status)
{
    m_status = status;
}

void NodeParam::setPolyFactor(nodePayload payload,std::vector<double> factor)
{
    m_mapPayloadAndpolyFactor[payload] = factor;
}

void NodeParam::setNodeCalibrationTemp(double temp)
{
    m_node_calibrationTemp = temp;
}

void NodeParam::setNodeReceiveData(QByteArray data)
{
    m_node_receiveData = data;
}

void NodeParam::appendUpData(nodePayload payload, node_Data data)
{
    m_upData[payload].append(data);
}

void NodeParam::appendDownData(nodePayload payload, node_Data data)
{
    m_downData[payload].append(data);
}

void NodeParam::appendnodeData(nodePayload payload, node_Data data)
{
    m_node_datas[payload].append(data);
}

void NodeParam::clearUpData()
{
    m_upData.clear();
}

void NodeParam::clearDownData()
{
    m_downData.clear();
}

void NodeParam::clearNodeDatas()
{
    m_node_datas.clear();
}

void NodeParam::clearPolyFactor()
{
    m_mapPayloadAndpolyFactor.clear();
}

void NodeParam::removeUpPayloadLastData(nodePayload payload)
{
    if(!m_upData[payload].isEmpty())
        m_upData[payload].removeLast();
}

void NodeParam::removeDownPayloadLastData(nodePayload payload)
{
    if(!m_downData[payload].isEmpty())
        m_downData[payload].removeLast();
}

void NodeParam::removeNodePayloadLastData(nodePayload payload)
{
    if(!m_node_datas[payload].isEmpty())
        m_node_datas[payload].removeLast();
}

void NodeParam::setNodeSoftVersion(quint16 version)
{
    m_softversion = version;
}

quint16 NodeParam::getNodeSoftVersion() const
{
    return m_softversion;
}

void NodeParam::setNodeSelectedStatus(bool is_selected)
{
    m_is_selected = is_selected;
}

bool NodeParam::getNodeSelectedStatus() const
{
    return m_is_selected;
}

void NodeParam::setNodeDataMinTime(uint time)
{
    m_minDataTime = time;
}

uint NodeParam::getNodeDataMinTime() const
{
    return m_minDataTime;
}

void NodeParam::setNodeDataMaxTime(uint time)
{
    m_maxDataTime = time;
}

uint NodeParam::getNodeDataMaxTime() const
{
    return m_maxDataTime;
}

void NodeParam::setSerialNumber(QString sn)
{
    m_serialNumber = sn;
}

QString NodeParam::getSerialNumber() const
{
    return m_serialNumber;
}

NodeParam &NodeParam::operator=(const NodeParam &other)
{
    if(this == &other)
        return *this;
    m_port = other.m_port;
    m_node_addr = other.m_node_addr;
    m_softversion = other.m_softversion;
    m_node_range = other.m_node_range;
    m_status = other.m_status;
    m_node_calibrationTemp = other.m_node_calibrationTemp;
    m_node_receiveData = other.m_node_receiveData;
    m_minDataTime = other.m_minDataTime;
    m_maxDataTime = other.m_maxDataTime;
    m_serialNumber = other.m_serialNumber;

    m_upData = other.m_upData;
    m_downData = other.m_downData;
    m_node_datas = other.m_node_datas;
    m_mapPayloadAndpolyFactor = other.m_mapPayloadAndpolyFactor;
    m_is_selected = other.m_is_selected;

    return *this;
}
