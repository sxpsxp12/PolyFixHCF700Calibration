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
#include <QSound>
#include "customframe/headerview.h"
#include "customframe/tableviewmodel.h"
#include "customframe/itemdelegate.h"
#include "customframe/standarditemmodel.h"
#include "dboperate.h"
#include "components/ccalendarwidget.h"
#include "calibrationDisplay/originaldatadisplay.h"
#include "customframe/standarditem.h"

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

    void slot_op_timer_out();

    void on_pbt_stopCalibrationSample_clicked();

    void on_pbt_calibration_clicked();

    void on_pbt_returnPageNodeList_clicked();

    void on_pbt_exportCalibrationData_clicked();

    void on_pbt_downloadToNode_clicked();

    void on_pbt_calibrationDisplay_clicked();

    void on_pbt_polyfit_clicked();

    void on_pbt_undoSample_clicked();

    void slot_requestNodeTempTimerOut();
    void on_groupBoxTip_toggled(bool arg1);

    void on_pbt_clearFactor_clicked();

    void slot_changeNodeSelectedStatus(QSerialPortInfo port_info,quint16 addr,bool status);

    void on_pbt_sample_data_clicked();

    void slot_dateTimeButtonClicked();
    void on_pbt_datasDisplay_clicked();

    void on_treeView_Polyfit_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    QSound m_sound;
    DBOperate *m_db_operate;
    QDialog *m_loadDialog;
    CCalendarWidget *m_calendarWidget;
    OriginalDataDisplay *m_originalDataDisplay;

    QMap<QSerialPort *,nodeShareedPtrParamList> m_MapPortAndNodeParam;
    QMap<QSerialPort *,nodeShareedPtrParamList> m_MapPortAndSelectedNodeParam;

    HeaderView *m_headerview_nodelist;
    TableViewModel *m_tableview_model_nodelist;
    ItemDelegate *m_checkbox_delegate;

    HeaderView *m_polyfit_header;
    StandardItemModel *m_polyfit_treemodel;

    nodePayload m_currentPayload;
    nodePayload m_lastPayload;
    double m_lastNodeTemp;

    nodeCalibrationStatus m_currentCalibrationStatus;

    QTimer m_checkPortDrop; //校验串口掉线定时器
    QTimer m_requestNodeTemp;

    QTimer m_opTimer;   //数据包超时定时器

    int m_current_port;
    int m_current_node;

    void _initWindow();
    void _initLoadMovie();
    void _displayOutInfo(QString info);
    bool _scanNodeInfo(QSerialPort *serial, nodeShareedPtrParamList &param_list, QSerialPortInfo info);
    void _requestNodeData();
    double _getReferencePresure(nodePayload payload, quint16 range);
    void _displayCurrentPayloadSampleData();

    //多项式拟合数据
    void _displayPolyfitResult();
    bool _polyFitData(QSharedPointer<NodeParam> param);
    std::vector<double> _polyFitCurrentPayloadData(QList<node_Data> &up_list, QList<node_Data> &down_list);
    void statisticDownloadResult(QString downloadInfo,QString successAddrInfo);
};

#endif // MAINWINDOW_H
