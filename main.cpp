#include "mainwindow.h"
#include <QApplication>
#include "customframe/nofocusstyle.h"

/*!
    日志输出功能
*/
void crashingMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
        mutex.lock();

        QString text;
        switch(type)
        {
        case QtDebugMsg:
            text = QString("Debug:");
            break;

        case QtWarningMsg:
            text = QString("Warning:");
            break;

        case QtCriticalMsg:
            text = QString("Critical:");
            break;

        case QtFatalMsg:
            text = QString("Fatal:");
        }

        QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
        QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
        QString current_date = QString("(%1)").arg(current_date_time);
        QString message = QString("%1: %2 %3-> %4").arg(current_date).arg(text).arg(context_info).arg(msg);

        QString fileDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/SensorCamellia/Log";
        QDir dir;
        if(!dir.exists(fileDir))
        {
            dir.mkpath(fileDir);
        }

        QFile file(fileDir + "/log.txt");

        file.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream text_stream(&file);
        text_stream << message << "\r\n";
        file.flush();
        file.close();

        mutex.unlock();
}

int main(int argc, char *argv[])
{
    //日志输出
    qInstallMessageHandler(crashingMessageHandler);

    QApplication a(argc, argv);

    a.setStyle(new NoFocusStyle);
    MainWindow w;
    w.show();
    return a.exec();
}
