#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include "WinSock2.h"
#include <QElapsedTimer>
#include <QDebug>
#include <QDateTime>
#include <windows.h>
#include <QMessageBox>
#include <QFile>
#include <QString>
#include <QStringList>
#include "fitting.h"
#include <QStandardPaths>
#include "calibrationDisplay/calibrationdisplay.h"
#include <QDesktopWidget>

#define TIMEROUT 5      //倒计时时间

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _initWindow();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::_initWindow()
{
    m_MapPortAndNodeParam.clear();
    m_currentPayload = FULL_PAYLOAD;
    m_currentCalibrationStatus = CALIBRATION_SAMPLE;    //标定采样状态
    m_total_timerout = TIMEROUT; //3分钟定时
    isUp = true;
    ui->progressBar->hide();
    ui->pbt_calibration->setVisible(false);

    //设备展示
    ui->treeWidget->setColumnCount(4);
    ui->treeWidget->headerItem()->setTextAlignment(0,Qt::AlignHCenter);
    ui->treeWidget->headerItem()->setTextAlignment(1,Qt::AlignHCenter);
    ui->treeWidget->headerItem()->setTextAlignment(2,Qt::AlignHCenter);
    ui->treeWidget->headerItem()->setTextAlignment(3,Qt::AlignHCenter);
    ui->treeWidget->setHeaderLabels(QStringList() <<"串口号"<< "设备地址" << "设备量程(KPA)" << "标定温度(℃)");

    //采样数据展示
    ui->treeWidgetSampleData->setColumnCount(8);
    for(int i = 0; i < 8;++i)
    {
        ui->treeWidgetSampleData->headerItem()->setTextAlignment(i,Qt::AlignHCenter);
    }
    ui->treeWidgetSampleData->setColumnWidth(0,150);
    ui->treeWidgetSampleData->setHeaderLabels(QStringList() << "时间" << "串口号" << "地址" << "温度(℃)" <<"实际压强(PA)" << "负载压强(KPA)"<<"压强偏差(PA)"<<"当前负载数据量");

    //拟合结果展示
    ui->treeWidget_Polyfit->setColumnCount(4);
    for(int i = 0; i < 4;++i)
    {
        ui->treeWidget_Polyfit->headerItem()->setTextAlignment(i,Qt::AlignHCenter);
    }
    ui->treeWidget_Polyfit->setColumnWidth(0,80);
    ui->treeWidget_Polyfit->setColumnWidth(1,80);
    ui->treeWidget_Polyfit->setColumnWidth(2,80);
    ui->treeWidget_Polyfit->setHeaderLabels(QStringList()<< "串口号" << "节点"<<"负载量程"<<"多项式系数");

    //定时校验节点掉线
    connect(&m_checkPortDrop,&QTimer::timeout,this,&MainWindow::slot_nodeDropCheck);
    connect(&m_timerout,&QTimer::timeout,this,&MainWindow::slot_timer_out);
    connect(&m_opTimer,&QTimer::timeout,this,&MainWindow::slot_op_timer_out);

    ui->stackedWidget->setCurrentWidget(ui->pageNodeList);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    while(m_MapPortAndNodeParam.count())
    {
        QSerialPort *port = m_MapPortAndNodeParam.firstKey();
        port->close();
        m_MapPortAndNodeParam.remove(port);
        delete port;
        port = NULL;
    }
    m_MapPortAndNodeParam.clear();
    e->accept();
}

/**
 *  节点掉线检测
 * @brief MainWindow::slot_nodeDropCheck
 */
void MainWindow::slot_nodeDropCheck()
{
    for(int i = 0;i < m_MapPortAndNodeParam.count();)
    {
        //该串口连接的所有节点都掉线
        nodeParamList param_list = m_MapPortAndNodeParam.values()[i];
        if(!param_list.isEmpty() && !param_list[0].getNodePort().isValid())
        {
            for(int j = 0; j < param_list.count();++j)
            {
                _displayOutInfo(QString("串口%1的节点:%2掉线!").arg(param_list[j].getNodePort().portName()).arg(param_list[j].getNodeAddr()));
            }
            m_MapPortAndNodeParam.remove(m_MapPortAndNodeParam.keys()[i]);
            continue;
        }
        ++i;
        QCoreApplication::processEvents();
    }

    //重新刷新在线节点
    ui->treeWidget->clear();
    QMap<QSerialPort *,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        for(int i = 0;i < iter.value().count();++i)
        {
            _addNodeParamToView(iter.value()[i]);
            QCoreApplication::processEvents();
        }
        ++iter;
    }
}

void MainWindow::on_pbt_connectScan_clicked()
{
    ui->statusBar->showMessage(QString("连接新串口并完成节点的扫描可能需要一段时间，请耐心等待..."),0);

    //首先连接所有的串口
    int availablePort = QSerialPortInfo::availablePorts().count();
    ui->progressBar->setMaximum(availablePort*254);
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    ui->centralWidget->setEnabled(false);
    m_checkPortDrop.stop();

    for(int i = 0;i < availablePort;++i)
    {
        QSerialPortInfo info = QSerialPortInfo::availablePorts().at(i);
        _displayOutInfo(QString("串口:%1,isBusy:%2").arg(info.portName()).arg(info.isBusy()==true?"true":"false"));
        if(info.isBusy())       //避开已经连接的节点
        {
            ui->progressBar->setValue(ui->progressBar->value()+254);
            continue;
        }

        QSerialPort *serial = new QSerialPort;
        serial->setPortName(info.portName());
        if(!serial->open(QIODevice::ReadWrite))
        {
            ui->progressBar->setValue(ui->progressBar->value()+254);
            delete serial;
            serial = NULL;
            continue;
        }
        serial->setBaudRate(QSerialPort::Baud9600);
        serial->setDataBits(QSerialPort::Data8);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);
        serial->setParity(QSerialPort::NoParity);

        //扫描该节点信息
        nodeParamList param_list;
        param_list.clear();
        if(_scanNodeInfo(serial,param_list,info))
        {
            m_MapPortAndNodeParam.insert(serial,param_list);
            for(int index = 0;index < param_list.count();++index)
            {
                //界面展示
                _addNodeParamToView(param_list.at(index));
            }
        }else
        {
            serial->close();
            delete serial;
            serial = NULL;
        }
    }

    ui->centralWidget->setEnabled(true);
    ui->progressBar->setVisible(false);
    m_checkPortDrop.start(3000);
    ui->statusBar->showMessage(QString("设备连接并扫描完毕!"),5000);
}

//扫描节点信息，可以获取到地址和量程
bool MainWindow::_scanNodeInfo(QSerialPort *serial,nodeParamList &param_list,QSerialPortInfo info)
{
    bool is_ok = false;
    quint16 node_range=0;
    quint16 node_addr=0;
    double node_calibration_temp = 0;
    //1-255地址依次扫描新节点，以确定该节点的地址
    for(int addr = 1;addr < 255;++addr)
    {
        //直接查询该地址的量程
        sendRequestRange(addr,serial);

        serial->waitForReadyRead(50);
        if(serial->bytesAvailable())
        {
            QByteArray array;
            bool time_out = false;
            QTimer t;
            connect(&t,&QTimer::timeout,this,[&](){
               time_out = true;
            });
            t.start(2000);

            while(!time_out && array.count() < 45)
            {
                serial->waitForReadyRead(50);
                array.append(serial->readAll());
                QCoreApplication::processEvents();
            }
            t.stop();
            if(!time_out && !array.isEmpty())
            {
                char * data = array.data();
                node_addr = addr;
                node_range = *((quint8*)data+3)*256+*((quint8*)data+4);
                node_calibration_temp = ntohf(*((float*)(data+39)));

                NodeParam param(info);
                param.setNodeAddr(node_addr);
                param.setNodeRange(node_range);
                param.setNodeCalibrationTemp(node_calibration_temp);
                param_list.append(param);

                is_ok = true;
            }else
            {
                _displayOutInfo(QString("串口:%1请求超时").arg(info.portName()));
            }
        }

        ui->progressBar->setValue(ui->progressBar->value()+1);
        QElapsedTimer t;    //相邻数据请求，最小时间5ms
        t.start();
        while(t.elapsed()<150)
            QCoreApplication::processEvents();
    }
    return is_ok;
}

void MainWindow::_displayOutInfo(QString info)
{
    QString context = QString("<b>%1</b>").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    context.append(">>>");
    context.append(info);
    ui->textBrowser->append(context);
}


/**
 *计算参照压强
 * @brief MainWindow::_getRangescale
 * @return
 */
double MainWindow::_getReferencePresure(nodePayload payload ,quint16 range)
{
    if(payload == FULL_PAYLOAD)
    {
        return range;
    }else if(payload == FOUR_FIFTH_PAYLOAD)
    {
        return range*4/5.0;
    }else if(payload == THREE_FIFTH_PAYLOAD)
    {
        return range*3/5.0;
    }else if(payload == TWO_FIFTH_PAYLOAD)
    {
        return range*2/5.0;
    }else if(payload == ONE_FIFTH_PAYLOAD)
    {
        return range*1/5.0;
    }else if(payload == EMPTY_PAYLOAD)
    {
        return 0;
    }
    return range;
}

//数据包超时定时器
void MainWindow::slot_op_timer_out()
{
    if(m_currentCalibrationStatus == CALIBRATION_SAMPLE)
    {
        _displayOutInfo(QString("等待节点数据超时,自动忽略该数据包!"));
        QTimer::singleShot(1000,this,[&](){     //进行下一数据包的请求
            m_MapPortAndNodeParam[m_MapPortAndNodeParam.keys()[m_current_port]][m_current_node].setNodeStatus(Status_None);
            m_MapPortAndNodeParam[m_MapPortAndNodeParam.keys()[m_current_port]][m_current_node].setNodeReceiveData(QByteArray());

            //请求下一节点的数据
            m_current_node++;
            if(m_current_node >= m_MapPortAndNodeParam.values()[m_current_port].count())
            {
                ++m_current_port;
                m_current_node=0;
            }
            _requestNodeData();
        });
    }
    m_opTimer.stop();
}

/*********************************HCF700节点展示界面********************/

void MainWindow::on_pbt_startCalibrationSample_clicked()
{
    if(m_MapPortAndNodeParam.isEmpty())
    {
        ui->statusBar->showMessage(QString("当前没有连接的设备,请先连接设备!"),5000);
        return;
    }


    if(ui->pbt_calibration->isVisible())
    {
        int ret = QMessageBox::information(this,"提示","重新开始标定采样将会使之前的标定数据丢失，确认重新采样?",QMessageBox::Yes,QMessageBox::Cancel);
        if(ret == QMessageBox::Yes)
        {
        }else if(ret == QMessageBox::Cancel){
            return;
        }
    }

    //初始化节点升温数据接收容器
    QMap<QSerialPort*,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        nodeParamList &param_list = iter.value();
        for(int i = 0;i < param_list.count();++i)
        {
            param_list[i].clearUpData();
            param_list[i].clearDownData();
        }
        ++iter;
    }

    m_currentCalibrationStatus = CALIBRATION_SAMPLE;    //标定采样状态
    ui->pbt_connectScan->setEnabled(false);
    m_currentPayload = FULL_PAYLOAD;
    ui->labelCurrentStatus->setText(QString("升温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
    ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

    isUp = true;
    m_current_port = 0;    //当前请求串口的索引
    m_current_node = 0;     //请求当前节点的索引
    _displayCurrentPayloadSampleData();
}


void MainWindow::_addNodeParamToView(const NodeParam &param)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0,param.getNodePort().portName());
    item->setText(1,QString::number(param.getNodeAddr()));
    item->setText(2,QString::number(param.getNodeRange()));
    item->setText(3,QString::number(param.getNodeCalibrationTemp()));

    item->setTextAlignment(0,Qt::AlignHCenter);
    item->setTextAlignment(1,Qt::AlignHCenter);
    item->setTextAlignment(2,Qt::AlignHCenter);
    item->setTextAlignment(3,Qt::AlignHCenter);

    ui->treeWidget->scrollToItem(item);

}

/*******************************标定采样过程界面**************************/

//读数据
void MainWindow::slot_serialDataComming()
{
    QSerialPort *serial = qobject_cast<QSerialPort *>(sender());
    QByteArray receiveData = m_MapPortAndNodeParam[serial][m_current_node].getNodeReceiveData();
    receiveData.append(serial->readAll());
    m_MapPortAndNodeParam[serial][m_current_node].setNodeReceiveData(receiveData);

    _displayOutInfo(QString("串口%1的节点%2接收的总数据量:%3").arg(m_MapPortAndNodeParam[serial][m_current_node].getNodePort().portName())
                    .arg(m_MapPortAndNodeParam[serial][m_current_node].getNodeAddr()).arg(m_MapPortAndNodeParam[serial][m_current_node].getNodeReceiveData().count()));

//    qDebug() << "addr:" << m_MapPortAndNodeParam[serial][m_current_node].getNodeAddr() << ";" << "size:" << m_MapPortAndNodeParam[serial][m_current_node].getNodeReceiveData().count()
//             << ";status:" << m_MapPortAndNodeParam[serial][m_current_node].getNodeStatus();
//    qDebug() << "-------------------------------";
//    for(int i = 0;i < m_MapPortAndNodeParam[serial][m_current_node].getNodeReceiveData().count();++i)
//    {
//        qDebug()<< QString("%1").arg((int)(m_MapPortAndNodeParam[serial][m_current_node].getNodeReceiveData()[i]&0xff),2,16,QLatin1Char('0')).toUpper();
//    }
//    qDebug() << "*******************************";

    if(m_MapPortAndNodeParam[serial][m_current_node].getNodeStatus()==Status_QueryInputRegister && receiveData.length()>=37)
    {
        m_opTimer.stop();

        const char *result = receiveData.data();
        float height_residue,pressure,temperature;
        quint16 crc;

        height_residue = ntohf(*((float*)(result+15)));
        temperature = ntohf(*((float*)(result+19)));
        pressure = ntohf(*((float*)(result+31)));

        crc = *((quint16*)(result+35));

        if(crc != crc16((quint8*)result, 0, 35))
        {
            _displayOutInfo(QString("串口%1的节点 %2 CRC校验失败，自动忽略该数据包!").arg(m_MapPortAndNodeParam[serial][m_current_node].getNodePort().portName())
                            .arg(m_MapPortAndNodeParam[serial][m_current_node].getNodeAddr()));
        }
        else
        {
//            pressure = height_residue*10;
            node_Data data;
            data.addr = m_MapPortAndNodeParam[serial][m_current_node].getNodeAddr();
            data.unix_time = QDateTime::currentDateTime().toTime_t();
            data.temp = temperature;
            data.pressure = pressure;
            data.ref_pressure = _getReferencePresure(m_currentPayload,m_MapPortAndNodeParam[serial][m_current_node].getNodeRange());
            data.div_pressure = pressure-(data.ref_pressure*1000);
            if(isUp)    //升温数据
            {
                m_MapPortAndNodeParam[serial][m_current_node].appendUpData(m_currentPayload,data);
            }else       //降温数据
            {
                m_MapPortAndNodeParam[serial][m_current_node].appendDownData(m_currentPayload,data);
            }
            _displayOutInfo(QString("串口%1的节点%2数据采集成功!").arg(m_MapPortAndNodeParam[serial][m_current_node].getNodePort().portName())
                            .arg(m_MapPortAndNodeParam[serial][m_current_node].getNodeAddr()));
        }
        m_MapPortAndNodeParam[serial][m_current_node].setNodeStatus(Status_None);
        m_MapPortAndNodeParam[serial][m_current_node].setNodeReceiveData(QByteArray());

        disconnect(serial,&QSerialPort::readyRead,this,&MainWindow::slot_serialDataComming);

        //请求下一节点的数据
        m_current_node++;
        if(m_current_node >= m_MapPortAndNodeParam[serial].count())
        {
            ++m_current_port;
            m_current_node=0;
        }
        _requestNodeData();

    }else if(m_MapPortAndNodeParam[serial][m_current_node].getNodeStatus()==Status_None)
    {
        m_MapPortAndNodeParam[serial][m_current_node].setNodeReceiveData(QByteArray());
    }
}

//请求节点数据
void MainWindow::_requestNodeData()
{
    ui->progressBarRequestSampleData->setValue(ui->progressBarRequestSampleData->value()+1);
    if(m_current_port < m_MapPortAndNodeParam.count() && m_current_node < m_MapPortAndNodeParam.values()[m_current_port].count())
    {
        m_MapPortAndNodeParam[m_MapPortAndNodeParam.keys()[m_current_port]][m_current_node].setNodeStatus(Status_QueryInputRegister);
        m_MapPortAndNodeParam[m_MapPortAndNodeParam.keys()[m_current_port]][m_current_node].setNodeReceiveData(QByteArray());

        connect(m_MapPortAndNodeParam.keys()[m_current_port],&QSerialPort::readyRead,this,&MainWindow::slot_serialDataComming);

        _displayOutInfo(QString("等待串口%1的节点%2数据的返回...").arg(m_MapPortAndNodeParam[m_MapPortAndNodeParam.keys()[m_current_port]][m_current_node].getNodePort().portName())
                        .arg(m_MapPortAndNodeParam[m_MapPortAndNodeParam.keys()[m_current_port]][m_current_node].getNodeAddr()));

        QElapsedTimer t;    //相邻数据请求，最小时间5ms
        t.start();
        while(t.elapsed()<50)
            QCoreApplication::processEvents();

        sendRequestNodeData(m_MapPortAndNodeParam.values()[m_current_port][m_current_node].getNodeAddr(),m_MapPortAndNodeParam.keys()[m_current_port]);
        m_opTimer.start(4000);  //指令超时定时器
    }else   //请求了所有节点的数据，可以进行下一负载的数据请求
    {
        //展示本次负载的数据
        _displayCurrentPayloadSampleData();

        if(m_currentPayload == FULL_PAYLOAD)
        {
            m_currentPayload = FOUR_FIFTH_PAYLOAD;
            if(isUp)
            {
                ui->labelCurrentStatus->setText(QString("升温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }else
            {
                ui->labelCurrentStatus->setText(QString("降温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }
            ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

            m_current_port = 0;    //当前请求的串口
            m_current_node = 0;     //当前请求的节点
        }else if(m_currentPayload == FOUR_FIFTH_PAYLOAD)
        {
            m_currentPayload = THREE_FIFTH_PAYLOAD;
            if(isUp)
            {
                ui->labelCurrentStatus->setText(QString("升温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }else
            {
                ui->labelCurrentStatus->setText(QString("降温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }
            ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

            m_current_port = 0;    //当前请求的串口
            m_current_node = 0;     //当前请求的节点
        }else if(m_currentPayload == THREE_FIFTH_PAYLOAD)
        {
            m_currentPayload = TWO_FIFTH_PAYLOAD;
            if(isUp)
            {
                ui->labelCurrentStatus->setText(QString("升温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }else
            {
                ui->labelCurrentStatus->setText(QString("降温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }
            ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

            m_current_port = 0;    //当前请求的串口
            m_current_node = 0;     //当前请求的节点
        }else if(m_currentPayload == TWO_FIFTH_PAYLOAD)
        {
            m_currentPayload = ONE_FIFTH_PAYLOAD;
            if(isUp)
            {
                ui->labelCurrentStatus->setText(QString("升温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }else
            {
                ui->labelCurrentStatus->setText(QString("降温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }
            ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

            m_current_port = 0;    //当前请求的串口
            m_current_node = 0;     //当前请求的节点
        }else if(m_currentPayload == ONE_FIFTH_PAYLOAD)
        {
            m_currentPayload = EMPTY_PAYLOAD;
            if(isUp)
            {
                ui->labelCurrentStatus->setText(QString("升温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }else
            {
                ui->labelCurrentStatus->setText(QString("降温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }
            ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

            m_current_port = 0;    //当前请求的串口
            m_current_node = 0;     //当前请求的节点
        }else if(m_currentPayload == EMPTY_PAYLOAD)
        {
            m_currentPayload = FULL_PAYLOAD;
            if(isUp)
            {
                ui->labelCurrentStatus->setText(QString("升温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }else
            {
                ui->labelCurrentStatus->setText(QString("降温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
            }
            ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

            m_current_port = 0;    //当前请求的串口
            m_current_node = 0;     //当前请求的节点
        }
    }
}

/**
 *结束标定采样
 * @brief MainWindow::on_pbt_stopCalibration_clicked
 */
void MainWindow::on_pbt_stopCalibrationSample_clicked()
{
    int ret = QMessageBox::information(this,"提示","本次采样标定完成?",QMessageBox::Yes,QMessageBox::Cancel);
    if(ret == QMessageBox::Yes)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageNodeList);
        ui->pbt_calibration->setVisible(true);
        ui->pbt_connectScan->setEnabled(true);
        ui->pbt_DownCalibrationSample->setEnabled(true);
    }else if(ret == QMessageBox::Cancel){}
}

/**
 *  降温标定采样
 * @brief MainWindow::on_pbt_DownCalibrationSample_clicked
 */
void MainWindow::on_pbt_DownCalibrationSample_clicked()
{
    int ret = QMessageBox::information(this,"提示","确定以后，默认确认升温采样完成。确定即将开始降温标定采样吗?",QMessageBox::Yes,QMessageBox::Cancel);
    if(ret == QMessageBox::Yes)
    {
        isUp = false;
        ui->pbt_DownCalibrationSample->setEnabled(false);

        m_currentPayload = FULL_PAYLOAD;
        ui->labelCurrentStatus->setText(QString("降温采样-->%1").arg(getStrPayLoad(m_currentPayload)));
        ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

        m_current_port = 0;    //当前请求的串口
        m_current_node = 0;     //当前请求的节点

        _displayCurrentPayloadSampleData();
    }else if(ret == QMessageBox::Cancel){}
}

//点击下一步，等待3分钟后开始采样
void MainWindow::on_pbt_next_clicked()
{
    ui->progressBarRequestSampleData->setVisible(false);
    ui->pbt_startSample->setVisible(false);
    ui->pbt_startSample->setEnabled(true);
    ui->stackedWidget->setCurrentWidget(ui->pageTimer);
    ui->lcdNumber_minute->display(QString("%1").arg(m_total_timerout/60,2,10,QLatin1Char('0')));
    ui->lcdNumber_second->display(QString("%1").arg(m_total_timerout%60,2,10,QLatin1Char('0')));
    m_timerout.start(1000); //倒计时开始

    QMap<QSerialPort *,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        nodeParamList &param_list = iter.value();
        for(int i = 0; i < param_list.count();++i)
        {
            _displayOutInfo(QString("请调整串口%1的节点%2的负载压强为%3KPa再开始采样...").arg(param_list.at(i).getNodePort().portName())
                            .arg(param_list.at(i).getNodeAddr())
                            .arg(_getReferencePresure(m_currentPayload,param_list.at(i).getNodeRange())));
        }
        ++iter;
    }
}

//定时器倒计时
void MainWindow::slot_timer_out()
{
    if(m_total_timerout)
    {
        m_total_timerout--;
        ui->lcdNumber_minute->display(QString("%1").arg(m_total_timerout/60,2,10,QLatin1Char('0')));
        ui->lcdNumber_second->display(QString("%1").arg(m_total_timerout%60,2,10,QLatin1Char('0')));
    }else
    {
        m_timerout.stop();
        m_total_timerout = TIMEROUT;
        ui->pbt_startSample->setVisible(true);
        ui->progressBarRequestSampleData->setVisible(true);

        int total = 0;
        QMap<QSerialPort*,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
        while(iter != m_MapPortAndNodeParam.end())
        {
            total += iter.value().count();
            ++iter;
        }
        ui->progressBarRequestSampleData->setMaximum(total);
        ui->progressBarRequestSampleData->setValue(0);
    }
}


/**
 *  展示本次负载的数据
 * @brief MainWindow::_displayCurrentPayloadSampleData
 * @param data
 */
void MainWindow::_displayCurrentPayloadSampleData()
{
    ui->treeWidgetSampleData->clear();
    foreach (const nodeParamList &param_list, m_MapPortAndNodeParam.values()) {

        for(int i = 0;i < param_list.count();++i)
        {
            NodeParam param = param_list[i];

            node_Data data;
            int total = 0;
            if(isUp)
            {
                if(param.getUpdata()[m_currentPayload].isEmpty())
                    continue;
                data = param.getUpdata()[m_currentPayload].last();
                total = param.getUpdata()[m_currentPayload].count();
            }else
            {
                if(param.getDowndata()[m_currentPayload].isEmpty())
                    continue;
                data = param.getDowndata()[m_currentPayload].last();
                total = param.getDowndata()[m_currentPayload].count();
            }
            QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidgetSampleData);
            item->setText(0,QDateTime::fromTime_t(data.unix_time).toString("yyyy-MM-dd hh:mm:ss"));
            item->setText(1,param.getNodePort().portName());
            item->setText(2,QString::number(data.addr));
            item->setText(3,QString::number(data.temp));
            item->setText(4,QString::number(data.pressure));
            item->setText(5,QString::number(data.ref_pressure));
            item->setText(6,QString::number(data.div_pressure));
            item->setText(7,QString::number(total));
            for(int i = 0;i < 8;++i)
                item->setTextAlignment(i,Qt::AlignHCenter);
        }
    }
}

void MainWindow::on_pbt_startSample_clicked()
{
    ui->pbt_startSample->setEnabled(false);
    //广播发送采样指令，不需要判断回复指令
    QMap<QSerialPort *,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        _displayOutInfo(QString("串口%1发送广播采样指令...").arg(iter.value().at(0).getNodePort().portName()));
        sendNodeSample(0,iter.key());
        ++iter;
    }

    QElapsedTimer t;    //相邻数据请求，最小时间5ms
    t.start();
    while(t.elapsed()<200)
        QCoreApplication::processEvents();

    //请求节点数据
    _requestNodeData();
}

/*************************************多项式拟合界面************************************/
/**
 *  展示多项式拟合结果
 * @brief MainWindow::_displayPolyfitResult
 */
void MainWindow::_displayPolyfitResult()
{
    ui->treeWidget_Polyfit->clear();

    QMap<QSerialPort *,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        for(int node_index = 0;node_index < iter.value().count();++node_index)
        {
            NodeParam param = iter.value()[node_index];
            QMap<nodePayload,std::vector<double>> poly_factor_map = param.getPolyFactorMap();

            for(int i = 0; i < poly_factor_map.count();++i)
            {
                QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget_Polyfit);
                item->setText(0,param.getNodePort().portName());
                item->setText(1,QString::number(param.getNodeAddr()));
                item->setText(2,QString::number(_getReferencePresure(poly_factor_map.keys()[i],param.getNodeRange())));
                item->setText(3,param.getPolyFactorStr(poly_factor_map.keys()[i]));
                item->setTextAlignment(0,Qt::AlignHCenter);
                item->setTextAlignment(1,Qt::AlignHCenter);
                item->setTextAlignment(2,Qt::AlignHCenter);
                item->setTextAlignment(3,Qt::AlignHCenter);
            }
        }
        ++iter;
    }
}

/**
 *HCF700标定
 * @brief MainWindow::on_pbt_calibration_clicked
 */
void MainWindow::on_pbt_calibration_clicked()
{
    if(m_MapPortAndNodeParam.isEmpty())
    {
        ui->statusBar->showMessage(QString("当前没有连接的设备,请先连接设备!"),5000);
        return;
    }

    ui->pbt_connectScan->setEnabled(false);
    /**测试**/
    //初始化节点升温数据接收容器
//    QMap<QSerialPort*,nodeParamList>::iterator testiter = m_MapPortAndNodeParam.begin();
//    while(testiter != m_MapPortAndNodeParam.end())
//    {
//        nodeParamList &param_list = testiter.value();
//        for(int i = 0;i < param_list.count();++i)
//        {
//            param_list[i].clearUpData();
//            param_list[i].clearDownData();
//        }
//        ++testiter;
//    }
//    _initDatas(EMPTY_PAYLOAD,false,"down_0.csv");
//    _initDatas(ONE_FIFTH_PAYLOAD,false,"down_4.csv");
//    _initDatas(TWO_FIFTH_PAYLOAD,false,"down_8.csv");
//    _initDatas(THREE_FIFTH_PAYLOAD,false,"down_12.csv");
//    _initDatas(FOUR_FIFTH_PAYLOAD,false,"down_16.csv");
//    _initDatas(FULL_PAYLOAD,false,"down_20.csv");

//    _initDatas(EMPTY_PAYLOAD,true,"up_0.csv");
//    _initDatas(ONE_FIFTH_PAYLOAD,true,"up_4.csv");
//    _initDatas(TWO_FIFTH_PAYLOAD,true,"up_8.csv");
//    _initDatas(THREE_FIFTH_PAYLOAD,true,"up_12.csv");
//    _initDatas(FOUR_FIFTH_PAYLOAD,true,"up_16.csv");
//    _initDatas(FULL_PAYLOAD,true,"up_20.csv");

    /******************************/

    QMap<QSerialPort *,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        for(int i = 0;i < iter.value().count();++i)
        {
            NodeParam &param = iter.value()[i];
            if(param.getUpdata().isEmpty() || param.getDowndata().isEmpty())
            {
                _displayOutInfo(QString("存在没有数据的节点，请重新尝试标定采样!"));
                return;
            }
        }
        ++iter;
    }

    m_currentCalibrationStatus = CALIBRATION_POLYFIT;    //标定采样状态
    ui->progressBarPolyfit->setVisible(false);
    ui->pbt_downloadToNode->setEnabled(false);
    ui->pbt_exportCalibrationData->setEnabled(false);
    ui->pbt_calibrationDisplay->setEnabled(false);
    ui->treeWidget_Polyfit->clear();

    ui->stackedWidget->setCurrentWidget(ui->pageCalibration);
}

void MainWindow::_initDatas(nodePayload payload, bool isUp, QString filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        QStringList lineData = QString(line).split(",");
        qDebug() << "lineData:" << lineData;
        node_Data data;
        data.addr = 7;
        data.unix_time = QDateTime::fromString(lineData.at(0),"yyyy/M/d hh:mm").toTime_t();
        data.temp = QString(lineData.at(2)).replace("\n","").toDouble();
        data.pressure = QString(lineData.at(1)).toDouble()*10;
        if(payload == EMPTY_PAYLOAD)
        {
            data.ref_pressure = 0;
            data.div_pressure = data.pressure-data.ref_pressure;
        }else if(payload == ONE_FIFTH_PAYLOAD)
        {
            data.ref_pressure = 4;
            data.div_pressure = data.pressure-(1000*data.ref_pressure);
        }else if(payload == TWO_FIFTH_PAYLOAD)
        {
            data.ref_pressure = 8;
            data.div_pressure = data.pressure-(1000*data.ref_pressure);
        }else if(payload == THREE_FIFTH_PAYLOAD)
        {
            data.ref_pressure = 12;
            data.div_pressure = data.pressure-(1000*data.ref_pressure);
        }else if(payload == FOUR_FIFTH_PAYLOAD)
        {
            data.ref_pressure = 16;
            data.div_pressure = data.pressure-(1000*data.ref_pressure);
        }else if(payload == FULL_PAYLOAD)
        {
            data.ref_pressure = 20;
            data.div_pressure = data.pressure-(1000*data.ref_pressure);
        }

        QMap<QSerialPort*,nodeParamList>::iterator testiter = m_MapPortAndNodeParam.begin();
        while(testiter != m_MapPortAndNodeParam.end())
        {
            nodeParamList &param_list = testiter.value();
            for(int i = 0;i < param_list.count();++i)
            {
                if(isUp)
                {
                    param_list[i].appendUpData(payload,data);
                }else
                {
                    param_list[i].appendDownData(payload,data);
                }
            }
            ++testiter;
        }
    }
}

void MainWindow::on_pbt_returnPageNodeList_clicked()
{
    ui->pbt_connectScan->setEnabled(true);
    ui->stackedWidget->setCurrentWidget(ui->pageNodeList);
}

/**开始拟合***********/
void MainWindow::on_pbt_polyfit_clicked()
{
    ui->progressBarPolyfit->setVisible(true);

    int total = 0;
    QMap<QSerialPort *,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        total += iter.value().count()*6;
        ++iter;
    }
    ui->progressBarPolyfit->setMaximum(total);
    ui->progressBarPolyfit->setValue(0);

    bool isok = true;
    iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        for(int i = 0;i < iter.value().count();++i)
        {
            NodeParam &param = iter.value()[i];

            ui->progressBarPolyfit->setValue(ui->progressBarPolyfit->value()+1);
    //        qDebug() << "addr:" << param.getNodeAddr() << "满量程负载";
            _displayOutInfo(QString("正在串口%1的节点:%2满量程负载标定...").arg(param.getNodePort().portName())
                            .arg(param.getNodeAddr()));
            m_currentPayload = FULL_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo("多项式拟合失败!");
                ui->statusBar->showMessage("多项式拟合失败",5000);
                break;
            }

    //        qDebug() << "addr:" << param.getNodeAddr() << "五分之四量程负载";
            _displayOutInfo(QString("正在串口%1的节点:%2五分之四量程负载标定...").arg(param.getNodePort().portName())
                            .arg(param.getNodeAddr()));
            ui->progressBarPolyfit->setValue(ui->progressBarPolyfit->value()+1);
            m_currentPayload = FOUR_FIFTH_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo("多项式拟合失败!");
                ui->statusBar->showMessage("多项式拟合失败",5000);
                break;
            }

    //        qDebug() << "addr:" << param.getNodeAddr() << "五分之三量程负载";
            _displayOutInfo(QString("正在串口%1的节点:%2五分之三量程负载标定...").arg(param.getNodePort().portName())
                            .arg(param.getNodeAddr()));
            ui->progressBarPolyfit->setValue(ui->progressBarPolyfit->value()+1);
            m_currentPayload = THREE_FIFTH_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo("多项式拟合失败!");
                ui->statusBar->showMessage("多项式拟合失败",5000);
                break;
            }

    //        qDebug() << "addr:" << param.getNodeAddr() << "五分之二量程负载";
            _displayOutInfo(QString("正在串口%1的节点:%2五分之二量程负载标定...").arg(param.getNodePort().portName())
                            .arg(param.getNodeAddr()));
            ui->progressBarPolyfit->setValue(ui->progressBarPolyfit->value()+1);
            m_currentPayload = TWO_FIFTH_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo("多项式拟合失败!");
                ui->statusBar->showMessage("多项式拟合失败",5000);
                break;
            }

    //        qDebug() << "addr:" << param.getNodeAddr() << "五分之一量程负载";
            _displayOutInfo(QString("正在串口%1的节点:%2五分之一量程负载标定...").arg(param.getNodePort().portName())
                            .arg(param.getNodeAddr()));
            ui->progressBarPolyfit->setValue(ui->progressBarPolyfit->value()+1);
            m_currentPayload = ONE_FIFTH_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo("多项式拟合失败!");
                ui->statusBar->showMessage("多项式拟合失败",5000);
                break;
            }

    //        qDebug() << "addr:" << param.getNodeAddr() << "零量程负载";
            _displayOutInfo(QString("正在串口%1的节点:%2零量程负载标定...").arg(param.getNodePort().portName())
                            .arg(param.getNodeAddr()));
            ui->progressBarPolyfit->setValue(ui->progressBarPolyfit->value()+1);
            m_currentPayload = EMPTY_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo("多项式拟合失败!");
                ui->statusBar->showMessage("多项式拟合失败",5000);
                break;
            }
        }
        if(!isok)
            break;
        ++iter;
    }

    ui->progressBarPolyfit->setVisible(false);
    if(isok)    //拟合成功
    {
        ui->pbt_downloadToNode->setEnabled(true);
        ui->pbt_exportCalibrationData->setEnabled(true);
        ui->pbt_calibrationDisplay->setEnabled(true);
        //展示拟合结果
        _displayPolyfitResult();
    }
}

bool MainWindow::_polyFitData(NodeParam &param)
{
    QList<node_Data> upList = param.getUpdata().value(m_currentPayload);
    QList<node_Data> downList = param.getDowndata().value(m_currentPayload);
    if(upList.isEmpty() || downList.isEmpty())
    {
        _displayOutInfo(QString("串口%1的节点%2,升温标定数据或者降温标定数据为空!!!").arg(param.getNodePort().portName()).arg(param.getNodeAddr()));
        return false;
    }
    std::vector<double> factor = _polyFitCurrentPayloadData(upList,downList);
    if(factor.empty())
    {
        _displayOutInfo(QString("串口%1的节点%2多项式拟合失败!").arg(param.getNodePort().portName()).arg(param.getNodeAddr()));
        return false;
    }else
    {
        param.setPolyFactor(m_currentPayload,factor);
    }
    return true;
}

std::vector<double> MainWindow::_polyFitCurrentPayloadData(QList<node_Data> &up_list,QList<node_Data> &down_list)
{
    std::vector<double> temps;
    std::vector<double> pressure;

    //获取升温和降温的数据
    for(int i = 0;i < up_list.count();++i)
    {
        temps.push_back(up_list[i].temp);
        pressure.push_back(up_list[i].pressure);
    }

    for(int i = down_list.count()-1;i >= 0;--i)
    {
        temps.push_back(down_list[i].temp);
        pressure.push_back(down_list[i].pressure);
    }

    //================================曲线拟合
    czy::Fit fit;

    fit.polyfit(temps,pressure,4,false);
    QString strFun("y="),strTemp("");
    for (int i=0;i<fit.getFactorSize();++i)
    {
        if (0 == i)
        {
            strTemp = QString::number(fit.getFactor(i));
        }
        else
        {

            double fac = fit.getFactor(i);
            if(fac < 0)
                strTemp = QString("%1x^%2").arg(fac).arg(i);
            else
                strTemp = QString("+%1x^%2").arg(fac).arg(i);
        }
        strFun += strTemp;
    }
    qDebug() << "升温拟合方程:"<<strFun
             << " ssr:" << fit.getSSR()<< " sse:" <<fit.getSSE()<<
                " rmse:"<<fit.getRMSE()<< " 确定系数:" <<fit.getR_square();

    //获取多项式系数并返回
    std::vector<double> factor;
    fit.getFactor(factor);
    return factor;
}

/**
 *  0.2度为跨度
 * @brief MainWindow::divideTemp
 * @param startTemp
 * @param endTemp
 * @return
 */

std::vector<double> MainWindow::divideTemp(double startTemp, double endTemp)
{
    std::vector<double> value;
    double tmp_value = startTemp;
    while (startTemp < endTemp)
    {
        value.push_back(tmp_value);
        tmp_value += 0.2;
        if(tmp_value>endTemp)
            break;
    }

    return value;
}

/**
 *  标定数据导出
 * @brief MainWindow::on_pbt_downloadCalibrationData_clicked
 */
void MainWindow::on_pbt_exportCalibrationData_clicked()
{
    ui->progressBarPolyfit->setVisible(true);
    int total = 0;
    QMap<QSerialPort *,nodeParamList>::iterator nodeIter = m_MapPortAndNodeParam.begin();
    while(nodeIter != m_MapPortAndNodeParam.end())
    {
        total += nodeIter.value().count()*6;
        ++nodeIter;
    }
    ui->progressBarPolyfit->setMaximum(total);
    ui->progressBarPolyfit->setValue(0);

    QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    nodeIter = m_MapPortAndNodeParam.begin();
    while(nodeIter != m_MapPortAndNodeParam.end())
    {
        for(int node_index = 0; node_index < nodeIter.value().count();++node_index)
        {
            NodeParam &param = nodeIter.value()[node_index];

            QMap<nodePayload,std::vector<double>> poly_factor_map = param.getPolyFactorMap();
            QString filename = path+QString("/%1-%2.csv")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm"))
                    .arg(param.getNodeAddr());
            QFile file(filename);
            if(file.open(QIODevice::WriteOnly|QIODevice::Text))
            {
                QTextStream stream(&file);
                stream.setFieldAlignment(QTextStream::AlignLeft);
                stream << QString("\"节点:%1      \",").arg(param.getNodeAddr()) << QString("\"节点量程(kPA):%1  \",").arg(param.getNodeRange())
                       << QString("\"节点校准温度(℃):%1\"").arg(param.getNodeCalibrationTemp())<<endl;
                for(int i = 0;i < poly_factor_map.keys().count();++i)
                {
                    ui->progressBarPolyfit->setValue(ui->progressBarPolyfit->value()+1);

                    stream << endl;
                    stream << endl;

                    stream << QString("\"节点当前负载(kPA)(%1):%2  \",").arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(_getReferencePresure(poly_factor_map.keys()[i],param.getNodeRange()))
                           << QString("\"多项式拟合公式:%1\"").arg(param.getPolyFactorStr(poly_factor_map.keys()[i])) << endl;

                    stream << QString("\"=======================升温标定采样数据==============\"") << endl;
                    stream.setFieldWidth(30);
                    stream<<QString("\"时间\",") << QString("\"温度(℃)\",") << QString("\"实际压强(PA)\",") << QString("\"负载压强(kPA)\",")
                         << QString("\"压强差(PA)\"") << qSetFieldWidth(0) <<endl;

                    for(int j =0;j < param.getUpdata().value(poly_factor_map.keys()[i]).count();++j)
                    {
                        stream.setFieldWidth(32);
                        node_Data data = param.getUpdata().value(poly_factor_map.keys()[i]).at(j);
                        stream << QString("\"%1\",").arg(QDateTime::fromTime_t(data.unix_time).toString("yyyy-MM-dd hh:mm:ss")) << QString("\"%1\",").arg(data.temp,0,'f',3)
                               << QString("\"%1\",").arg(data.pressure,0,'f',3) << QString("\"%1\",").arg(data.ref_pressure)
                               << QString("\"%1\"").arg(data.div_pressure,0,'f',3)<< qSetFieldWidth(0) << endl;
                    }

                    stream << QString("\"=======================降温标定采样数据==============\"") << endl;
                    stream.setFieldWidth(30);
                    stream<<QString("\"时间\",") << QString("\"温度(℃)\",") << QString("\"实际压强(PA)\",") << QString("\"负载压强(kPA)\",")
                         << QString("\"压强差(PA)\"") << qSetFieldWidth(0) <<endl;

                    for(int j =0;j < param.getDowndata().value(poly_factor_map.keys()[i]).count();++j)
                    {
                        stream.setFieldWidth(32);
                        node_Data data = param.getDowndata().value(poly_factor_map.keys()[i]).at(j);
                        stream << QString("\"%1\",").arg(QDateTime::fromTime_t(data.unix_time).toString("yyyy-MM-dd hh:mm:ss")) << QString("\"%1\",").arg(data.temp,0,'f',3)
                               << QString("\"%1\",").arg(data.pressure,0,'f',3) << QString("\"%1\",").arg(data.ref_pressure)
                               << QString("\"%1\"").arg(data.div_pressure,0,'f',3)<< qSetFieldWidth(0) << endl;
                    }
                }
            }
            file.close();
        }
        ++nodeIter;
    }
    ui->progressBarPolyfit->setVisible(false);
}

//将拟合系数写到节点中
void MainWindow::on_pbt_downloadToNode_clicked()
{
    ui->progressBarPolyfit->setVisible(true);
    ui->pbt_downloadToNode->setEnabled(false);

    int total = 0;
    QMap<QSerialPort *,nodeParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        total += iter.value().count()*6*5;
        ++iter;
    }

    ui->progressBarPolyfit->setMaximum(total);
    ui->progressBarPolyfit->setValue(0);

    iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        for(int node_index = 0; node_index < iter.value().count(); ++node_index)
        {
            NodeParam &param = iter.value()[node_index];

            QMap<nodePayload,std::vector<double>> poly_factor_map = param.getPolyFactorMap();
            for(int i = 0;i < poly_factor_map.count();++i)
            {
                for(int index = 0; index < poly_factor_map.values()[i].size();++index)
                {
                    ui->progressBarPolyfit->setValue(ui->progressBarPolyfit->value()+1);

                    _displayOutInfo(QString("正在下载串口%1的节点%2%3%4次项系数...").arg(param.getNodePort().portName())
                                    .arg(param.getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));

                    sendSetPayloadFactor(param.getNodeAddr(),iter.key(),poly_factor_map.keys()[i],index,poly_factor_map.values()[i].at(index)*1000);

                    iter.key()->waitForReadyRead(100);
                    if(iter.key()->bytesAvailable())
                    {
                        bool time_out = false;
                        QTimer t;
                        connect(&t,&QTimer::timeout,this,[&](){
                           time_out = true;
                        });
                        t.start(2000);

                        QByteArray array;
                        array.clear();
                        while(!time_out && array.count() < 8)       //读取是否回复正确，错误回复小于8字节
                        {
                            iter.key()->waitForReadyRead(100);
                            array.append(iter.key()->readAll());//将缓冲区所有的数据都读取出来
                            QCoreApplication::processEvents();
                        }
    //                    qDebug() << "reponse:" << array;
                        t.stop();
                        if(time_out)
                        {
                            _displayOutInfo(QString("等待串口%1的节点%2%3%4次项系数回复超时，请重新尝试!").arg(param.getNodePort().portName())
                                            .arg(param.getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));
                            ui->progressBarPolyfit->setVisible(false);
                            ui->pbt_downloadToNode->setEnabled(true);
                            return;
                        }
                    }else
                    {
                        _displayOutInfo(QString("串口%1的节点%2%3%4拟合系数下载失败，请重新尝试!").arg(param.getNodePort().portName())
                                        .arg(param.getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));
                        ui->progressBarPolyfit->setVisible(false);
                        ui->pbt_downloadToNode->setEnabled(true);
                        return;
                    }
                    QElapsedTimer t;    //相邻数据请求，最小时间5ms
                    t.start();
                    while(t.elapsed()<150)
                        QCoreApplication::processEvents();
                }
            }
        }
        ++iter;
    }

    ui->statusBar->showMessage("下载成功",5000);
    ui->progressBarPolyfit->setVisible(false);
    ui->pbt_downloadToNode->setEnabled(true);
}

/**
 *  标定曲线展示
 * @brief MainWindow::on_pbt_calibrationDisplay_clicked
 */
void MainWindow::on_pbt_calibrationDisplay_clicked()
{
    CalibrationDisplay calibration_display(&m_MapPortAndNodeParam);
    calibration_display.move( (QApplication::desktop()->width()-calibration_display.width())/2,
                              (QApplication::desktop()->height()-calibration_display.height())/2 );
    calibration_display.exec();
}
