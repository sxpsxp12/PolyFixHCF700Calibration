#include "nodeparam.h"

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

    m_upData = other.m_upData;
    m_downData = other.m_downData;
    m_mapPayloadAndpolyFactor = other.m_mapPayloadAndpolyFactor;
}

bool NodeParam::operator ==(const NodeParam &other) const
{
    if(m_port.portName() == other.m_port.portName() && m_node_addr == other.m_node_addr && m_node_range == other.m_node_range)
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
            strTemp = QString::number(factor.at(i),'f',6);
        }
        else
        {

            double fac = factor.at(i);
            if(fac < 0)
                strTemp = QString("%1x^%2").arg(fac,0,'f',6).arg(i);
            else
                strTemp = QString("+%1x^%2").arg(fac,0,'f',6).arg(i);
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

void NodeParam::clearUpData()
{
    m_upData.clear();
}

void NodeParam::clearDownData()
{
    m_downData.clear();
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

    m_upData = other.m_upData;
    m_downData = other.m_downData;
    m_mapPayloadAndpolyFactor = other.m_mapPayloadAndpolyFactor;

    return *this;
}
