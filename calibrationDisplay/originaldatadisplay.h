#ifndef ORIGINALDATADISPLAY_H
#define ORIGINALDATADISPLAY_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "nodeparam.h"
#include "sensorglobal.h"
#include "components/qcustomplot.h"
#include <QMouseEvent>
#include "components/ccalendarwidget.h"
#include "components/cdatetimebutton.h"
#include <QPair>
#include "dboperate.h"
#include "customframe/headerview.h"
#include "customframe/tableviewmodel.h"
#include "customframe/itemdelegate.h"

namespace Ui {
class OriginalDataDisplay;
}

class OriginalDataDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit OriginalDataDisplay(DBOperate *db_operate,QWidget *parent = 0);
    ~OriginalDataDisplay();

    void initOriginalDataDisplay(QMap<QSerialPort *,nodeShareedPtrParamList> *mapSerialPtrAndNodeList);
private slots:

    void on_cbx_ports_currentIndexChanged(const QString &arg1);

    virtual void mousePressEvent4Plot(QMouseEvent *e);//鼠标点击事件，通常会在靠近采样点的时候强调采样点并显示采样点的数值
    virtual void selectionChangedByUser();//点击图例事件，通常会切换曲线的显示和隐藏状态
    void slot_dateTimeButtonClicked();
    void on_cbx_addrs_currentIndexChanged(const QString &arg1);

    void on_pbt_queryDatas_clicked();

    void slot_deleteData(const QModelIndex index);
    void slot_clearGraph(bool is_checked);
private:
    Ui::OriginalDataDisplay *ui;
    DBOperate *m_db_operate;
    QMap<QSerialPort *,nodeShareedPtrParamList> *m_mapSerialPtrAndNodeList;
    QMap<QString,QPair<QSerialPort *,QList<QString>>> m_mapPortStrAndAddrList;
    CCalendarWidget *m_calendarWidget;
    QAction *m_changeViewAction;
    QAction *m_changeDragAxis;
    QAction *m_clearGraph;

    QCPItemText *m_textTip;
    QCPItemTracer *m_tracer;//用于强调鼠标点击时间点击到的采样点
    QPointF m_pressPos;//跟踪鼠标点击事件点击位置
    QCPGraph *m_pressGraph;//跟踪鼠标点击时间点击的曲线
    QVector<double> m_textTipMargin;

    HeaderView *m_headerview;
    TableViewModel *m_tableview_model;
    ItemDelegate *m_item_delegate;

    void _initCustomPlot();
    void _initTableView();
};

#endif // ORIGINALDATADISPLAY_H
