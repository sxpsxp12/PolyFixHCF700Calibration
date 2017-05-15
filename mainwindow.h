#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "sensorglobal.h"
#include "nodeparam.h"

#include <QSerialPort>
#include <QMap>
#include <QTimer>
#include <QCloseEvent>
#include <QList>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void closeEvent(QCloseEvent *e);
    void on_pbt_startCalibrationSample_clicked();

    void slot_nodeDropCheck();

    void on_pbt_connectScan_clicked();

    void slot_serialDataComming();

    void on_pbt_next_clicked();

    void slot_timer_out();

    void slot_op_timer_out();
    void on_pbt_calibration_clicked();

    void on_pbt_stopCalibrationSample_clicked();

    void on_pbt_DownCalibrationSample_clicked();

    void on_pbt_returnPageNodeList_clicked();

    void on_pbt_startSample_clicked();

    void on_pbt_polyfit_clicked();

    void on_pbt_exportCalibrationData_clicked();

    void on_pbt_downloadToNode_clicked();

    void on_pbt_calibrationDisplay_clicked();

private:
    Ui::MainWindow *ui;

    QMap<QSerialPort *,nodeParamList> m_MapPortAndNodeParam;
    QList<quint16> m_nodeAddrList;

    nodePayload m_currentPayload;
    nodeCalibrationStatus m_currentCalibrationStatus;
    bool isUp;

    QTimer m_checkPortDrop; //校验串口掉线定时器
    QTimer m_timerout;  //倒计时定时器
    int m_total_timerout;
    QTimer m_opTimer;   //数据包超时定时器

    int m_current_port;
    int m_current_node;

    void _initWindow();
    void _displayOutInfo(QString info);
    bool _scanNodeInfo(QSerialPort *serial, nodeParamList &param_list, QSerialPortInfo info);
    void _addNodeParamToView(const NodeParam &param);
    void _requestNodeData();
    double _getReferencePresure(nodePayload payload, quint16 range);
    void _displayCurrentPayloadSampleData();

    //多项式拟合数据
    void _displayPolyfitResult();
    bool _polyFitData(NodeParam &param);
    std::vector<double> _polyFitCurrentPayloadData(QList<node_Data> &up_list, QList<node_Data> &down_list);
    std::vector<double> divideTemp(double startTemp, double endTemp);
    /****测试用*****/
    void _initDatas(nodePayload payload,bool isUp,QString filename);
};

#endif // MAINWINDOW_H
