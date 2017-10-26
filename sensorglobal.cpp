#include "sensorglobal.h"
#include <QMap>
#include "WinSock2.h"
#include <QDebug>

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

quint16 crc16(const quint8 *buf, int start, int length)
{
    int i, j;
    quint16 msbinfo, base_crc = 0xFFFF;

    for (i = start; i < start + length; i++) {
        base_crc ^= buf[i];
        for (j = 0; j < 8; j++) {
            msbinfo = base_crc & 0x0001;
            base_crc >>= 1;
            if (msbinfo != 0)
                base_crc ^= 0xA001;
        }
    }
    return base_crc;
}

float ntohf(float val)
{
    float res;
    char *p = (char*)&val, *p_res = (char*)&res;
    *p_res = *(p+3);
    *(p_res+1) = *(p+2);
    *(p_res+2) = *(p+1);
    *(p_res+3) = *p;
    return res;
}

QString getStrPayLoad(nodePayload payload)
{
    QMap<nodePayload,QString> map_payLoadAndStr;
    map_payLoadAndStr.insert(FULL_PAYLOAD,"满量程负载");
    map_payLoadAndStr.insert(FOUR_FIFTH_PAYLOAD,"五分之四量程负载");
    map_payLoadAndStr.insert(THREE_FIFTH_PAYLOAD,"五分之三量程负载");
    map_payLoadAndStr.insert(TWO_FIFTH_PAYLOAD,"五分之二量程负载");
    map_payLoadAndStr.insert(ONE_FIFTH_PAYLOAD,"五分之一量程负载");
    map_payLoadAndStr.insert(EMPTY_PAYLOAD,"零负载");

    return map_payLoadAndStr[payload];
}
/**
 *  请求节点量程和校准温度
 * @brief sendRequestRange
 * @param serial
 */
void sendRequestRange(int addr,QSerialPort *serial)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FuncCode_ReadHoldingRegister;
    *((quint16*)p) = htons(0x06);
    p += 2;
    *((quint16*)p) = htons(20);
    p += 2;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;

//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }
    serial->write(buf, p-buf);
}
/**
 * @brief sendRequestSN
 * @param addr
 * @param serial
 */
void sendRequestSN(int addr,QSerialPort *serial)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FunCode_ReadSN;
    *p++ = 0x44;
    *p++ = 0x66;
    *p++ = 0x88;
    *p++ = 0xaa;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;

//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }
    serial->write(buf, p-buf);
}


/**
 *  节点采样
 * @brief sendNodeSample
 * @param addr
 * @param serial
 */
void sendNodeSample(int addr,QSerialPort *serial)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FuncCode_Sample;
    *p++ = 0x44;
    *p++ = 0x66;
    *p++ = 0x88;
    *p++ = 0xaa;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;
//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("sendNodeSample:%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }
    int length = serial->write(buf, p-buf);
    qDebug() << "sendNodeSample write length:" << length;
}

void sendRequestNodeTemp(int addr,QSerialPort *serial)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FuncCode_ReadInputRegister;
    *((quint16*)p) = htons(0x0008);
    p += 2;
    *((quint16*)p) = htons(2);
    p += 2;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;

//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("sendRequestNodeData:%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }

    int length = serial->write(buf, p-buf);
    qDebug() << "sendRequestNodeTemp write length:" << length;
}

/**
 *  清空多项式系数指令
 * @brief sendClearFactor
 * @param addr
 * @param serial
 */
void sendClearFactor(int addr,QSerialPort *serial)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FunCode_ClearFactor;
    *p++ = 0x44;
    *p++ = 0x66;
    *p++ = 0x88;
    *p++ = 0xaa;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;
//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("sendNodeSample:%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }

    int length = serial->write(buf, p-buf);
    qDebug() << "sendClearFactor write length:" << length;
}
/**
 *  发送请求数据指令，可以获取温度值和压强值
 * @brief sendRequestNodeData
 * @param addr
 * @param serial
 */
void sendRequestNodeData(int addr,QSerialPort *serial)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FuncCode_ReadInputRegister;
    *((quint16*)p) = htons(0);
    p += 2;
    *((quint16*)p) = htons(16);
    p += 2;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;

//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("sendRequestNodeData:%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }

    int length = serial->write(buf, p-buf);
    qDebug() << "sendRequestNodeData write length:" << length;
}

/**
 *  请求节点设备软件版本
 * @brief sendRequestNodeSoftVertion
 * @param addr
 * @param serial
 */
void sendRequestNodeSoftVertion(int addr,QSerialPort *serial)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FuncCode_ReadInputRegister;
    *((quint16*)p) = htons(0x01);
    p += 2;
    *((quint16*)p) = htons(1);
    p += 2;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;

//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("sendRequestNodeSoftVertion:%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }

    int length = serial->write(buf, p-buf);
    qDebug() << "sendRequestNodeSoftVertion write length:" << length;
}

void sendSetPayloadFactor(int addr,QSerialPort *serial,nodePayload payload,int index,float value)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FuncCode_SetMultiHoldingRegister;

    switch (payload) {
    case FULL_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x4C);
        else if(index == 1)
            *((quint16*)p) = htons(0x4E);
        else if(index == 2)
            *((quint16*)p) = htons(0x50);
        else if(index == 3)
            *((quint16*)p) = htons(0x52);
        else if(index == 4)
            *((quint16*)p) = htons(0x54);
        break;
    case FOUR_FIFTH_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x42);
        else if(index == 1)
            *((quint16*)p) = htons(0x44);
        else if(index == 2)
            *((quint16*)p) = htons(0x46);
        else if(index == 3)
            *((quint16*)p) = htons(0x48);
        else if(index == 4)
            *((quint16*)p) = htons(0x4A);
        break;
    case THREE_FIFTH_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x38);
        else if(index == 1)
            *((quint16*)p) = htons(0x3A);
        else if(index == 2)
            *((quint16*)p) = htons(0x3C);
        else if(index == 3)
            *((quint16*)p) = htons(0x3E);
        else if(index == 4)
            *((quint16*)p) = htons(0x40);
        break;
    case TWO_FIFTH_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x2E);
        else if(index == 1)
            *((quint16*)p) = htons(0x30);
        else if(index == 2)
            *((quint16*)p) = htons(0x32);
        else if(index == 3)
            *((quint16*)p) = htons(0x34);
        else if(index == 4)
            *((quint16*)p) = htons(0x36);
        break;
    case ONE_FIFTH_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x24);
        else if(index == 1)
            *((quint16*)p) = htons(0x26);
        else if(index == 2)
            *((quint16*)p) = htons(0x28);
        else if(index == 3)
            *((quint16*)p) = htons(0x2A);
        else if(index == 4)
            *((quint16*)p) = htons(0x2C);
        break;
    case EMPTY_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x1A);
        else if(index == 1)
            *((quint16*)p) = htons(0x1C);
        else if(index == 2)
            *((quint16*)p) = htons(0x1E);
        else if(index == 3)
            *((quint16*)p) = htons(0x20);
        else if(index == 4)
            *((quint16*)p) = htons(0x22);
        break;
    default:
        break;
    }
    p += 2;
    *((quint16*)p) = htons(0x02);
    p += 2;
    *p++ = 0x04;
    *((float*)p) = ntohf(value);
    p += 4;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;

//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("sendSetPayloadFactor:%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }

    int length = serial->write(buf, p-buf);
    qDebug() << "sendSetPayloadFactor write length:" << length;
}

void sendReadPayloadFactor(int addr,QSerialPort *serial,nodePayload payload,int index)
{
    char buf[128];
    memset(buf,0,128);
    char *p=buf;

    *p++ = (char)addr;
    *p++ = FuncCode_ReadHoldingRegister;

    switch (payload) {
    case FULL_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x4C);
        else if(index == 1)
            *((quint16*)p) = htons(0x4E);
        else if(index == 2)
            *((quint16*)p) = htons(0x50);
        else if(index == 3)
            *((quint16*)p) = htons(0x52);
        else if(index == 4)
            *((quint16*)p) = htons(0x54);
        break;
    case FOUR_FIFTH_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x42);
        else if(index == 1)
            *((quint16*)p) = htons(0x44);
        else if(index == 2)
            *((quint16*)p) = htons(0x46);
        else if(index == 3)
            *((quint16*)p) = htons(0x48);
        else if(index == 4)
            *((quint16*)p) = htons(0x4A);
        break;
    case THREE_FIFTH_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x38);
        else if(index == 1)
            *((quint16*)p) = htons(0x3A);
        else if(index == 2)
            *((quint16*)p) = htons(0x3C);
        else if(index == 3)
            *((quint16*)p) = htons(0x3E);
        else if(index == 4)
            *((quint16*)p) = htons(0x40);
        break;
    case TWO_FIFTH_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x2E);
        else if(index == 1)
            *((quint16*)p) = htons(0x30);
        else if(index == 2)
            *((quint16*)p) = htons(0x32);
        else if(index == 3)
            *((quint16*)p) = htons(0x34);
        else if(index == 4)
            *((quint16*)p) = htons(0x36);
        break;
    case ONE_FIFTH_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x24);
        else if(index == 1)
            *((quint16*)p) = htons(0x26);
        else if(index == 2)
            *((quint16*)p) = htons(0x28);
        else if(index == 3)
            *((quint16*)p) = htons(0x2A);
        else if(index == 4)
            *((quint16*)p) = htons(0x2C);
        break;
    case EMPTY_PAYLOAD:
        if(index == 0)
            *((quint16*)p) = htons(0x1A);
        else if(index == 1)
            *((quint16*)p) = htons(0x1C);
        else if(index == 2)
            *((quint16*)p) = htons(0x1E);
        else if(index == 3)
            *((quint16*)p) = htons(0x20);
        else if(index == 4)
            *((quint16*)p) = htons(0x22);
        break;
    default:
        break;
    }
    p += 2;
    *((quint16*)p) = htons(0x02);
    p += 2;
    *((quint16*)p) = crc16((quint8*)buf, 0, p-buf);
    p += 2;

//    for(int i = 0;i < p-buf;++i)
//    {
//        qDebug()<< QString("sendSetPayloadFactor:%1").arg((int)(buf[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }

    int length = serial->write(buf, p-buf);
    qDebug() << "sendReadPayloadFactor write length:" << length;
}


