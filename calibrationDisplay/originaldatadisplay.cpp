#include "originaldatadisplay.h"
#include "ui_originaldatadisplay.h"
#include <QDebug>

OriginalDataDisplay::OriginalDataDisplay(DBOperate *db_operate, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OriginalDataDisplay)
{
    ui->setupUi(this);

    this->resize(QApplication::desktop()->width()*0.8,QApplication::desktop()->height()*0.6);

    m_db_operate = db_operate;

    _initCustomPlot();
    _initTableView();
}

OriginalDataDisplay::~OriginalDataDisplay()
{
    delete ui;
}

void OriginalDataDisplay::initOriginalDataDisplay(QMap<QSerialPort *, nodeShareedPtrParamList> *mapSerialPtrAndNodeList)
{
    m_mapSerialPtrAndNodeList = mapSerialPtrAndNodeList;

    m_mapPortStrAndAddrList.clear();
    ui->cbx_ports->clear();
    ui->cbx_addrs->clear();
    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator iter = m_mapSerialPtrAndNodeList->begin();
    while(iter != m_mapSerialPtrAndNodeList->end())
    {
        QList<QString> addr_list;
        addr_list.clear();
        for(int i = 0;i < iter.value().count();++i)
        {
            addr_list.append(QString::number(iter.value()[i]->getNodeAddr()));
        }
        if(!addr_list.isEmpty())
        {
            m_mapPortStrAndAddrList[iter.value().first()->getNodePort().portName()] = qMakePair(iter.key(),addr_list);
        }
        ++iter;
    }

    ui->cbx_ports->addItems(m_mapPortStrAndAddrList.keys());
    on_cbx_ports_currentIndexChanged(ui->cbx_ports->currentText());
}

void OriginalDataDisplay::on_cbx_ports_currentIndexChanged(const QString &arg1)
{
    if(arg1.isEmpty())
        return;

    disconnect(ui->cbx_addrs,SIGNAL(currentIndexChanged(QString)),this,SLOT(on_cbx_addrs_currentIndexChanged(QString)));
    ui->cbx_addrs->clear();
    ui->cbx_addrs->addItems(m_mapPortStrAndAddrList.value(arg1).second);
    connect(ui->cbx_addrs,SIGNAL(currentIndexChanged(QString)),this,SLOT(on_cbx_addrs_currentIndexChanged(QString)));
    on_cbx_addrs_currentIndexChanged(ui->cbx_addrs->itemText(0));
}

void OriginalDataDisplay::mousePressEvent4Plot(QMouseEvent *e)
{
    QCPGraph *graph = qobject_cast<QCPGraph*>(ui->customplot->plottableAt(e->pos(),true));
    QRect rect(0,0,1,1);
    double key,value;
    m_pressPos.setX(0);
    m_pressPos.setY(0);

    if(graph)
    {
        if(m_tracer != NULL)
        {
            delete m_tracer;
            m_tracer = NULL;
        }
        if(m_textTip != NULL)
        {
            delete m_textTip;
            m_textTip = NULL;
        }
        if(m_tracer == NULL)
        {
            m_tracer = new QCPItemTracer(ui->customplot);
            m_tracer->setParent(ui->customplot);
            m_tracer->setPen(QColor(255,255,255));
            m_tracer->setBrush(QBrush(QColor(255,0,0),Qt::SolidPattern));
            m_tracer->setStyle(QCPItemTracer::tsCircle);
            m_tracer->setSize(10);
            m_tracer->setVisible(false);
        }
        if(m_textTip == NULL)
        {
            m_textTip = new QCPItemText(ui->customplot);
            m_textTip->setPositionAlignment(Qt::AlignLeft|Qt::AlignHCenter);
            m_textTip->position->setType(QCPItemPosition::ptAbsolute);
            QFont font;
            font.setPixelSize(12);
            m_textTip->setPadding(QMargins(2,2,2,2));
            m_textTip->setFont(font); // make font a bit larger
            m_textTip->setPen(QPen(Qt::black)); // show black border around text
            m_textTip->setBrush(Qt::white);
            m_textTip->setVisible(false);
        }
        double x1 = ui->customplot->xAxis->coordToPixel(ui->customplot->xAxis->range().lower);
        double x2 = ui->customplot->xAxis->coordToPixel(ui->customplot->xAxis->range().upper);
        double y1 = ui->customplot->yAxis->coordToPixel(ui->customplot->yAxis->range().lower);
        double y2 = ui->customplot->yAxis->coordToPixel(ui->customplot->yAxis->range().upper);
        m_textTipMargin.clear();
        m_textTipMargin<<x1<<x2<<y1<<y2;
        QPoint p;p.setX(e->pos().x());p.setY(e->pos().y());
        foreach(QCPData data,graph->data()->values())
        {
            double posKey = ui->customplot->xAxis->coordToPixel(data.key);
            if(qAbs(posKey-e->pos().x())<=10)
            {
                key = data.key;
                value = data.value;
                double posx = graph->keyAxis()->coordToPixel(data.key);
                double posy = graph->valueAxis()->coordToPixel(data.value);
                rect.setRect(posx-10,posy-10,21,21);
                if(!rect.contains(e->pos()))
                    continue;
                else
                    break;
            }
            if(posKey-e->pos().x()>10)
                break;
        }
        if(!graph->realVisibility())
        {
            m_tracer->setVisible(false);
            m_pressPos.setX(0);
            m_pressPos.setY(0);
            ui->customplot->replot();
        }
        else if(rect.contains(e->pos()))
        {
            m_tracer->setGraph(graph);
            m_tracer->setGraphKey(key);
            m_tracer->setVisible(true);
            ui->customplot->replot();
            m_pressPos.setX(key);
            m_pressPos.setY(value);
        }
        else
        {
            if(m_tracer->visible())
            {
                m_tracer->setVisible(false);
                m_pressPos.setX(0);
                m_pressPos.setY(0);
                ui->customplot->replot();
            }
            if(m_textTip->visible())
            {
                m_textTip->setVisible(false);
                m_pressPos.setX(0);
                m_pressPos.setY(0);
                ui->customplot->replot();
            }
        }
    }
    else
    {
        if(!m_tracer == NULL)
        {
            if(m_tracer->visible())
            {
                m_tracer->setVisible(false);
                m_pressPos.setX(0);
                m_pressPos.setY(0);
                ui->customplot->replot();
            }
        }

        if(!m_textTip == NULL)
        {
            if(m_textTip->visible())
            {
                m_textTip->setVisible(false);
                m_pressPos.setX(0);
                m_pressPos.setY(0);
                ui->customplot->replot();
            }
        }
    }

    if(m_pressPos.x()!=0 && m_pressPos.y()!=0)
    {
        if(graph->valueAxis() == ui->customplot->yAxis)
        {
            m_textTip->setText("时间: "+ QDateTime::fromTime_t(m_pressPos.x()).toString("yyyy-MM-dd hh:mm:ss")\
                               +"\n压强差: "+QString::number(m_pressPos.y())+"PA");
        }else
        {
            m_textTip->setText("时间: "+ QDateTime::fromTime_t(m_pressPos.x()).toString("yyyy-MM-dd hh:mm:ss")\
                               +"\n温度: "+QString::number(m_pressPos.y(),'f',2)+"℃");
        }
        double x,y;
        double width=m_textTip->right->pixelPoint().x()-m_textTip->left->pixelPoint().x();
        double height=m_textTip->bottom->pixelPoint().y()-m_textTip->top->pixelPoint().y();
        if(e->pos().x()+10+width>m_textTipMargin[1])
            x = e->pos().x()-10-width/2;
        else
            x = e->pos().x()+10+width/2;
        if(e->pos().y()+10+height>m_textTipMargin[2])
            y = e->pos().y()-10-height;
        else
            y = e->pos().y()+10;
        m_textTip->position->setCoords(x,y);
        m_textTip->setVisible(true);
        ui->customplot->replot();
    }
}

void OriginalDataDisplay::selectionChangedByUser()
{
    for (int i=0; i<ui->customplot->graphCount();++i)
    {
        QCPGraph *graph = ui->customplot->graph(i);
        QCPPlottableLegendItem *item = ui->customplot->legend->itemWithPlottable(graph);
        if(item != NULL && item->selected())
        {
            item->setSelected(false);
            graph->setVisible(!graph->visible());
            if(graph->visible())
            {
                graph->setName(graph->name().split(':')[0]);
                graph->setSelectable(true);
            }
            else
            {
                graph->setName(graph->name().split(':')[0]+":被隐藏");
                graph->setSelectable(false);
            }
        }
    }
}

void OriginalDataDisplay::slot_dateTimeButtonClicked()
{
    CDateTimeButton *d = qobject_cast<CDateTimeButton*>(QObject::sender());
    m_calendarWidget->setDateTime(QDateTime::fromString(d->text(),"yyyy-MM-dd HH:mm:ss"));
    m_calendarWidget->setMaximumDate(d->maximumDateTime().date());
    m_calendarWidget->setMinimumDate(d->minimumDateTime().date());

    disconnect(m_calendarWidget,SIGNAL(dateTimeSelect(QDateTime)),ui->pbt_startTime,SLOT(setDateTime(QDateTime)));
    disconnect(m_calendarWidget,SIGNAL(dateTimeSelect(QDateTime)),ui->pbt_endTime,SLOT(setDateTime(QDateTime)));
    connect(m_calendarWidget,SIGNAL(dateTimeSelect(QDateTime)),d,SLOT(setDateTime(QDateTime)));

    QPoint p1 = d->mapToGlobal(d->rect().bottomLeft());
    QRect screen = QApplication::desktop()->rect();
    if(p1.x()+m_calendarWidget->width()>screen.right())
        p1.setX(screen.right()-m_calendarWidget->width());
    m_calendarWidget->move(p1);
    if(m_calendarWidget->isHidden())
        m_calendarWidget->show();
}

void OriginalDataDisplay::_initCustomPlot()
{
    //QCustomPlot init
    ui->customplot->plotLayout()->insertRow(0);
    ui->customplot->plotLayout()->addElement(0, 0, new QCPPlotTitle(ui->customplot, "原始数据展示图"));
    qobject_cast<QCPPlotTitle *>(ui->customplot->plotLayout()->element(0,0))->setFont(QFont("Times", 10, QFont::Bold));
    ui->customplot->setInteractions(QCP::iSelectLegend| QCP::iRangeDrag | QCP::iRangeZoom );
    ui->customplot->setBackground(QBrush(QColor(238,238,238)));//设置绘图背景颜色
    ui->customplot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->customplot->xAxis->setDateTimeFormat("yyyy-MM-dd\nHH:mm:ss");
    ui->customplot->xAxis->setLabel("时间");
    ui->customplot->axisRect()->setRangeZoomFactor(0.5,1);


    ui->customplot->yAxis2->setVisible(true);
    ui->customplot->yAxis2->setTicks(true);

    QFont font;
    font.setPixelSize(12);
    ui->customplot->xAxis->setLabelFont(font);
    ui->customplot->yAxis->setLabelFont(font);
    ui->customplot->yAxis2->setLabelFont(font);
    ui->customplot->xAxis->setTickLabelFont(font);
    ui->customplot->yAxis->setTickLabelFont(font);
    ui->customplot->yAxis2->setTickLabelFont(font);

    ui->customplot->legend->setVisible(true);
    ui->customplot->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items
    QBrush brush(QColor(255,255,255,50),Qt::SolidPattern);
    ui->customplot->legend->setBrush(brush);
    ui->customplot->legend->setFont(font);

    ui->customplot->xAxis->setRange(0,10);
    ui->customplot->yAxis->setRange(0,10);
    ui->customplot->yAxis->setLabel("压强差(PA)");

    ui->customplot->yAxis2->setRange(0,10);
    ui->customplot->yAxis2->setLabel("温度(℃)");

    connect(ui->customplot,SIGNAL(selectionChangedByUser()),this,SLOT(selectionChangedByUser()));
    connect(ui->customplot,SIGNAL(mousePress(QMouseEvent*)),this,SLOT(mousePressEvent4Plot(QMouseEvent*)));

    //原始数据
    QCPGraph *graph = ui->customplot->addGraph();
    graph->setName("满量程负载数据");
    graph->setPen(QColor(0,50,100));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph();
    graph->setName("五分之四量程负载数据");
    graph->setPen(QColor(0,100,50));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph();
    graph->setName("五分之三量程负载数据");
    graph->setPen(QColor(50,0,100));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph();
    graph->setName("五分之二量程负载数据");
    graph->setPen(QColor(50,150,0));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph();
    graph->setName("五分之一量程负载数据");
    graph->setPen(QColor(100,50,0));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph();
    graph->setName("零量程负载数据");
    graph->setPen(QColor(0,50,150));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));


    graph = ui->customplot->addGraph(ui->customplot->xAxis,ui->customplot->yAxis2);
    graph->setName("满量程负载温度曲线图");
    graph->setPen(QColor(100,250,100));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph(ui->customplot->xAxis,ui->customplot->yAxis2);
    graph->setName("五分之四量程温度曲线图");
    graph->setPen(QColor(250,100,100));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph(ui->customplot->xAxis,ui->customplot->yAxis2);
    graph->setName("五分之三量程温度曲线图");
    graph->setPen(QColor(100,100,250));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph(ui->customplot->xAxis,ui->customplot->yAxis2);
    graph->setName("五分之二量程温度曲线图");
    graph->setPen(QColor(200,200,50));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph(ui->customplot->xAxis,ui->customplot->yAxis2);
    graph->setName("五分之一量程温度曲线图");
    graph->setPen(QColor(200,50,0));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    graph = ui->customplot->addGraph(ui->customplot->xAxis,ui->customplot->yAxis2);
    graph->setName("零量程温度曲线图");
    graph->setPen(QColor(200,0,150));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    m_tracer = NULL;
    m_textTip = NULL;

    m_calendarWidget = new CCalendarWidget;
    connect(ui->pbt_startTime,SIGNAL(clicked()),this,SLOT(slot_dateTimeButtonClicked()));
    connect(ui->pbt_endTime,SIGNAL(clicked()),this,SLOT(slot_dateTimeButtonClicked()));

    m_changeViewAction = new QAction("切换视图",this);
    m_changeDragAxis = new QAction("切换拖拽纵坐标",this);
    m_clearGraph = new QAction("清空视图",this);
    ui->pbt_switchPage->setMenu(new QMenu);
    ui->pbt_switchPage->menu()->addAction(m_changeViewAction);
    ui->pbt_switchPage->menu()->addSeparator();
    ui->pbt_switchPage->menu()->addAction(m_clearGraph);
    ui->pbt_switchPage->menu()->addSeparator();
    ui->pbt_switchPage->menu()->addAction(m_changeDragAxis);

    connect(m_changeViewAction,&QAction::triggered,[=](){
        if(ui->stackedWidget->currentWidget() == ui->pageGraphDisplay)
            ui->stackedWidget->setCurrentWidget(ui->pageTableDisplay);
        else
            ui->stackedWidget->setCurrentWidget(ui->pageGraphDisplay);
    });

    connect(m_changeDragAxis,&QAction::triggered,[=](){
        if(ui->customplot->axisRect()->rangeDragAxis(Qt::Vertical)==ui->customplot->yAxis)
        {
            ui->customplot->axisRect()->setRangeDragAxes(ui->customplot->xAxis,ui->customplot->yAxis2);
        }else
        {
            ui->customplot->axisRect()->setRangeDragAxes(ui->customplot->xAxis,ui->customplot->yAxis);
        }
    });

    connect(m_clearGraph,SIGNAL(triggered(bool)),this,SLOT(slot_clearGraph(bool)));


    ui->stackedWidget->setCurrentWidget(ui->pageGraphDisplay);
}

void OriginalDataDisplay::_initTableView()
{
    //设备展示
    m_tableview_model = new TableViewModel(TABLE_NODEDATA);
    m_headerview = new HeaderView(Qt::Horizontal);
    m_item_delegate = new ItemDelegate(TABLE_NODEDATA);

    m_tableview_model->setColumnCount(8);
    m_tableview_model->setHeaderLabels(QStringList()<<"选择"<<"时间"<< "负载类型" << "温度(℃)" << "压强(Pa)" << "参照压强(KPa)" << "压强差(Pa)" <<"操作");

    ui->tableView->setModel(m_tableview_model);
    ui->tableView->setHorizontalHeader(m_headerview);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->setItemDelegate(m_item_delegate);
    ui->tableView->setColumnHidden(0,true);
    ui->tableView->setMouseTracking(true);

    ui->tableView->setColumnWidth(0,200);
    ui->tableView->setColumnWidth(1,200);
    for(int i = 2;i < m_tableview_model->columnCount();++i)
        ui->tableView->setColumnWidth(i,150);

    connect(m_headerview,SIGNAL(stateChanged(Qt::CheckState)),m_tableview_model,SLOT(slot_stateChanged(Qt::CheckState)));
    connect(m_tableview_model,SIGNAL(stateChanged(Qt::CheckState)),m_headerview,SLOT(slot_stateChanged(Qt::CheckState)));
    connect(m_item_delegate,SIGNAL(signal_deleteData(QModelIndex)),this,SLOT(slot_deleteData(QModelIndex)));
}

//更新时间
void OriginalDataDisplay::on_cbx_addrs_currentIndexChanged(const QString &arg1)
{
    if(arg1.isEmpty())
        return;

    uint min_time = 0;
    uint max_time = 0;
    quint16 addr=arg1.toUInt();
    QSerialPort *current_port = m_mapPortStrAndAddrList[ui->cbx_ports->currentText()].first;
    foreach (const QSharedPointer<NodeParam> &param, (*m_mapSerialPtrAndNodeList)[current_port]) {
        if(param->getNodeAddr() == addr)
        {
            min_time = param->getNodeDataMinTime();
            max_time = param->getNodeDataMaxTime();
            break;
        }
    }

    ui->pbt_startTime->setMaximumDateTime(QDateTime::fromTime_t(max_time));
    ui->pbt_startTime->setMinimumDateTime(QDateTime::fromTime_t(min_time));
    ui->pbt_endTime->setMaximumDateTime(QDateTime::fromTime_t(max_time));
    ui->pbt_endTime->setMinimumDateTime(QDateTime::fromTime_t(min_time));

    if(min_time == 0)
    {
        ui->pbt_startTime->setDateTime(QDateTime());
    }else
    {
        ui->pbt_startTime->setDateTime(QDateTime::fromTime_t(min_time));
    }
    if(max_time == 0)
    {
        ui->pbt_endTime->setDateTime(QDateTime());
    }else
    {
        ui->pbt_endTime->setDateTime(QDateTime::fromTime_t(max_time));
    }
}

//查询原始数据
void OriginalDataDisplay::on_pbt_queryDatas_clicked()
{
    slot_clearGraph(true);

    uint start_time = QDateTime::fromString(ui->pbt_startTime->text(),"yyyy-MM-dd HH:mm:ss").toTime_t();
    uint end_time = QDateTime::fromString(ui->pbt_endTime->text(),"yyyy-MM-dd HH:mm:ss").toTime_t();
    QVector<double> unix_times,data_pressures,data_temps;
    if(start_time > end_time)
    {
        QMessageBox::information(this,"提示","起始时间不能大于结束时间,\n请重新设置!",QMessageBox::Ok);
        return;
    }

    //数据获取
    quint16 addr=ui->cbx_addrs->currentText().toUInt();
    QWeakPointer<NodeParam> current_node;
    QSerialPort *current_port = m_mapPortStrAndAddrList[ui->cbx_ports->currentText()].first;
    foreach (QSharedPointer<NodeParam> node, m_mapSerialPtrAndNodeList->value(current_port)) {
        if(node->getNodeAddr() == addr)
        {
            m_db_operate->getNodeDatasFromStartTimeToEndTime(node,start_time,end_time);
            current_node = QWeakPointer<NodeParam>(node);
            break;
        }
    }

    if(current_node.isNull())
        return;

    QMap<nodePayload, QList<node_Data> > node_datas_list = current_node.lock()->getNodedatas();
    QMap<nodePayload, QList<node_Data> >::const_iterator iter = node_datas_list.begin();
    while(iter != node_datas_list.end())
    {
        unix_times.clear();
        data_pressures.clear();
        data_temps.clear();

        for(int i = 0;i < iter.value().count();++i)
        {
            unix_times.append(iter.value().at(i).unix_time);
            data_pressures.append(iter.value().at(i).div_pressure);
            data_temps.append(iter.value().at(i).temp);
        }

        //表格展示
        m_tableview_model->updateSampleDataList(iter.value(),false);
        //展示曲线
        ui->customplot->graph(iter.key())->setData(unix_times,data_pressures);
        ui->customplot->graph(iter.key()+6)->setData(unix_times,data_temps);

        ++iter;
    }

    ui->customplot->rescaleAxes();
    ui->customplot->xAxis->setRangeUpper(ui->customplot->xAxis->range().upper+ui->customplot->xAxis->range().size()*0.1);
    ui->customplot->xAxis->setRangeLower(ui->customplot->xAxis->range().lower-ui->customplot->xAxis->range().size()*0.1);
    ui->customplot->yAxis->setRangeUpper(ui->customplot->yAxis->range().upper+ui->customplot->yAxis->range().size()*0.25);
    ui->customplot->yAxis->setRangeLower(ui->customplot->yAxis->range().lower-ui->customplot->yAxis->range().size()*0.25);
    ui->customplot->yAxis2->setRangeUpper(ui->customplot->yAxis2->range().upper+ui->customplot->yAxis2->range().size()*0.25);
    ui->customplot->yAxis2->setRangeLower(ui->customplot->yAxis2->range().lower-ui->customplot->yAxis2->range().size()*0.25);
    ui->customplot->replot();
}

void OriginalDataDisplay::slot_deleteData(const QModelIndex index)
{
    int rect = QMessageBox::information(this,"提示","确认删除该条数据吗?\n此操作不可撤销!",QMessageBox::Ok,QMessageBox::No);
    if(rect == QMessageBox::No)
        return;

    bool is_ok=false;
    node_Data data = m_tableview_model->getSampleDataByRow(index.row(),is_ok);
    if(!is_ok)
    {
        QMessageBox::information(this,"提示","删除失败!",QMessageBox::Ok);
        return;
    }

    m_tableview_model->removeRow(index.row());

    //删除图上的压强差数据
    ui->customplot->graph((int)data.payload_type)->removeData(data.unix_time);

    //删除图上的温度数据
    ui->customplot->graph((int)data.payload_type+6)->removeData(data.unix_time);
    ui->customplot->replot();

    //删除数据库中的数据
    m_db_operate->addDBTableOperate(data.serial_number,QString("delete from `%1` where unix_time = %2").arg(m_db_operate->getDBTableStrFromPayload(data.payload_type))
                                    .arg(data.unix_time));
    m_db_operate->commitTransaction(true);
}

void OriginalDataDisplay::slot_clearGraph(bool is_checked)
{
    Q_UNUSED(is_checked)

    //清空绘图
    for (int i = 0; i < ui->customplot->graphCount(); ++i) {
       ui->customplot->graph(i)->clearData();
    }
    ui->customplot->replot();
    m_tableview_model->clear();
}
