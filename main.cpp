#include "mainwindow.h"
#include "src/hj_file_write.h"
#include "utils/utils.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QTranslator>

QString log_file_name;

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 加锁
    static QMutex mutex;
    mutex.lock();
    QByteArray localMsg = msg.toLocal8Bit();
    QString text;
    switch(type) {
    case QtDebugMsg:
        text = QString("DDD:");
        break;
    case QtInfoMsg:
        text = QString("III:");
        break;
    case QtWarningMsg:
        text = QString("WWW:");
        break;
    case QtCriticalMsg:
        text = QString("EEE:");
        break;
    case QtFatalMsg:
        text = QString("FFF:");
        break;
    default:
        text = QString("DDD:");
    }

    // 设置输出信息格式
    QString context_info = QString("F2:(%1) L:(%2)").arg(QString(context.file)).arg(context.line); // F文件L行数
    QString strDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString strMessage = QString("%1 %2  %3").arg(text).arg(strDateTime).arg(msg);
    // 输出信息至文件中（读写、追加形式）
    QFile file(log_file_name);
    file.open(QIODevice::ReadWrite | QIODevice::Append);
    QTextStream stream(&file);
    stream << strMessage << "\r\n";
    file.flush();
    file.close();
    // 解锁
    mutex.unlock();
}

static void init_log(void)
{
    //注册MessageHandler
    qInstallMessageHandler(outputMessage);
    // log_file_name = CONFIG_PATH+QString(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss").append("-log.txt"));
    log_file_name = CONFIG_PATH + QString("JLink_Flash_GUI-log.txt");
    if (get_file_size(log_file_name.toStdString()) > 1048576)
    {
        remove(log_file_name.toStdString().c_str());
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    init_log();

#if 0
    QTranslator translator;
    translator.load("JLink_Flash_GUI_en_US.qm");
    a.installTranslator(&translator);
#endif

    MainWindow w;
    w.show();
	
    return a.exec();
}
