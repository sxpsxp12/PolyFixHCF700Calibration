#include "calibrationdisplay.h"
#include "ui_calibrationdisplay.h"
#include <algorithm>

CalibrationDisplay::CalibrationDisplay(QMap<QSerialPort *, nodeShareedPtrParamList> *mapSerialPtrAndNodeList, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrationDisplay)
{
    ui->setupUi(this);

    this->resize(QApplication::desktop()->width()*0.8,QApplication::desktop()->height()*0.6);

    m_mapSerialPtrAndNodeList = mapSerialPtrAndNodeList;

    _initCustomPlot();
}

CalibrationDisplay::~CalibrationDisplay()
{
    delete ui;
}

void CalibrationDisplay::_initCustomPlot()
{
    m_mapPortStrAndAddrList.clear();
    ui->cbx_ports->clear();
    ui->cbx_addrs->clear();
    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator iter = m_mapSerialPtrAndNodeList->begin();
    while(iter != m_mapSerialPtrAndNodeList->end())
    {
        for(int i = 0;i < iter.value().count();++i)
        {
            m_mapPortStrAndAddrList[iter.value()[i]->getNodePort().portName()].append(QString::number(iter.value()[i]->getNodeAddr()));
        }
        ++iter;
    }

    ui->cbx_ports->addItems(m_mapPortStrAndAddrList.keys());
    on_cbx_ports_currentIndexChanged(ui->cbx_ports->currentText());

    //QCustomPlot init
    ui->customplot->plotLayout()->insertRow(0);
    ui->customplot->plotLayout()->addElement(0, 0, new QCPPlotTitle(ui->customplot, "标定数据展示图"));
    qobject_cast<QCPPlotTitle *>(ui->customplot->plotLayout()->element(0,0))->setFont(QFont("Times", 10, QFont::Bold));
    ui->customplot->setInteractions(QCP::iSelectLegend| QCP::iRangeDrag | QCP::iRangeZoom );
    ui->customplot->setBackground(QBrush(QColor(238,238,238)));//设置绘图背景颜色
    ui->customplot->xAxis->setTickLabelType(QCPAxis::ltNumber);
    ui->customplot->xAxis->setLabel("温度");
    ui->customplot->axisRect()->setRangeZoomFactor(0.5,1);

    QFont font;
    font.setPixelSize(12);
    ui->customplot->xAxis->setLabelFont(font);
    ui->customplot->yAxis->setLabelFont(font);
    ui->customplot->xAxis->setTickLabelFont(font);
    ui->customplot->yAxis->setTickLabelFont(font);

    ui->customplot->legend->setVisible(true);
    ui->customplot->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items
    QBrush brush(QColor(255,255,255,50),Qt::SolidPattern);
    ui->customplot->legend->setBrush(brush);
    ui->customplot->legend->setFont(font);

    ui->customplot->xAxis->setRange(0,10);
    ui->customplot->yAxis->setRange(0,10);
    ui->customplot->yAxis->setLabel("压强差(PA)");

    connect(ui->customplot,SIGNAL(selectionChangedByUser()),this,SLOT(selectionChangedByUser()));
    connect(ui->customplot,SIGNAL(mousePress(QMouseEvent*)),this,SLOT(mousePressEvent4Plot(QMouseEvent*)));

    //原始数据
    QCPGraph *graph = ui->customplot->addGraph();
    graph->setName("满量程负载数据");
    graph->setPen(QColor(0,50,100));
    graph->setLineStyle(QCPGraph::lsNone);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));

    graph = ui->customplot->addGraph();
    graph->setName("五分之四量程负载数据");
    graph->setPen(QColor(0,100,50));
    graph->setLineStyle(QCPGraph::lsNone);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));

    graph = ui->customplot->addGraph();
    graph->setName("五分之三量程负载数据");
    graph->setPen(QColor(50,0,100));
    graph->setLineStyle(QCPGraph::lsNone);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));

    graph = ui->customplot->addGraph();
    graph->setName("五分之二量程负载数据");
    graph->setPen(QColor(50,150,0));
    graph->setLineStyle(QCPGraph::lsNone);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));

    graph = ui->customplot->addGraph();
    graph->setName("五分之一量程负载数据");
    graph->setPen(QColor(100,50,0));
    graph->setLineStyle(QCPGraph::lsNone);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));

    graph = ui->customplot->addGraph();
    graph->setName("零量程负载数据");
    graph->setPen(QColor(0,50,150));
    graph->setLineStyle(QCPGraph::lsNone);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));


    graph = ui->customplot->addGraph();
    graph->setName("满量程负载标定曲线图");
    graph->setPen(QColor(100,250,100));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));

    graph = ui->customplot->addGraph();
    graph->setName("五分之四量程负载标定曲线图");
    graph->setPen(QColor(250,100,100));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));

    graph = ui->customplot->addGraph();
    graph->setName("五分之三量程负载标定曲线图");
    graph->setPen(QColor(100,100,250));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));

    graph = ui->customplot->addGraph();
    graph->setName("五分之二量程负载标定曲线图");
    graph->setPen(QColor(200,200,50));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));

    graph = ui->customplot->addGraph();
    graph->setName("五分之一量程负载标定曲线图");
    graph->setPen(QColor(200,50,0));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));

    graph = ui->customplot->addGraph();
    graph->setName("零量程负载标定曲线图");
    graph->setPen(QColor(200,0,150));
    graph->setLineStyle(QCPGraph::lsLine);
    graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));

    m_tracer = NULL;
    m_textTip = NULL;
}

void CalibrationDisplay::on_pbt_querycalibration_clicked()
{
    QString port = ui->cbx_ports->currentText();
    int addr = ui->cbx_addrs->currentText().toInt();
    QSharedPointer<NodeParam> node_param;

    qobject_cast<QCPPlotTitle *>(ui->customplot->plotLayout()->element(0,0))->setText(QString("串口%1的节点%2标定数据图").arg(port).arg(addr));
    QMap<QSerialPort *,nodeShareedPtrParamList>::iterator iter = m_mapSerialPtrAndNodeList->begin();
    while(iter != m_mapSerialPtrAndNodeList->end())
    {
        if(iter.value().first()->getNodePort().portName() == port)
        {
            for(int i = 0;i < iter.value().count();++i)
            {
                if(iter.value()[i]->getNodeAddr() == addr)
                {
                    node_param = iter.value()[i];
                    break;
                }
            }
            break;
        }
        ++iter;
    }

    if(node_param->getNodeAddr() != addr || node_param->getNodePort().portName() != port)
        return;

    QMap<nodePayload,std::vector<double>> poly_factor_map = node_param->getPolyFactorMap();
    QMap<nodePayload,std::vector<double>>::iterator payloadIter = poly_factor_map.begin();
    while(payloadIter != poly_factor_map.end())
    {
        QList<node_Data> up_list = node_param->getUpdata().value(payloadIter.key());
        QList<node_Data> down_list = node_param->getDowndata().value(payloadIter.key());
        QVector<double> origin_temp,origin_div_pressure;
        QVector<double> poly_temp,poly_div_pressure;

        //原始曲线数据
        for(int i = 0; i < up_list.count(); ++i)
        {
            origin_temp.append(up_list.at(i).temp);
            origin_div_pressure.append(up_list.at(i).div_pressure);
        }

        for(int i = down_list.count()-1; i >= 0; --i)
        {
            origin_temp.append(down_list.at(i).temp);
            origin_div_pressure.append(down_list.at(i).div_pressure);
        }


        //标定曲线数据
        double start_temp = origin_temp[0],end_temp = origin_temp[0];
        for(int i = 0;i < origin_temp.count();++i)
        {
            if(start_temp > origin_temp.at(i))
                start_temp = origin_temp.at(i);
            if(end_temp < origin_temp.at(i))
                end_temp = origin_temp.at(i);
        }

        poly_temp = _divideTemp(start_temp,end_temp);
        for(int i = 0; i < poly_temp.count();++i)
        {
            poly_div_pressure.append(node_param->getFactorComputeY(payloadIter.key(),poly_temp.at(i)));
        }

        int origin_index = payloadIter.key();
        int calibration_index = origin_index + 6;
        ui->customplot->graph(origin_index)->setData(origin_temp,origin_div_pressure);
        ui->customplot->graph(calibration_index)->setData(poly_temp,poly_div_pressure);

        ++payloadIter;
    }
    ui->customplot->rescaleAxes();
    ui->customplot->xAxis->setRangeUpper(ui->customplot->xAxis->range().upper+ui->customplot->xAxis->range().size()*0.1);
    ui->customplot->xAxis->setRangeLower(ui->customplot->xAxis->range().lower-ui->customplot->xAxis->range().size()*0.1);
    ui->customplot->yAxis->setRangeUpper(ui->customplot->yAxis->range().upper+ui->customplot->yAxis->range().size()*0.25);
    ui->customplot->yAxis->setRangeLower(ui->customplot->yAxis->range().lower-ui->customplot->yAxis->range().size()*0.25);
    ui->customplot->replot();
}

/**
 *  更新串口
 * @brief CalibrationDisplay::on_cbx_ports_currentIndexChanged
 * @param arg1
 */
void CalibrationDisplay::on_cbx_ports_currentIndexChanged(const QString &arg1)
{
    if(arg1.isEmpty())
        return;
    ui->cbx_addrs->clear();
    ui->cbx_addrs->addItems(m_mapPortStrAndAddrList.value(arg1));
}

void CalibrationDisplay::mousePressEvent4Plot(QMouseEvent *e)
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
            m_pressGraph = NULL;
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
            m_pressGraph = graph;
        }
        else
        {
            if(m_tracer->visible())
            {
                m_tracer->setVisible(false);
                m_pressPos.setX(0);
                m_pressPos.setY(0);
                m_pressGraph = graph;
                ui->customplot->replot();
            }
            if(m_textTip->visible())
            {
                m_textTip->setVisible(false);
                m_pressPos.setX(0);
                m_pressPos.setY(0);
                m_pressGraph = graph;
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
                m_pressGraph = NULL;
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
                m_pressGraph = NULL;
                ui->customplot->replot();
            }
        }
    }

    if(m_pressPos.x()!=0 && m_pressPos.y()!=0)
    {
        m_textTip->setText("温度: "+ QString::number(m_pressPos.x(),'f',3) + "℃"\
                           +"\n压强差:"+": "+QString::number(m_pressPos.y(),'f',6)+"PA");
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

void CalibrationDisplay::selectionChangedByUser()
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

QVector<double> CalibrationDisplay::_divideTemp(double startTemp, double endTemp)
{
    QVector<double> value;
    double tmp_value = startTemp;
    while (startTemp < endTemp)
    {
        value.append(tmp_value);
        tmp_value += 0.2;
        if(tmp_value>endTemp)
            break;
    }

    return value;
}

void CalibrationDisplay::on_pbt_exportImage_clicked()
{
    QString filename =QFileDialog::getSaveFileName(this, tr("保存图片文件"),QDir::currentPath(),tr("Bmp files (*.bmp);;Pdf files(*.pdf);;Png files(*.png)"));
    if(filename.split(".").count()==1)
        filename = filename + ".png";
    if(!filename.isEmpty())
    {
        if(filename.contains(".png"))
            ui->customplot->savePng(filename,ui->customplot->width()*1.4,ui->customplot->height()*1.2);
        if(filename.contains(".bmp"))
            ui->customplot->saveBmp(filename,ui->customplot->width()*1.4,ui->customplot->height()*1.2);
        if(filename.contains(".pdf"))
            ui->customplot->savePdf(filename,ui->customplot->width()*1.4,ui->customplot->height()*1.2);
    }
}
