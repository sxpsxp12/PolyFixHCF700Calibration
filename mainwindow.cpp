#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QElapsedTimer>
#include <QDebug>
#include <QDateTime>
#include <windows.h>
#include <QMessageBox>
#include <QFile>
#include <QString>
#include <QStringList>
#include "matlab/fitting.h"
#include <QStandardPaths>
#include "calibrationDisplay/calibrationdisplay.h"
#include <QDesktopWidget>
#include "components/cdatetimebutton.h"
#include <QStandardItem>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),m_sound(":/sound/sound.wav")
{
    ui->setupUi(this);

    resize(1280,800);
    setWindowTitle("静力水准计标定工具-1.0.0");

    _initWindow();
    _initLoadMovie();
    m_db_operate = new DBOperate;   //开启线程
    m_db_operate->setStartThread(true);
    m_db_operate->start();

    m_originalDataDisplay = new OriginalDataDisplay(m_db_operate);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::_initWindow()
{
    m_calendarWidget = new CCalendarWidget;
    connect(ui->pbt_UpStartTime,SIGNAL(clicked()),this,SLOT(slot_dateTimeButtonClicked()));
    connect(ui->pbt_UpEndTime,SIGNAL(clicked()),this,SLOT(slot_dateTimeButtonClicked()));
    connect(ui->pbt_DownStartTime,SIGNAL(clicked()),this,SLOT(slot_dateTimeButtonClicked()));
    connect(ui->pbt_DownEndTime,SIGNAL(clicked()),this,SLOT(slot_dateTimeButtonClicked()));

    m_MapPortAndNodeParam.clear();
    m_MapPortAndSelectedNodeParam.clear();
    m_currentPayload = FULL_PAYLOAD;
    m_lastPayload = m_currentPayload;
    m_currentCalibrationStatus = CALIBRATION_SAMPLE;    //标定采样状态
    m_lastNodeTemp = 100;

    ui->progressBar->hide();
    ui->spinBox->setFocusPolicy(Qt::NoFocus);
    ui->pbt_clearFactor->setVisible(false);

    //设备展示
    m_tableview_model_nodelist = new TableViewModel(TABLE_NODELIST);
    m_headerview_nodelist = new HeaderView(Qt::Horizontal);
    m_checkbox_delegate = new ItemDelegate(TABLE_NODELIST);

    m_tableview_model_nodelist->setColumnCount(6);
    m_tableview_model_nodelist->setHeaderLabels(QStringList()<<"选择"<<"串口号"<< "设备地址" << "设备SN号" << "设备量程(KPA)" << "设备版本");

    ui->tableViewNodeList->setModel(m_tableview_model_nodelist);
    ui->tableViewNodeList->setHorizontalHeader(m_headerview_nodelist);
    ui->tableViewNodeList->verticalHeader()->setVisible(false);
    ui->tableViewNodeList->horizontalHeader()->setStretchLastSection(true);
    ui->tableViewNodeList->setItemDelegate(m_checkbox_delegate);
    for(int i = 0;i < m_tableview_model_nodelist->columnCount();++i)
        ui->tableViewNodeList->setColumnWidth(i,120);

    connect(m_headerview_nodelist,SIGNAL(stateChanged(Qt::CheckState)),m_tableview_model_nodelist,SLOT(slot_stateChanged(Qt::CheckState)));
    connect(m_tableview_model_nodelist,SIGNAL(stateChanged(Qt::CheckState)),m_headerview_nodelist,SLOT(slot_stateChanged(Qt::CheckState)));
    connect(m_tableview_model_nodelist,SIGNAL(signal_changeNodeSelectedStatus(QSerialPortInfo,quint16,bool)),this,SLOT(slot_changeNodeSelectedStatus(QSerialPortInfo,quint16,bool)));

    //采样数据展示
    ui->treeWidgetSampleData->setColumnCount(8);
    for(int i = 0; i < 8;++i)
    {
        ui->treeWidgetSampleData->headerItem()->setTextAlignment(i,Qt::AlignHCenter);
    }
    ui->treeWidgetSampleData->setColumnWidth(0,150);
    ui->treeWidgetSampleData->setHeaderLabels(QStringList() << "时间" << "串口号" << "地址" << "温度(℃)" <<"实际压强(PA)" << "负载压强(KPA)"<<"压强偏差(PA)"<<"当前负载数据量");

    //拟合结果展示
    m_polyfit_header = new HeaderView(Qt::Horizontal,true);
    m_polyfit_treemodel = new StandardItemModel;

    ui->treeView_Polyfit->setHeader(m_polyfit_header);
    ui->treeView_Polyfit->setModel(m_polyfit_treemodel);
    ui->treeView_Polyfit->setItemDelegate(new ItemDelegate(TABLE_NONE));
    ui->treeView_Polyfit->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->treeView_Polyfit->setIndentation(30);
    ui->treeView_Polyfit->header()->setStretchLastSection(true);
    connect(m_polyfit_header,SIGNAL(stateChanged(Qt::CheckState)),m_polyfit_treemodel,SLOT(slot_stateChanged(Qt::CheckState)));
    connect(m_polyfit_treemodel,SIGNAL(stateChanged(Qt::CheckState)),m_polyfit_header,SLOT(slot_stateChanged(Qt::CheckState)));

    connect(&m_checkPortDrop,&QTimer::timeout,this,&MainWindow::slot_nodeDropCheck);//定时校验节点掉线
    connect(&m_opTimer,&QTimer::timeout,this,&MainWindow::slot_op_timer_out);
    connect(&m_requestNodeTemp,&QTimer::timeout,this,&MainWindow::slot_requestNodeTempTimerOut);

    ui->stackedWidget->setCurrentWidget(ui->pageNodeList);
}

void MainWindow::slot_dateTimeButtonClicked()
{
    CDateTimeButton *d = qobject_cast<CDateTimeButton*>(QObject::sender());
    m_calendarWidget->setDateTime(QDateTime::fromString(d->text(),"yyyy-MM-dd HH:mm:ss"));
    m_calendarWidget->setMaximumDate(d->maximumDateTime().date());
    m_calendarWidget->setMinimumDate(d->minimumDateTime().date());

    disconnect(m_calendarWidget,SIGNAL(dateTimeSelect(QDateTime)),ui->pbt_UpStartTime,SLOT(setDateTime(QDateTime)));
    disconnect(m_calendarWidget,SIGNAL(dateTimeSelect(QDateTime)),ui->pbt_UpEndTime,SLOT(setDateTime(QDateTime)));
    disconnect(m_calendarWidget,SIGNAL(dateTimeSelect(QDateTime)),ui->pbt_DownStartTime,SLOT(setDateTime(QDateTime)));
    disconnect(m_calendarWidget,SIGNAL(dateTimeSelect(QDateTime)),ui->pbt_DownEndTime,SLOT(setDateTime(QDateTime)));
    connect(m_calendarWidget,SIGNAL(dateTimeSelect(QDateTime)),d,SLOT(setDateTime(QDateTime)));

    QPoint p1 = d->mapToGlobal(d->rect().topLeft());
    QRect screen = QApplication::desktop()->rect();
    if(p1.x()+m_calendarWidget->width()>screen.right())
        p1.setX(screen.right()-m_calendarWidget->width());
    p1.setY(p1.y()-m_calendarWidget->height());
    m_calendarWidget->move(p1);
    if(m_calendarWidget->isHidden())
        m_calendarWidget->show();
}

void MainWindow::_initLoadMovie()
{
    m_loadDialog = new QDialog(this);
    m_loadDialog->hide();

    m_loadDialog->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint|Qt::Dialog);
    m_loadDialog->setModal(true);
    m_loadDialog->setStyleSheet("QDialog{background:lightgray;}");
    QLabel *labelLoading = new QLabel;
    QMovie * movie = new QMovie(":/image/loading_line.gif");
    labelLoading->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    labelLoading->setMaximumSize(280,30);
    labelLoading->setScaledContents(true);
    labelLoading->setMovie(movie);
    movie->start();
    QLabel *titleLabel = new QLabel("请稍候...");
    QVBoxLayout *vl = new QVBoxLayout;
    m_loadDialog->setLayout(vl);
    m_loadDialog->layout()->addWidget(titleLabel);
    m_loadDialog->layout()->addWidget(labelLoading);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    int ret = QMessageBox::information(this,"提示","确定要退出本软件吗?",QMessageBox::Ok,QMessageBox::No);
    if(ret == QMessageBox::No)
    {
        e->ignore();
        return;
    }

    m_db_operate->commitTransaction(true);  //提交事务

    //退出线程
    m_db_operate->setStartThread(false);
    while(m_db_operate->isRunning())
        QCoreApplication::processEvents();

    //强制导出标定曲线
    on_pbt_exportCalibrationData_clicked();

    while(m_MapPortAndNodeParam.count())
    {
        QSerialPort *port = m_MapPortAndNodeParam.firstKey();
        port->close();
        m_MapPortAndNodeParam[port].clear();
        m_MapPortAndSelectedNodeParam[port].clear();
        m_MapPortAndNodeParam.remove(port);
        m_MapPortAndSelectedNodeParam.remove(port);
        delete port;
        port = NULL;
    }
    m_MapPortAndNodeParam.clear();
    m_MapPortAndSelectedNodeParam.clear();
    e->accept();
}

void MainWindow::on_groupBoxTip_toggled(bool arg1)
{
    if(arg1)
    {
        _displayOutInfo(QString("启用温度阈值提醒!!!"));
        m_requestNodeTemp.start(3000);
    }else
    {
        m_lastNodeTemp = 100;
        _displayOutInfo(QString("禁用温度阈值提醒!!!"));
        m_requestNodeTemp.stop();
    }
}

void MainWindow::slot_requestNodeTempTimerOut()
{
    if(m_MapPortAndSelectedNodeParam.isEmpty() || m_MapPortAndSelectedNodeParam.first().isEmpty())
        return;

    //直接查询节点的温度
    sendRequestNodeTemp(m_MapPortAndSelectedNodeParam.first().first()->getNodeAddr(),m_MapPortAndSelectedNodeParam.firstKey());
    double node_temp = 0;

    m_MapPortAndSelectedNodeParam.firstKey()->waitForReadyRead(100);
    if(m_MapPortAndSelectedNodeParam.firstKey()->bytesAvailable())
    {
        QByteArray array;
        bool time_out = false;
        QTimer t;
        connect(&t,&QTimer::timeout,this,[&](){
           time_out = true;
        });
        t.start(2000);

        while(!time_out && array.count() < 9)
        {
            m_MapPortAndSelectedNodeParam.firstKey()->waitForReadyRead(100);
            array.append(m_MapPortAndSelectedNodeParam.firstKey()->readAll());
            QCoreApplication::processEvents();
        }
        t.stop();
        if(!time_out && !array.isEmpty())
        {
            char * data = array.data();
            node_temp = ntohf(*((float*)(data+3)));
        }else
        {
            return;
        }
    }else
    {
        return;
    }

    _displayOutInfo(QString("当前温度:%1℃").arg(node_temp));
    ui->label_currentTemp->setText(QString("%1℃").arg(node_temp,0,'f',3));

    if(m_lastNodeTemp != 100)
    {
        //声音提示
        if(abs(node_temp - m_lastNodeTemp) >= ui->spinBox->value())
        {
            m_sound.play();
            m_lastNodeTemp = node_temp;
        }
    }else
    {
        m_lastNodeTemp = node_temp;
    }
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
        nodeShareedPtrParamList param_list = m_MapPortAndNodeParam.values()[i];
        if(!param_list.isEmpty() && !param_list[0]->getNodePort().isValid())
        {
            for(int j = 0; j < param_list.count();++j)
            {
                m_tableview_model_nodelist->removeNodeOfTable(param_list[j]->getNodePort(),param_list[j]->getNodeAddr());
                _displayOutInfo(QString("串口%1的节点:%2掉线!").arg(param_list[j]->getNodePort().portName()).arg(param_list[j]->getNodeAddr()));
            }
            m_MapPortAndSelectedNodeParam[m_MapPortAndNodeParam.keys()[i]].clear();
            m_MapPortAndNodeParam[m_MapPortAndNodeParam.keys()[i]].clear();
            m_MapPortAndSelectedNodeParam.remove(m_MapPortAndNodeParam.keys()[i]);
            m_MapPortAndNodeParam.remove(m_MapPortAndNodeParam.keys()[i]);
            continue;
        }
        ++i;
        QCoreApplication::processEvents();
    }
}

void MainWindow::on_pbt_connectScan_clicked()
{
    //防止误触
    if(m_MapPortAndNodeParam.count())
    {
        int result = QMessageBox::information(this,"提示","确认清空当前节点列表，重新加载吗?",QMessageBox::Ok,QMessageBox::No);
        if(result == QMessageBox::No)
            return;
        else
        {
            while(m_MapPortAndNodeParam.count())
            {
                QSerialPort *port = m_MapPortAndNodeParam.firstKey();
                port->close();
                m_MapPortAndNodeParam[port].clear();
                m_MapPortAndNodeParam.remove(port);
                delete port;
                port = NULL;
            }
        }
    }
    ui->statusBar->showMessage(QString("连接新串口并完成节点的扫描可能需要一段时间，请耐心等待..."),0);

    m_MapPortAndNodeParam.clear();
    m_MapPortAndSelectedNodeParam.clear();
    m_tableview_model_nodelist->clear();
    ui->textBrowser->clear();

    //首先连接所有的串口
    int availablePort = QSerialPortInfo::availablePorts().count();
    ui->progressBar->setMaximum(availablePort*254);
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    ui->centralWidget->setEnabled(false);
    m_checkPortDrop.stop();

    bool is_have_new = false;
    for(int i = 0;i < availablePort;++i)
    {
        QSerialPortInfo info = QSerialPortInfo::availablePorts().at(i);
        if(info.isBusy())       //避开已经连接的节点
        {
            _displayOutInfo(QString("串口:%1,isBusy:%2").arg(info.portName()).arg(info.isBusy()==true?"true":"false"));
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
        nodeShareedPtrParamList param_list;
        param_list.clear();
        if(_scanNodeInfo(serial,param_list,info))
        {
            is_have_new = true;
            m_MapPortAndNodeParam.insert(serial,param_list);
        }else
        {
            serial->close();
            delete serial;
            serial = NULL;
        }
    }

    ui->statusBar->showMessage(QString("设备连接并扫描完毕!!!"),5000);
    //扫描完所有串口，展示节点
    QList<TableDataOfNodeList> nodelist;
    TableDataOfNodeList nodeinfo;
    nodelist.clear();
    bool is_support = true;
    foreach(const nodeShareedPtrParamList &list,m_MapPortAndNodeParam.values())  //列举要展示的节点列表，同时校验设备软件版本
    {
        for(int i = 0;i < list.count();++i)
        {
            nodeinfo.is_be_checked = false;
            nodeinfo.portinfo = list.at(i)->getNodePort();
            nodeinfo.node_addr = list.at(i)->getNodeAddr();
            nodeinfo.node_range = list.at(i)->getNodeRange();
            nodeinfo.serial_number = list.at(i)->getSerialNumber();
            nodeinfo.softversion = list.at(i)->getNodeSoftVersion();
            nodelist.append(nodeinfo);

            if(nodeinfo.softversion < NODESOFTVERSION)
                is_support = false;
        }
    }

    if(nodelist.count())
    {
        m_tableview_model_nodelist->updateNodeListData(nodelist);

        //建立数据库连接
        m_db_operate->init_DBs(nodelist);
    }

    if(is_support && is_have_new && nodelist.count())
    {
        m_checkPortDrop.start(3000);
        on_pbt_clearFactor_clicked();
    }

    ui->centralWidget->setEnabled(true);
    ui->progressBar->setVisible(false);
    ui->pbt_startCalibrationSample->setEnabled(true);
    ui->pbt_calibration->setEnabled(true);

    if(!is_support)
    {
        ui->pbt_startCalibrationSample->setEnabled(false);
        ui->pbt_calibration->setEnabled(false);
        QMessageBox::information(this,"提示","存在不支持温漂标定的HCF700设备,请移除后,重新加载设备!",QMessageBox::Ok);
    }
}

//清空多项式寄存器
void MainWindow::on_pbt_clearFactor_clicked()
{
    ui->pbt_clearFactor->setVisible(false);
    ui->pbt_connectScan->setEnabled(false);

    ui->statusBar->showMessage(QString("开始初始化多项式寄存器操作!!!"),5000);
    int total = 0;
    QMap<QSerialPort*,nodeShareedPtrParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        total += iter.value().count();
        ++iter;
    }
    ui->centralWidget->setEnabled(false);
    ui->progressBar->setVisible(true);
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(0);

    iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        nodeShareedPtrParamList &param_list = iter.value();
        for(int i = 0;i < param_list.count();++i)
        {
            ui->progressBar->setValue(ui->progressBar->value()+1);

            sendClearFactor(param_list[i]->getNodeAddr(),iter.key());
            iter.key()->waitForReadyRead(200);
            if(iter.key()->bytesAvailable())
            {
                QByteArray array;
                bool time_out = false;
                QTimer t;
                connect(&t,&QTimer::timeout,this,[&](){
                   time_out = true;
                });
                t.start(2000);

                while(!time_out && array.count() < 8)
                {
                    iter.key()->waitForReadyRead(200);
                    array.append(iter.key()->readAll());
                    QCoreApplication::processEvents();
                }
                t.stop();
                if(!time_out && !array.isEmpty())
                {
                    char *data = array.data();
                    quint8 instructCode = *((quint8*)data+1);
                    if(instructCode & 0x80)
                    {
                        _displayOutInfo(QString("初始化串口%1的节点%2多项式系数寄存器失败!!!").arg(param_list[i]->getNodePort().portName())
                                        .arg(param_list[i]->getNodeAddr()));
                        ui->pbt_clearFactor->setVisible(true);
                    }else
                    {
                        _displayOutInfo(QString("初始化串口%1的节点%2多项式系数寄存器成功!!!").arg(param_list[i]->getNodePort().portName())
                                        .arg(param_list[i]->getNodeAddr()));
                    }
                }else
                {
                    _displayOutInfo(QString("初始化串口%1的节点%2多项式系数寄存器失败!!!").arg(param_list[i]->getNodePort().portName())
                                    .arg(param_list[i]->getNodeAddr()));
                    ui->pbt_clearFactor->setVisible(true);
                }
            }else
            {
                _displayOutInfo(QString("初始化串口%1的节点%2多项式系数寄存器失败!!!").arg(param_list[i]->getNodePort().portName())
                                .arg(param_list[i]->getNodeAddr()));
                ui->pbt_clearFactor->setVisible(true);
            }
        }
        ++iter;
    }
    ui->centralWidget->setEnabled(true);
    ui->progressBar->setVisible(false);
    ui->pbt_connectScan->setEnabled(true);

    ui->statusBar->showMessage(QString("初始化操作结束!!!"),5000);
}

void MainWindow::slot_changeNodeSelectedStatus(QSerialPortInfo port_info, quint16 addr, bool status)
{
    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator iter = m_MapPortAndNodeParam.begin();
    while(iter != m_MapPortAndNodeParam.end())
    {
        nodeShareedPtrParamList &param_list = iter.value();
        for(int i = 0; i < param_list.count();++i)
        {
            QSharedPointer<NodeParam> &node_param = param_list[i];
            if(node_param->getNodePort().portName() == port_info.portName()
                    && node_param->getNodeAddr() == addr)
            {
                node_param->setNodeSelectedStatus(status);
                if(status)  //选中状态
                {
                    if(!m_MapPortAndSelectedNodeParam[iter.key()].contains(node_param))
                    {
                        m_MapPortAndSelectedNodeParam[iter.key()].append(node_param);
                    }
                }else   //未选中状态
                {
                    if(m_MapPortAndSelectedNodeParam[iter.key()].contains(node_param))
                    {
                        m_MapPortAndSelectedNodeParam[iter.key()].removeOne(node_param);
                        if(m_MapPortAndSelectedNodeParam[iter.key()].isEmpty())
                            m_MapPortAndSelectedNodeParam.remove(iter.key());
                    }
                }
                return;
            }
        }
        ++iter;
    }
}

//扫描节点信息，可以获取到地址和量程
bool MainWindow::_scanNodeInfo(QSerialPort *serial,nodeShareedPtrParamList &param_list,QSerialPortInfo info)
{
    bool is_ok = false;
    quint16 node_range=0;
    quint16 node_addr=0;
    double node_calibration_temp = 0;
    //1-255地址依次扫描新节点，以确定该节点的地址
    for(int addr = 1;addr < 255;++addr)
    {
        NodeParam *param = new NodeParam(info);
        bool operate = false;
        //直接查询该地址的量程和校准温度
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

                param->setNodeAddr(node_addr);
                param->setNodeRange(node_range);
                param->setNodeCalibrationTemp(node_calibration_temp);

                operate = true;
            }
        }

        if(!operate)
        {
            ui->progressBar->setValue(ui->progressBar->value()+1);
            QElapsedTimer t;    //相邻数据请求，最小时间5ms
            t.start();
            while(t.elapsed()<150)
                QCoreApplication::processEvents();

            delete param;
            param = NULL;
            continue;
        }
        {
            QElapsedTimer t;    //相邻数据请求，最小时间5ms
            t.start();
            while(t.elapsed()<50)
                QCoreApplication::processEvents();
        }

        operate = false;
        //查询该设备的版本
        sendRequestNodeSoftVertion(addr,serial);
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

            while(!time_out && array.count() < 7)
            {
                serial->waitForReadyRead(50);
                array.append(serial->readAll());
                QCoreApplication::processEvents();
            }
            t.stop();
            if(!time_out && !array.isEmpty())
            {
                char * data = array.data();
                quint16 version = ntohs(*((quint16*)(data+3)));

                quint16 soft_version = (version>>8)*100+(version&0xff);
                param->setNodeSoftVersion(soft_version);
                operate = true;
            }
        }

        if(!operate)
        {
            ui->progressBar->setValue(ui->progressBar->value()+1);
            QElapsedTimer t;    //相邻数据请求，最小时间5ms
            t.start();
            while(t.elapsed()<50)
                QCoreApplication::processEvents();

            delete param;
            param = NULL;
            continue;
        }
        {
            QElapsedTimer t;    //相邻数据请求，最小时间5ms
            t.start();
            while(t.elapsed()<50)
                QCoreApplication::processEvents();
        }

        //获取SN号
        operate = false;
        sendRequestSN(addr,serial);
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

            while(!time_out && array.count() < 20)
            {
                serial->waitForReadyRead(50);
                array.append(serial->readAll());
                QCoreApplication::processEvents();
            }
            t.stop();
            if(!time_out && !array.isEmpty())
            {
                char * data = array.data();
                int len = *(data+2);
                QString sn;
                for(int i = 0;i < len;++i)
                {
                    QChar one_char(*(data+3+i));
                    sn.append(one_char);
                }
                param->setSerialNumber(sn);
                operate = true;
            }
        }

        if(!operate)
        {
            ui->progressBar->setValue(ui->progressBar->value()+1);
            QElapsedTimer t;    //相邻数据请求，最小时间5ms
            t.start();
            while(t.elapsed()<50)
                QCoreApplication::processEvents();

            delete param;
            param = NULL;
            continue;
        }

        param_list.append(QSharedPointer<NodeParam>(param));
        is_ok = true;

        ui->progressBar->setValue(ui->progressBar->value()+1);
        {
            QElapsedTimer t;    //相邻数据请求，最小时间5ms
            t.start();
            while(t.elapsed()<50)
                QCoreApplication::processEvents();
        }
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
            m_MapPortAndSelectedNodeParam[m_MapPortAndSelectedNodeParam.keys()[m_current_port]][m_current_node]->setNodeStatus(Status_None);
            m_MapPortAndSelectedNodeParam[m_MapPortAndSelectedNodeParam.keys()[m_current_port]][m_current_node]->setNodeReceiveData(QByteArray());

            //请求下一节点的数据
            m_current_node++;
            if(m_current_node >= m_MapPortAndSelectedNodeParam.values()[m_current_port].count())
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
    //判断是否存在未选中节点
    bool is_have_selected = false;
    foreach (const nodeShareedPtrParamList &list, m_MapPortAndSelectedNodeParam.values()) {
        if(!list.isEmpty())
        {
            is_have_selected = true;
            break;
        }
    }
    if(!is_have_selected)
    {
        ui->statusBar->showMessage(QString("当前没有可操作设备,请先获取要参与采样的设备!"),5000);
        return;
    }

    m_currentCalibrationStatus = CALIBRATION_SAMPLE;    //标定采样状态
    ui->pbt_connectScan->setEnabled(false);
    ui->pbt_clearFactor->setEnabled(false);

    m_currentPayload = FULL_PAYLOAD;
    m_lastPayload = m_currentPayload;

    ui->labelCurrentStatus->setText(QString("当前:%1--%2KPa").arg(getStrPayLoad(m_currentPayload)).
                                    arg(_getReferencePresure(m_currentPayload,m_MapPortAndSelectedNodeParam.values().first().first()->getNodeRange())));
    ui->stackedWidget->setCurrentWidget(ui->pageCalibrationSampleProcess);

    m_current_port = 0;    //当前请求串口的索引
    m_current_node = 0;     //请求当前节点的索引
    ui->pbt_undoSample->setEnabled(false);

    //阈值提醒
    m_lastNodeTemp = 100;
    ui->groupBoxTip->setChecked(true);
    on_groupBoxTip_toggled(true);
}


/*******************************标定采样过程界面**************************/
void MainWindow::on_pbt_sample_data_clicked()
{
    if(m_sound.isFinished())
    {
        m_sound.stop();
    }

    if(m_MapPortAndSelectedNodeParam.isEmpty())
    {
        _displayOutInfo(QString("选中节点掉线，请重新连接节点再继续采样!"));
        return;
    }

    m_requestNodeTemp.stop();
    ui->pbt_sample_data->setEnabled(false);
    ui->pbt_undoSample->setEnabled(false);
    ui->progressBar->setVisible(true);
    int total = 0;
    QMap<QSerialPort*,nodeShareedPtrParamList>::iterator iter = m_MapPortAndSelectedNodeParam.begin();
    while(iter != m_MapPortAndSelectedNodeParam.end())
    {
        total += iter.value().count();
        ++iter;
    }
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(0);


    //广播发送采样指令，不需要判断回复指令
    iter = m_MapPortAndSelectedNodeParam.begin();
    while(iter != m_MapPortAndSelectedNodeParam.end())
    {
        _displayOutInfo(QString("串口%1发送广播采样指令...").arg(iter.value().at(0)->getNodePort().portName()));
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

//读数据
void MainWindow::slot_serialDataComming()
{
    QSerialPort *serial = qobject_cast<QSerialPort *>(sender());
    QByteArray receiveData = m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodeReceiveData();
    receiveData.append(serial->readAll());
    m_MapPortAndSelectedNodeParam[serial][m_current_node]->setNodeReceiveData(receiveData);

    _displayOutInfo(QString("串口%1的节点%2正在解析数据...").arg(m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodePort().portName())
                    .arg(m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodeAddr()));

    if(m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodeStatus()==Status_QueryInputRegister && receiveData.length()>=37)
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
            _displayOutInfo(QString("串口%1的节点 %2 CRC校验失败，自动忽略该数据包!").arg(m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodePort().portName())
                            .arg(m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodeAddr()));
        }
        else
        {
            node_Data data;
            data.addr = m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodeAddr();
            data.serial_number = m_MapPortAndSelectedNodeParam[serial][m_current_node]->getSerialNumber();
            data.unix_time = QDateTime::currentDateTime().toTime_t();
            data.temp = temperature;
            data.pressure = pressure;
            data.ref_pressure = _getReferencePresure(m_currentPayload,m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodeRange());
            data.div_pressure = pressure-(data.ref_pressure*1000);

            //添加数据库操作
            QString db_table = m_db_operate->getDBTableStrFromPayload(m_currentPayload);
            if(!db_table.isEmpty())
                m_db_operate->addDBTableOperate(data.serial_number,QString("insert into `%1` values(%2,%3,%4,%5,%6);").arg(db_table).arg(data.unix_time)
                                                .arg(data.temp).arg(data.pressure).arg(data.ref_pressure).arg(data.div_pressure));

            m_MapPortAndSelectedNodeParam[serial][m_current_node]->appendnodeData(m_currentPayload,data);

            _displayOutInfo(QString("串口%1的节点%2数据采集成功!").arg(m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodePort().portName())
                            .arg(m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodeAddr()));
        }
        m_MapPortAndSelectedNodeParam[serial][m_current_node]->setNodeStatus(Status_None);
        m_MapPortAndSelectedNodeParam[serial][m_current_node]->setNodeReceiveData(QByteArray());

        disconnect(serial,&QSerialPort::readyRead,this,&MainWindow::slot_serialDataComming);

        //请求下一节点的数据
        m_current_node++;
        if(m_current_node >= m_MapPortAndNodeParam[serial].count())
        {
            ++m_current_port;
            m_current_node=0;
        }
        _requestNodeData();

    }else if(m_MapPortAndSelectedNodeParam[serial][m_current_node]->getNodeStatus()==Status_None)
    {
        m_MapPortAndSelectedNodeParam[serial][m_current_node]->setNodeReceiveData(QByteArray());
    }
}

//请求节点数据
void MainWindow::_requestNodeData()
{
    if(m_MapPortAndSelectedNodeParam.isEmpty() || m_MapPortAndSelectedNodeParam.first().isEmpty())
    {
        ui->statusBar->showMessage(QString("当前没有可操作设备,可能设备已经掉线,请重新获取参与采样的设备!"),0);
        return;
    }

    ui->progressBar->setValue(ui->progressBar->value()+1);
    if(m_current_port < m_MapPortAndSelectedNodeParam.count() && m_current_node < m_MapPortAndSelectedNodeParam.values()[m_current_port].count())
    {
        m_MapPortAndSelectedNodeParam[m_MapPortAndSelectedNodeParam.keys()[m_current_port]][m_current_node]->setNodeStatus(Status_QueryInputRegister);
        m_MapPortAndSelectedNodeParam[m_MapPortAndSelectedNodeParam.keys()[m_current_port]][m_current_node]->setNodeReceiveData(QByteArray());

        connect(m_MapPortAndSelectedNodeParam.keys()[m_current_port],&QSerialPort::readyRead,this,&MainWindow::slot_serialDataComming);

        _displayOutInfo(QString("等待串口%1的节点%2数据的返回...").arg(m_MapPortAndSelectedNodeParam[m_MapPortAndSelectedNodeParam.keys()[m_current_port]][m_current_node]->getNodePort().portName())
                        .arg(m_MapPortAndSelectedNodeParam[m_MapPortAndSelectedNodeParam.keys()[m_current_port]][m_current_node]->getNodeAddr()));

        QElapsedTimer t;    //相邻数据请求，最小时间5ms
        t.start();
        while(t.elapsed()<50)
            QCoreApplication::processEvents();

        sendRequestNodeData(m_MapPortAndSelectedNodeParam.values()[m_current_port][m_current_node]->getNodeAddr(),m_MapPortAndSelectedNodeParam.keys()[m_current_port]);
        m_opTimer.start(4000);  //指令超时定时器
    }else   //请求了所有节点的数据，可以进行下一负载的数据请求
    {
        ui->progressBar->setVisible(false);
        ui->pbt_sample_data->setEnabled(true);
        ui->pbt_undoSample->setEnabled(true);
        m_lastPayload = m_currentPayload;       //记录上次负载
        m_requestNodeTemp.start(3000);

        //展示本次负载的数据
        _displayCurrentPayloadSampleData();

        //提交事务
        m_db_operate->commitTransaction(true);      //每做完一组，提交一次

        if(m_currentPayload == FULL_PAYLOAD)
        {
            m_currentPayload = FOUR_FIFTH_PAYLOAD;
        }else if(m_currentPayload == FOUR_FIFTH_PAYLOAD)
        {
            m_currentPayload = THREE_FIFTH_PAYLOAD;
        }else if(m_currentPayload == THREE_FIFTH_PAYLOAD)
        {
            m_currentPayload = TWO_FIFTH_PAYLOAD;
        }else if(m_currentPayload == TWO_FIFTH_PAYLOAD)
        {
            m_currentPayload = ONE_FIFTH_PAYLOAD;
        }else if(m_currentPayload == ONE_FIFTH_PAYLOAD)
        {
            m_currentPayload = EMPTY_PAYLOAD;
        }else if(m_currentPayload == EMPTY_PAYLOAD)
        {
            m_currentPayload = FULL_PAYLOAD;
        }

        m_current_port = 0;    //当前请求的串口
        m_current_node = 0;     //当前请求的节点
        //负载压强提示
        ui->labelCurrentStatus->setText(QString("当前:%1--%2KPa").arg(getStrPayLoad(m_currentPayload)).
                                        arg(_getReferencePresure(m_currentPayload,m_MapPortAndSelectedNodeParam.values().first().first()->getNodeRange())));
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
        m_db_operate->commitTransaction(true);//结束采样提价一次事务

        ui->stackedWidget->setCurrentWidget(ui->pageNodeList);
        ui->pbt_connectScan->setEnabled(true);
        ui->pbt_clearFactor->setEnabled(true);
        m_requestNodeTemp.stop();
    }else if(ret == QMessageBox::Cancel){}
}

/**
 *  撤销最近一次的采样
 * @brief MainWindow::on_pbt_undoSample_clicked
 */
void MainWindow::on_pbt_undoSample_clicked()
{
    ui->pbt_undoSample->setEnabled(false);
    m_requestNodeTemp.stop();
    //初始化节点升温数据接收容器
    QMap<QSerialPort*,nodeShareedPtrParamList>::iterator iter = m_MapPortAndSelectedNodeParam.begin();
    while(iter != m_MapPortAndSelectedNodeParam.end())
    {
        nodeShareedPtrParamList &param_list = iter.value();
        for(int i = 0;i < param_list.count();++i)
        {
            //添加指令
            QString table_str = m_db_operate->getDBTableStrFromPayload(m_lastPayload);
            if(!table_str.isEmpty())
                m_db_operate->addDBTableOperate(param_list[i]->getSerialNumber(),QString("delete from `%1` where unix_time=%2").arg(table_str)
                                                .arg(param_list[i]->getNodedatas()[m_lastPayload].last().unix_time));

            param_list[i]->removeNodePayloadLastData(m_lastPayload);
        }
        ++iter;
    }
    ui->treeWidgetSampleData->clear();

    m_db_operate->commitTransaction(true);  //提交更改

    m_currentPayload = m_lastPayload;

    m_current_port = 0;    //当前请求的串口
    m_current_node = 0;     //当前请求的节点
    //负载压强提示
    ui->labelCurrentStatus->setText(QString("当前:%1--%2KPa").arg(getStrPayLoad(m_currentPayload)).
                                    arg(_getReferencePresure(m_currentPayload,m_MapPortAndSelectedNodeParam.values().first().first()->getNodeRange())));
    m_requestNodeTemp.start(3000);
}

/**
 *  展示本次负载的数据
 * @brief MainWindow::_displayCurrentPayloadSampleData
 * @param data
 */
void MainWindow::_displayCurrentPayloadSampleData()
{
    ui->treeWidgetSampleData->clear();
    foreach (const nodeShareedPtrParamList &param_list, m_MapPortAndSelectedNodeParam.values()) {

        for(int i = 0;i < param_list.count();++i)
        {
            QSharedPointer<NodeParam> param = param_list[i];

            node_Data data;
            int total = 0;

            if(param->getNodedatas()[m_currentPayload].isEmpty())
                continue;
            data = param->getNodedatas()[m_currentPayload].last();
            total = param->getNodedatas()[m_currentPayload].count();

            QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidgetSampleData);
            item->setText(0,QDateTime::fromTime_t(data.unix_time).toString("yyyy-MM-dd hh:mm:ss"));
            item->setText(1,param->getNodePort().portName());
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

/*************************************多项式拟合界面************************************/
/**
 *  展示多项式拟合结果
 * @brief MainWindow::_displayPolyfitResult
 */
void MainWindow::_displayPolyfitResult()
{
    m_polyfit_treemodel->removeRows(0,m_polyfit_treemodel->invisibleRootItem()->rowCount());
    StandardItem *mainItem = NULL;
    StandardItem *childItem = NULL;
    QList<QStandardItem *> item_list;

    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator iter = m_MapPortAndSelectedNodeParam.begin();
    while(iter != m_MapPortAndSelectedNodeParam.end())
    {
        for(int node_index = 0;node_index < iter.value().count();++node_index)
        {
            QSharedPointer<NodeParam> param = iter.value()[node_index];
            QMap<nodePayload,std::vector<double>> poly_factor_map = param->getPolyFactorMap();
            if(!poly_factor_map.isEmpty())
            {
                item_list.clear();
                mainItem = new StandardItem("");
                mainItem->portItem()->setText(param->getNodePort().portName());
                mainItem->addrItem()->setText(QString::number(param->getNodeAddr()));
                item_list.append(mainItem);
                item_list.append(mainItem->portItem());
                item_list.append(mainItem->addrItem());
                item_list.append(mainItem->pressureItem());
                item_list.append(mainItem->factorItem());
                m_polyfit_treemodel->invisibleRootItem()->appendRow(item_list);
            }
            for(int i = 0; i < poly_factor_map.count();++i)
            {
                item_list.clear();
                childItem = new StandardItem("");
                childItem->pressureItem()->setText(QString::number(_getReferencePresure(poly_factor_map.keys()[i],param->getNodeRange()),'f',0));
                childItem->factorItem()->setText(param->getPolyFactorStr(poly_factor_map.keys()[i]));
                item_list.append(childItem);
                item_list.append(childItem->portItem());
                item_list.append(childItem->addrItem());
                item_list.append(childItem->pressureItem());
                item_list.append(childItem->factorItem());
                mainItem->appendRow(item_list);
            }
        }
        ++iter;
    }

    if(!m_polyfit_treemodel->invisibleRootItem()->rowCount())
        return;

    ui->treeView_Polyfit->expandAll();
    ui->pbt_downloadToNode->setEnabled(true);
    ui->pbt_exportCalibrationData->setEnabled(true);
    ui->pbt_calibrationDisplay->setEnabled(true);
}

/**
 *HCF700标定
 * @brief MainWindow::on_pbt_calibration_clicked
 */
void MainWindow::on_pbt_calibration_clicked()
{
    //判断是否存在未选中节点
    bool is_have_selected = false;
    foreach (const nodeShareedPtrParamList &list, m_MapPortAndSelectedNodeParam.values()) {
        if(!list.isEmpty())
        {
            is_have_selected = true;
            break;
        }
    }
    if(!is_have_selected)
    {
        ui->statusBar->showMessage(QString("当前没有可操作设备,请先获取要参与标定的设备!"),5000);
        return;
    }

    ui->pbt_connectScan->setEnabled(false);
    ui->pbt_clearFactor->setEnabled(false);

    m_currentCalibrationStatus = CALIBRATION_POLYFIT;    //标定采样状态
    ui->progressBar->setVisible(false);
    ui->pbt_downloadToNode->setEnabled(false);
    ui->pbt_exportCalibrationData->setEnabled(false);
    ui->pbt_calibrationDisplay->setEnabled(false);
    m_polyfit_treemodel->removeRows(0,m_polyfit_treemodel->invisibleRootItem()->rowCount());
    ui->stackedWidget->setCurrentWidget(ui->pageCalibration);

    m_loadDialog->show();
    uint min_time=0,max_time=0;
    //从数据库中获取节点的数据区间
    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator iter = m_MapPortAndSelectedNodeParam.begin();
    while(iter != m_MapPortAndSelectedNodeParam.end())
    {
        foreach (QSharedPointer<NodeParam> node, m_MapPortAndSelectedNodeParam[iter.key()]) {
            m_db_operate->getNodeDatasTimeRangeFromDB(node);
        }
        ++iter;
    }

    //参与拟合的节点的时间区间
    foreach (const nodeShareedPtrParamList list, m_MapPortAndSelectedNodeParam.values()) {
        foreach (QSharedPointer<NodeParam> node, list) {
            if(min_time == 0 && max_time == 0)
            {
                min_time = node->getNodeDataMinTime();
                max_time = node->getNodeDataMaxTime();
            }else
            {
                if(min_time > node->getNodeDataMinTime())
                    min_time = node->getNodeDataMinTime();
                if(max_time < node->getNodeDataMaxTime())
                    max_time = node->getNodeDataMaxTime();
            }
        }
    }

    //设置拟合配置  时间区间
    ui->pbt_UpStartTime->setMaximumDateTime(QDateTime::fromTime_t(max_time));
    ui->pbt_UpStartTime->setMinimumDateTime(QDateTime::fromTime_t(min_time));
    ui->pbt_UpEndTime->setMaximumDateTime(QDateTime::fromTime_t(max_time));
    ui->pbt_UpEndTime->setMinimumDateTime(QDateTime::fromTime_t(min_time));

    ui->pbt_DownStartTime->setMaximumDateTime(QDateTime::fromTime_t(max_time));
    ui->pbt_DownStartTime->setMinimumDateTime(QDateTime::fromTime_t(min_time));
    ui->pbt_DownEndTime->setMaximumDateTime(QDateTime::fromTime_t(max_time));
    ui->pbt_DownEndTime->setMinimumDateTime(QDateTime::fromTime_t(min_time));

    if(min_time == 0)
    {
        ui->pbt_UpStartTime->setDateTime(QDateTime());

        ui->pbt_DownStartTime->setDateTime(QDateTime());

        ui->pbt_datasDisplay->setEnabled(false);
        ui->pbt_polyfit->setEnabled(false);
    }else
    {
        ui->pbt_UpStartTime->setDateTime(QDateTime::fromTime_t(min_time));

        ui->pbt_DownStartTime->setDateTime(QDateTime::fromTime_t(min_time));

        ui->pbt_datasDisplay->setEnabled(true);
        ui->pbt_polyfit->setEnabled(true);
    }
    if(max_time == 0)
    {
        ui->pbt_UpEndTime->setDateTime(QDateTime());

        ui->pbt_DownEndTime->setDateTime(QDateTime());

        ui->pbt_datasDisplay->setEnabled(false);
        ui->pbt_polyfit->setEnabled(false);
    }else
    {
        ui->pbt_UpEndTime->setDateTime(QDateTime::fromTime_t(max_time));

        ui->pbt_DownEndTime->setDateTime(QDateTime::fromTime_t(max_time));

        ui->pbt_datasDisplay->setEnabled(true);
        ui->pbt_polyfit->setEnabled(true);
    }

    m_loadDialog->hide();
}

void MainWindow::on_pbt_returnPageNodeList_clicked()
{
    ui->pbt_connectScan->setEnabled(true);
    ui->pbt_clearFactor->setEnabled(true);
    ui->stackedWidget->setCurrentWidget(ui->pageNodeList);
}
/**开始拟合***********/
void MainWindow::on_pbt_polyfit_clicked()
{
    //时间区间验证有效性
    uint up_start_time = QDateTime::fromString(ui->pbt_UpStartTime->text(),"yyyy-MM-dd HH:mm:ss").toTime_t();
    uint up_end_time = QDateTime::fromString(ui->pbt_UpEndTime->text(),"yyyy-MM-dd HH:mm:ss").toTime_t();
    uint down_start_time = QDateTime::fromString(ui->pbt_DownStartTime->text(),"yyyy-MM-dd HH:mm:ss").toTime_t();
    uint down_end_time = QDateTime::fromString(ui->pbt_DownEndTime->text(),"yyyy-MM-dd HH:mm:ss").toTime_t();
    if(up_start_time > up_end_time || down_start_time > down_end_time)
    {
        QMessageBox::information(this,"提示","起始时间不能大于结束时间,\n请重新设置!",QMessageBox::Ok);
        return;
    }

    ui->progressBar->setVisible(true);

    ui->statusBar->showMessage("正在获取设备数据,请稍候...",0);
    int total = 0;
    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator iter = m_MapPortAndSelectedNodeParam.begin();
    while(iter != m_MapPortAndSelectedNodeParam.end())
    {
        total += iter.value().count()*6;
        ++iter;
    }
    ui->progressBar->setMaximum(total/6*2);
    ui->progressBar->setValue(0);

    //获取设备数据
    iter = m_MapPortAndSelectedNodeParam.begin();
    while(iter != m_MapPortAndSelectedNodeParam.end())
    {
        foreach (QSharedPointer<NodeParam> node, iter.value()) {
            ui->progressBar->setValue(ui->progressBar->value()+1);
            m_db_operate->getNodeDatasFromStartTimeToEndTime(node,up_start_time,up_end_time,DATA_UP);
            ui->progressBar->setValue(ui->progressBar->value()+1);
            m_db_operate->getNodeDatasFromStartTimeToEndTime(node,down_start_time,down_end_time,DATA_DOWN);
        }
        ++iter;
    }

    //数据拟合
    ui->statusBar->showMessage("正在拟合数据，请稍候...",0);
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(0);

    bool isok = true;
    iter = m_MapPortAndSelectedNodeParam.begin();
    while(iter != m_MapPortAndSelectedNodeParam.end())
    {
        for(int i = 0;i < iter.value().count();++i)
        {
            QSharedPointer<NodeParam> &param = iter.value()[i];
            param->clearPolyFactor();

            ui->progressBar->setValue(ui->progressBar->value()+1);
            _displayOutInfo(QString("正在串口%1的节点:%2满量程负载标定...").arg(param->getNodePort().portName())
                            .arg(param->getNodeAddr()));
            m_currentPayload = FULL_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo(QString("串口%1的节点:%2多项式拟合失败!").arg(param->getNodePort().portName())
                                .arg(param->getNodeAddr()));
                ui->progressBar->setValue(ui->progressBar->value()+5);
                param->clearPolyFactor();
                continue;
            }

            _displayOutInfo(QString("正在串口%1的节点:%2五分之四量程负载标定...").arg(param->getNodePort().portName())
                            .arg(param->getNodeAddr()));
            ui->progressBar->setValue(ui->progressBar->value()+1);
            m_currentPayload = FOUR_FIFTH_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo(QString("串口%1的节点:%2多项式拟合失败!").arg(param->getNodePort().portName())
                                .arg(param->getNodeAddr()));
                ui->progressBar->setValue(ui->progressBar->value()+4);
                param->clearPolyFactor();
                continue;
            }

            _displayOutInfo(QString("正在串口%1的节点:%2五分之三量程负载标定...").arg(param->getNodePort().portName())
                            .arg(param->getNodeAddr()));
            ui->progressBar->setValue(ui->progressBar->value()+1);
            m_currentPayload = THREE_FIFTH_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo(QString("串口%1的节点:%2多项式拟合失败!").arg(param->getNodePort().portName())
                                .arg(param->getNodeAddr()));
                ui->progressBar->setValue(ui->progressBar->value()+3);
                param->clearPolyFactor();
                continue;
            }

            _displayOutInfo(QString("正在串口%1的节点:%2五分之二量程负载标定...").arg(param->getNodePort().portName())
                            .arg(param->getNodeAddr()));
            ui->progressBar->setValue(ui->progressBar->value()+1);
            m_currentPayload = TWO_FIFTH_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo(QString("串口%1的节点:%2多项式拟合失败!").arg(param->getNodePort().portName())
                                .arg(param->getNodeAddr()));
                ui->progressBar->setValue(ui->progressBar->value()+2);
                param->clearPolyFactor();
                continue;
            }

            _displayOutInfo(QString("正在串口%1的节点:%2五分之一量程负载标定...").arg(param->getNodePort().portName())
                            .arg(param->getNodeAddr()));
            ui->progressBar->setValue(ui->progressBar->value()+1);
            m_currentPayload = ONE_FIFTH_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo(QString("串口%1的节点:%2多项式拟合失败!").arg(param->getNodePort().portName())
                                .arg(param->getNodeAddr()));
                ui->progressBar->setValue(ui->progressBar->value()+1);
                param->clearPolyFactor();
                continue;
            }

            _displayOutInfo(QString("正在串口%1的节点:%2零量程负载标定...").arg(param->getNodePort().portName())
                            .arg(param->getNodeAddr()));
            ui->progressBar->setValue(ui->progressBar->value()+1);
            m_currentPayload = EMPTY_PAYLOAD;
            isok = _polyFitData(param);
            if(!isok)
            {
                _displayOutInfo(QString("串口%1的节点:%2多项式拟合失败!").arg(param->getNodePort().portName())
                                .arg(param->getNodeAddr()));
                param->clearPolyFactor();
                continue;
            }
        }
        ++iter;
    }

    ui->progressBar->setVisible(false);
    ui->statusBar->showMessage("多项式系数拟合完成!",3000);

    //展示拟合结果
    _displayPolyfitResult();
}

bool MainWindow::_polyFitData(QSharedPointer<NodeParam> param)
{
    QList<node_Data> upList = param->getUpdata().value(m_currentPayload);
    QList<node_Data> downList = param->getDowndata().value(m_currentPayload);
    if(upList.isEmpty() || downList.isEmpty())
    {
        _displayOutInfo(QString("串口%1的节点%2,升温标定数据或者降温标定数据为空!!!").arg(param->getNodePort().portName()).arg(param->getNodeAddr()));
        return false;
    }
    std::vector<double> factor = _polyFitCurrentPayloadData(upList,downList);
    if(factor.empty())
    {
        _displayOutInfo(QString("串口%1的节点%2多项式拟合失败!").arg(param->getNodePort().portName()).arg(param->getNodeAddr()));
        return false;
    }else
    {
        param->setPolyFactor(m_currentPayload,factor);
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
        pressure.push_back(up_list[i].div_pressure);
    }

    for(int i = down_list.count()-1;i >= 0;--i)
    {
        temps.push_back(down_list[i].temp);
        pressure.push_back(down_list[i].div_pressure);
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
    qDebug() << "拟合方程:"<<strFun
             << " ssr:" << fit.getSSR()<< " sse:" <<fit.getSSE()<<
                " rmse:"<<fit.getRMSE()<< " 确定系数:" <<fit.getR_square();

    //获取多项式系数并返回
    std::vector<double> factor;
    fit.getFactor(factor);
    return factor;
}

/**
 *  标定数据导出
 * @brief MainWindow::on_pbt_downloadCalibrationData_clicked
 */
void MainWindow::on_pbt_exportCalibrationData_clicked()
{
    ui->progressBar->setVisible(true);
    int total = 0;
    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator nodeIter = m_MapPortAndSelectedNodeParam.begin();
    while(nodeIter != m_MapPortAndSelectedNodeParam.end())
    {
        total += nodeIter.value().count()*6;
        ++nodeIter;
    }
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(0);

    QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    nodeIter = m_MapPortAndSelectedNodeParam.begin();
    while(nodeIter != m_MapPortAndSelectedNodeParam.end())
    {
        for(int node_index = 0; node_index < nodeIter.value().count();++node_index)
        {
            QSharedPointer<NodeParam> param = nodeIter.value()[node_index];

            QMap<nodePayload,std::vector<double>> poly_factor_map = param->getPolyFactorMap();
            QString filename = path+QString("/%1-%2.csv")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm"))
                    .arg(param->getSerialNumber());
            QFile file(filename);
            if(file.open(QIODevice::WriteOnly|QIODevice::Text))
            {
                QTextStream stream(&file);
                stream.setFieldAlignment(QTextStream::AlignLeft);
                stream << QString("\"节点地址:%1      \",").arg(param->getNodeAddr()) << QString("\"节点SN号:%1      \",").arg(param->getSerialNumber())
                       << QString("\"节点量程(kPA):%1  \",").arg(param->getNodeRange())
                       << QString("\"节点校准温度(℃):%1\"").arg(param->getNodeCalibrationTemp())<<endl;
                for(int i = 0;i < poly_factor_map.keys().count();++i)
                {
                    ui->progressBar->setValue(ui->progressBar->value()+1);

                    stream << endl;
                    stream << endl;

                    stream << QString("\"节点当前负载(kPA)(%1):%2  \",").arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(_getReferencePresure(poly_factor_map.keys()[i],param->getNodeRange()))
                           << QString("\"多项式拟合公式:%1\"").arg(param->getPolyFactorStr(poly_factor_map.keys()[i])) << endl;

                    stream << QString("\"=======================升温标定采样数据==============\"") << endl;
                    stream.setFieldWidth(30);
                    stream<<QString("\"时间\",") << QString("\"温度(℃)\",") << QString("\"实际压强(PA)\",") << QString("\"负载压强(kPA)\",")
                         << QString("\"压强差(PA)\"") << qSetFieldWidth(0) <<endl;

                    for(int j =0;j < param->getUpdata().value(poly_factor_map.keys()[i]).count();++j)
                    {
                        stream.setFieldWidth(32);
                        node_Data data = param->getUpdata().value(poly_factor_map.keys()[i]).at(j);
                        stream << QString("\"%1\",").arg(QDateTime::fromTime_t(data.unix_time).toString("yyyy-MM-dd hh:mm:ss")) << QString("\"%1\",").arg(data.temp,0,'f',3)
                               << QString("\"%1\",").arg(data.pressure,0,'f',3) << QString("\"%1\",").arg(data.ref_pressure)
                               << QString("\"%1\"").arg(data.div_pressure,0,'f',3)<< qSetFieldWidth(0) << endl;
                    }

                    stream << QString("\"=======================降温标定采样数据==============\"") << endl;
                    stream.setFieldWidth(30);
                    stream<<QString("\"时间\",") << QString("\"温度(℃)\",") << QString("\"实际压强(PA)\",") << QString("\"负载压强(kPA)\",")
                         << QString("\"压强差(PA)\"") << qSetFieldWidth(0) <<endl;

                    for(int j =0;j < param->getDowndata().value(poly_factor_map.keys()[i]).count();++j)
                    {
                        stream.setFieldWidth(32);
                        node_Data data = param->getDowndata().value(poly_factor_map.keys()[i]).at(j);
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
    ui->progressBar->setVisible(false);
}

//将拟合系数写到节点中
void MainWindow::on_pbt_downloadToNode_clicked()
{
    ui->progressBar->setVisible(true);
    ui->pbt_downloadToNode->setEnabled(false);

    QMap<QSerialPort *,nodeShareedPtrParamList> map_downloadNodeParam;
    map_downloadNodeParam.clear();

    for(int i = 0;i < m_polyfit_treemodel->invisibleRootItem()->rowCount();++i)
    {
        StandardItem *item = static_cast<StandardItem *>(m_polyfit_treemodel->invisibleRootItem()->child(i));
        if(item->getIsChecked())
        {
            nodeShareedPtrParamList list;
            QSerialPort *serialPort=NULL;
            list.clear();

            for(int port_index = 0; port_index < m_MapPortAndSelectedNodeParam.count();++port_index)
            {
                if(m_MapPortAndSelectedNodeParam.values().at(port_index).first()->getNodePort().portName() == item->portItem()->text())
                {
                    serialPort = m_MapPortAndSelectedNodeParam.keys().at(port_index);
                    list = m_MapPortAndSelectedNodeParam.values().at(port_index);
                    break;
                }
            }
            if(!list.isEmpty())
            {
                for(int addr_index = 0;addr_index < list.count();++addr_index)
                {
                    if(item->addrItem()->text().toUInt() == list.at(addr_index)->getNodeAddr())
                    {
                        map_downloadNodeParam[serialPort].append(list.at(addr_index));
                        break;
                    }
                }
            }
        }
    }

    int total = 0;
    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator iter = map_downloadNodeParam.begin();
    while(iter != map_downloadNodeParam.end())
    {
        total += iter.value().count()*6*5;
        ++iter;
    }

    if(!total)
    {
        QMessageBox::information(this,"提示","请先选择要下载的点!",QMessageBox::Ok);

        ui->progressBar->setVisible(false);
        ui->pbt_downloadToNode->setEnabled(true);
        return;
    }

    QString downloadInfo;
    QString successAddrInfo;

    downloadInfo.append(QString("本次参与下载点总数为:%1,").arg(total/6/5));

    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(0);

    iter = map_downloadNodeParam.begin();
    while(iter != map_downloadNodeParam.end())
    {
        for(int node_index = 0; node_index < iter.value().count(); ++node_index)
        {
            QSharedPointer<NodeParam> param = iter.value()[node_index];

            QMap<nodePayload,std::vector<double>> poly_factor_map = param->getPolyFactorMap();
            for(int i = 0;i < poly_factor_map.count();++i)
            {
                for(int index = 0; index < poly_factor_map.values()[i].size();++index)
                {
                    ui->progressBar->setValue(ui->progressBar->value()+1);

                    int send_total = 4;
                    //当下载失败后，最多重试3次
                    for(int resend_count = 0;resend_count < send_total;++resend_count)
                    {
                        //首先下载
                        _displayOutInfo(QString("正在下载串口%1的节点%2%3%4次项系数...").arg(param->getNodePort().portName())
                                        .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));

                        sendSetPayloadFactor(param->getNodeAddr(),iter.key(),poly_factor_map.keys()[i],index,poly_factor_map.values()[i].at(index)*1000);

                        iter.key()->waitForReadyRead(500);
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
                                iter.key()->waitForReadyRead(500);
                                array.append(iter.key()->readAll());//将缓冲区所有的数据都读取出来
                                QCoreApplication::processEvents();
                            }
                            t.stop();
                            if( time_out && ((send_total-1) == resend_count) ) //重试失败，退出
                            {
                                _displayOutInfo(QString("等待串口%1的节点%2%3%4次项系数回复超时，请重新尝试!").arg(param->getNodePort().portName())
                                                .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));
                                ui->progressBar->setVisible(false);
                                ui->pbt_downloadToNode->setEnabled(true);
                                //统计结果
                                statisticDownloadResult(downloadInfo,successAddrInfo);
                                return;
                            }
                            if(!time_out)   //未超时
                                break;
                        }else
                        {
                            if((send_total-1) == resend_count)
                            {
                                _displayOutInfo(QString("串口%1的节点%2%3%4拟合系数下载失败，请重新尝试!").arg(param->getNodePort().portName())
                                                .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));
                                ui->progressBar->setVisible(false);
                                ui->pbt_downloadToNode->setEnabled(true);
                                //统计结果
                                statisticDownloadResult(downloadInfo,successAddrInfo);
                                return;
                            }
                        }

                        QElapsedTimer t;    //相邻数据请求，最小时间5ms
                        t.start();
                        while(t.elapsed()<150)
                            QCoreApplication::processEvents();
                    }

                    QElapsedTimer t;    //相邻数据请求，最小时间5ms
                    t.start();
                    while(t.elapsed()<150)
                        QCoreApplication::processEvents();

                    //当读取失败后，最多重试3次
                    for(int resend_count = 0;resend_count < send_total;++resend_count)
                    {
                        //然后读取寄存器数值进行验证
                        _displayOutInfo(QString("正在读取串口%1的节点%2%3%4次项系数...").arg(param->getNodePort().portName())
                                        .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));

                        bool is_check_ok = false;
                        double factor = 0;
                        sendReadPayloadFactor(param->getNodeAddr(),iter.key(),poly_factor_map.keys()[i],index);

                        iter.key()->waitForReadyRead(500);
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
                            while(!time_out && array.count() < 9)       //读取是否回复正确，错误回复小于9字节
                            {
                                iter.key()->waitForReadyRead(500);
                                array.append(iter.key()->readAll());//将缓冲区所有的数据都读取出来
                                QCoreApplication::processEvents();
                            }
                            t.stop();

                            if(!time_out && !array.isEmpty())
                            {
                                char * data = array.data();
                                factor = ntohf(*((float*)(data+3)));
                                double download_factor = poly_factor_map.values()[i].at(index)*1000;
    //                            qDebug() << QString::number(factor) << QString::number(download_factor);
                                if(abs(factor-download_factor) < 10000)
                                    is_check_ok = true;
                            }else
                            {
                                if((send_total-1) == resend_count)
                                {
                                    _displayOutInfo(QString("读取串口%1的节点%2%3%4次项系数回复超时，请重新尝试!").arg(param->getNodePort().portName())
                                                    .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));
                                    ui->progressBar->setVisible(false);
                                    ui->pbt_downloadToNode->setEnabled(true);
                                    //统计结果
                                    statisticDownloadResult(downloadInfo,successAddrInfo);
                                    return;
                                }
                            }
                        }else
                        {
                            if((send_total-1) == resend_count)
                            {
                                _displayOutInfo(QString("串口%1的节点%2%3%4拟合系数读取失败，请重新尝试!").arg(param->getNodePort().portName())
                                                .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));
                                ui->progressBar->setVisible(false);
                                ui->pbt_downloadToNode->setEnabled(true);
                                //统计结果
                                statisticDownloadResult(downloadInfo,successAddrInfo);
                                return;
                            }
                        }
                        if(is_check_ok)
                        {
                            _displayOutInfo(QString("获取到串口%1的节点%2%3%4次项系数值为%5").arg(param->getNodePort().portName())
                                            .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index)
                                            .arg(QString::number(factor)));
                            _displayOutInfo(QString("验证串口%1的节点%2%3%4次项系数成功!").arg(param->getNodePort().portName())
                                            .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));
                            break;
                        }else
                        {
                            if((send_total-1) == resend_count)
                            {
                                _displayOutInfo(QString("验证串口%1的节点%2%3%4次项系数失败!").arg(param->getNodePort().portName())
                                                .arg(param->getNodeAddr()).arg(getStrPayLoad(poly_factor_map.keys()[i])).arg(index));
                                ui->progressBar->setVisible(false);
                                ui->pbt_downloadToNode->setEnabled(true);
                                //统计结果
                                statisticDownloadResult(downloadInfo,successAddrInfo);
                                return;
                            }
                        }

                        QElapsedTimer t;    //相邻数据请求，最小时间5ms
                        t.start();
                        while(t.elapsed()<150)
                            QCoreApplication::processEvents();
                    }

                    t.restart();//相邻数据请求，最小时间5ms
                    while(t.elapsed()<150)
                        QCoreApplication::processEvents();
                }
            }

            successAddrInfo.append(QString("%1-%2,").arg(param->getNodePort().portName()).arg(param->getNodeAddr()));
        }
        ++iter;
    }

    ui->statusBar->showMessage("下载并校验成功",5000);
    ui->progressBar->setVisible(false);
    ui->pbt_downloadToNode->setEnabled(true);

    //统计结果
    statisticDownloadResult(downloadInfo,successAddrInfo);
}

void MainWindow::statisticDownloadResult(QString downloadInfo, QString successAddrInfo)
{
    QStringList success_list = successAddrInfo.split(",",QString::SkipEmptyParts);
    downloadInfo.append(QString("成功设备总数:%1,包括:%2!").arg(success_list.count()).arg(success_list.count()?success_list.join(","):QString("null")));
    _displayOutInfo(downloadInfo);
}

/**
 *  标定曲线展示
 * @brief MainWindow::on_pbt_calibrationDisplay_clicked
 */
void MainWindow::on_pbt_calibrationDisplay_clicked()
{
    CalibrationDisplay calibration_display(&m_MapPortAndSelectedNodeParam);
    calibration_display.move( (QApplication::desktop()->width()-calibration_display.width())/2,
                              (QApplication::desktop()->height()-calibration_display.height())/2 );
    calibration_display.exec();
}


//原始数据展示
void MainWindow::on_pbt_datasDisplay_clicked()
{
    if(!m_originalDataDisplay->isHidden())
        return;
    m_originalDataDisplay->initOriginalDataDisplay(&m_MapPortAndSelectedNodeParam);
    m_originalDataDisplay->move( (QApplication::desktop()->width()-m_originalDataDisplay->width())/2,
                                 (QApplication::desktop()->height()-m_originalDataDisplay->height())/2 );
    m_originalDataDisplay->show();
}

void MainWindow::on_treeView_Polyfit_doubleClicked(const QModelIndex &index)
{
    if(!index.parent().isValid())   //可折叠的行
    {
        if(ui->treeView_Polyfit->isExpanded(m_polyfit_treemodel->index(index.row(),0)))
        {
            ui->treeView_Polyfit->collapse(m_polyfit_treemodel->index(index.row(),0));
        }else
        {
            ui->treeView_Polyfit->expand(m_polyfit_treemodel->index(index.row(),0));
        }
    }
}
