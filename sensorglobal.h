#ifndef SENSORGLOBAL_H
#define SENSORGLOBAL_H

#include "qglobal.h"
#include <QSerialPort>
#include <QSerialPortInfo>

#define NODESOFTVERSION 411

typedef enum{
    FuncCode_ReadHoldingRegister=0x03,//读取多个保持寄存器
    FuncCode_ReadInputRegister=0x04,//读取多个输入寄存器
    FuncCode_SetHoldingRegister=0x06,//设置单个保持寄存器
    FuncCode_SetMultiHoldingRegister=0x10,//设置多个保持寄存器
    FunCode_ReadSN = 0x40,  //读取SN号
    FuncCode_SetZero=0x41,//HCF700设置初始液位
    FuncCode_Sample=0x42,//HCF700采样
    FuncCode_FullCalibration=0x43,//HCF700满量程校准
    FuncCode_ZeroCalibration=0x44,//HCF700零压力校准
    FunCode_ClearFactor = 0x49,  //清空多项式系数
    FuncCode_Reset=0x68//HCF700回复出厂设置
} FuncCode;

typedef enum{
    Status_None,
    Status_QueryHoldingRegister,//读取多个保持寄存器
    Status_QueryInputRegister,//读取多个输入寄存器
    Status_QueryHFS316HoldingRegister,
    Status_QueryHFS316InputRegister,
    Status_Sample,//HCF700采样
    Status_HFS316Sample,
    Status_SetZero,//HCF700设置初始液位
    Status_ZeroCalibration,//HCF700零压力校准
    Status_FullCalibration,//HCF700满量程校准
    Status_SetSensorFull,
    Status_SetAddr,//HCF700设置地址
    Status_SetRange,//HCF700设置量程
    Status_SetHeight,//HCF700设置初始液位高度
    Status_SetDensity,//HCF700设置液体密度
    Status_SetGravity,//HCF700设置重力加速度
    Status_SetAdcFull,//HCF700设置ADC满量程数值
    Status_SetAdcZero,//HCF700设置ADC零压力数值
    Status_Reset,//HCF700回复出厂设置
} Status;

typedef enum{
    FULL_PAYLOAD,
    FOUR_FIFTH_PAYLOAD,
    THREE_FIFTH_PAYLOAD,
    TWO_FIFTH_PAYLOAD,
    ONE_FIFTH_PAYLOAD,
    EMPTY_PAYLOAD
}nodePayload;

struct node_Data{
    bool is_be_checked = false;
    uint unix_time;         //数据接收时间
    nodePayload payload_type = EMPTY_PAYLOAD;
    QString serial_number;
    quint16 addr;           //节点地址
    double temp;            //温度
    double pressure;        //实际压强
    double ref_pressure;    //参照压强
    double div_pressure;    //压强偏差
    bool operator<(const node_Data &other)
    {
        if(unix_time < other.unix_time)
            return true;
        return false;
    }
};

typedef enum{
    TABLE_NONE,
    TABLE_NODELIST, //节点展示表
    TABLE_NODEDATA, //数据编辑表
    TABLE_POLYFIT   //拟合数据展示表
}TableType;

typedef enum{
    CALIBRATION_SAMPLE, //标定采样
    CALIBRATION_POLYFIT //多项式拟合
}nodeCalibrationStatus;

typedef enum{
    DATA_NONE,
    DATA_UP,
    DATA_DOWN
}nodeDataType;

//节点列表的表格展示数据
struct TableDataOfNodeList{
    TableDataOfNodeList():is_be_checked(false){
    }

    bool operator ==(const TableDataOfNodeList &other)
    {
        if(serial_number == other.serial_number)
            return true;
        return false;
    }
    bool is_be_checked;
    QSerialPortInfo portinfo;        //该设备连接的port
    QString serial_number;
    quint16 node_addr;          //设备地址
    quint16 node_range;        //设备量程
    quint16 softversion;       //软件版本
};

quint16 crc16(const quint8 *buf, int start, int length);
float ntohf(float val);
QString getStrPayLoad(nodePayload payload);
void sendRequestRange(int addr, QSerialPort *serial);
void sendNodeSample(int addr,QSerialPort *serial);
void sendRequestNodeData(int addr,QSerialPort *serial);
void sendRequestNodeTemp(int addr,QSerialPort *serial);
void sendSetPayloadFactor(int addr, QSerialPort *serial, nodePayload payload, int index, float value);
void sendClearFactor(int addr,QSerialPort *serial);
void sendReadPayloadFactor(int addr,QSerialPort *serial,nodePayload payload,int index);
void sendRequestNodeSoftVertion(int addr,QSerialPort *serial);
void sendRequestSN(int addr,QSerialPort *serial);
#endif // SENSORGLOBAL_H
