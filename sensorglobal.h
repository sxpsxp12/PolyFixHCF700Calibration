#ifndef SENSORGLOBAL_H
#define SENSORGLOBAL_H

#include "qglobal.h"
#include <QSerialPort>

typedef enum{
    FuncCode_ReadHoldingRegister=0x03,//读取多个保持寄存器
    FuncCode_ReadInputRegister=0x04,//读取多个输入寄存器
    FuncCode_SetHoldingRegister=0x06,//设置单个保持寄存器
    FuncCode_SetMultiHoldingRegister=0x10,//设置多个保持寄存器
    FuncCode_SetZero=0x41,//HCF700设置初始液位
    FuncCode_Sample=0x42,//HCF700采样
    FuncCode_ZeroCalibration=0x44,//HCF700零压力校准
    FuncCode_FullCalibration=0x43,//HCF700满量程校准
    FuncCode_Reset=0x68,//HCF700回复出厂设置
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

typedef struct{
    uint unix_time;         //数据接收时间
    quint16 addr;           //节点地址
    double temp;            //温度
    double pressure;        //实际压强
    double ref_pressure;    //参照压强
    double div_pressure;    //压强偏差
}node_Data;

typedef enum{
    FULL_PAYLOAD,
    FOUR_FIFTH_PAYLOAD,
    THREE_FIFTH_PAYLOAD,
    TWO_FIFTH_PAYLOAD,
    ONE_FIFTH_PAYLOAD,
    EMPTY_PAYLOAD
}nodePayload;

typedef enum{
    CALIBRATION_SAMPLE, //标定采样
    CALIBRATION_POLYFIT //多项式拟合
}nodeCalibrationStatus;

quint16 crc16(const quint8 *buf, int start, int length);
float ntohf(float val);
QString getStrPayLoad(nodePayload payload);
void sendRequestRange(int addr, QSerialPort *serial);
void sendNodeSample(int addr,QSerialPort *serial);
void sendRequestNodeData(int addr,QSerialPort *serial);
void sendSetPayloadFactor(int addr, QSerialPort *serial, nodePayload payload, int index, float value);

#endif // SENSORGLOBAL_H
