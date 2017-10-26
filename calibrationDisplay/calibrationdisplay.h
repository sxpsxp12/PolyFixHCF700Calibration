#ifndef CALIBRATIONDISPLAY_H
#define CALIBRATIONDISPLAY_H

#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "nodeparam.h"
#include "sensorglobal.h"
#include "components/qcustomplot.h"
#include <QMouseEvent>

namespace Ui {
class CalibrationDisplay;
}

class CalibrationDisplay : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDisplay(QMap<QSerialPort *,nodeShareedPtrParamList> *mapSerialPtrAndNodeList,QWidget *parent = 0);
    ~CalibrationDisplay();
private slots:
    void on_pbt_querycalibration_clicked();

    void on_cbx_ports_currentIndexChanged(const QString &arg1);

    virtual void mousePressEvent4Plot(QMouseEvent *e);//鼠标点击事件，通常会在靠近采样点的时候强调采样点并显示采样点的数值
    virtual void selectionChangedByUser();//点击图例事件，通常会切换曲线的显示和隐藏状态
    void on_pbt_exportImage_clicked();

private:
    Ui::CalibrationDisplay *ui;
    QMap<QSerialPort *,nodeShareedPtrParamList> *m_mapSerialPtrAndNodeList;
    QMap<QString,QList<QString>> m_mapPortStrAndAddrList;
    QCPItemText *m_textTip;
    QCPItemTracer *m_tracer;//用于强调鼠标点击时间点击到的采样点
    QPointF m_pressPos;//跟踪鼠标点击事件点击位置
    QCPGraph *m_pressGraph;//跟踪鼠标点击时间点击的曲线
    QVector<double> m_textTipMargin;

    void _initCustomPlot();
    QVector<double> _divideTemp(double startTemp, double endTemp);
};

#endif // CALIBRATIONDISPLAY_H
