#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jlink_settings.h"
#include "src/hj_file_write.h"
#include "utils/utils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <fstream>
#include <iostream>
#include <QProcess>
#include <QFileDialog>
#include <QDropEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QMessageBox>

#define JLINK_LOG_FILE              CONFIG_PATH "jlink_log.txt"
#define JLINK_DEVICE_INFO_FILE      CONFIG_PATH "jlink_device_info.json"

global_config_t globalConf;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ui 初始化
    ui->pushButton_dev_flush->setToolTip(tr("点击刷新开始识别设备"));

    // 加载Jlink配置
    get_global_config();
    set_default_config_display();

    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionJLink_settings_triggered()
{
    settings_window = new JLink_settings;

    settings_window->show();
}

void MainWindow::get_current_jlink_config()
{
    jlinkArgs.usb_sn =  ui->comboBox_sn->currentText().toStdString();

    if (ui->comboBox_2_connect_2->currentText() == "SWD")
    {
        qDebug() << "Select connect type is SWD";
        jlinkArgs.si = SWD;
    }

    jlinkArgs.speed = ui->comboBox_3_speed_2->currentText().toLong();
    jlinkArgs.dev_type = ui->comboBox_4_dev_type_2->currentText().toStdString();
    jlinkArgs.load_file = ui->lineEdit_fw_path_2->displayText().toStdString();
}

void MainWindow::set_default_config_display()
{
    QFile file(JLINK_DEVICE_INFO_FILE);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString value = file.readAll();
    file.close();

    QJsonObject root;
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);

    if (!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qWarning() << tr("解析配置错误！");
    }
    else
    {
        root = document.object();
    }

    // 读取配置记录的第一个，作为启动默认选项
    if (root.contains("current_dev_sn")) {
        QJsonValue value = root.value("current_dev_sn");
        if (value.isString()) {
            set_default_config_display(value.toString());
        }
    }
    else {
        // 设置默认
        ui->comboBox_2_connect_2->setCurrentIndex(1);
        ui->comboBox_3_speed_2->setCurrentIndex(2);
        ui->comboBox_4_dev_type_2->setCurrentIndex(2);
    }
}

void MainWindow::set_default_config_display(QString usb_sn)
{
    if (usb_sn == "") {
        return ;
    }

    QFile file(JLINK_DEVICE_INFO_FILE);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString value = file.readAll();
    file.close();

    QJsonObject root;
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);

    if (!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qWarning() << tr("解析配置错误！");
    }
    else
    {
        root = document.object();
    }

    qDebug() << "dev sn count: " << ui->comboBox_sn->count();
    if (ui->comboBox_sn->count() == 0) {
        ui->comboBox_sn->addItem(usb_sn);
    }

    if (root.contains(usb_sn)) {
        QJsonValue device_jlink_config = root.value(usb_sn);
        QJsonObject p = device_jlink_config.toObject();
        QJsonValue item;

        if (p.contains("connect_type")) {
            item = p.value("connect_type");
            if (item.isString()) {
                qDebug() << "设置连接方式：" << item.toString();
                ui->comboBox_2_connect_2->setCurrentText(item.toString());
            }
        }

        if (p.contains("device_type")) {
            item = p.value("device_type");
            if (item.isString()) {
                ui->comboBox_4_dev_type_2->setCurrentText(item.toString());
            }
        }

        if (p.contains("speed")) {
            item = p.value("speed");
            if (item.isString()) {
                ui->comboBox_3_speed_2->setCurrentText(item.toString());
            }
        }

        if (p.contains("firmware")) {
            item = p.value("firmware");
            if (item.isString()) {
                ui->lineEdit_fw_path_2->setText(item.toString());
            }
        }
    }
}

void MainWindow::save_current_jlink_config(QString usb_sn)
{
    qInfo() << "保存当前设备的配置";

    // 设置默认
    QFile file(JLINK_DEVICE_INFO_FILE);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString value = file.readAll();
    file.close();

    QJsonObject root;
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);

    if (!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qWarning() << tr("解析配置错误！");
    }
    else
    {
        root = document.object();
    }

    QJsonObject jlink_config;
    jlink_config.insert("connect_type", ui->comboBox_2_connect_2->currentText());
    jlink_config.insert("speed", ui->comboBox_3_speed_2->currentText());
    jlink_config.insert("device_type", ui->comboBox_4_dev_type_2->currentText());
    jlink_config.insert("firmware", ui->lineEdit_fw_path_2->displayText());

    root.insert("current_dev_sn", usb_sn);
    root.insert(usb_sn, QJsonValue(jlink_config));

    if (!(file.open(QIODevice::WriteOnly))) {
        qCritical() << "打开" << JLINK_DEVICE_INFO_FILE << "文件失败";
    }

    QJsonDocument jsonDoc;
    jsonDoc.setObject(root);

    file.write(jsonDoc.toJson());
    file.close();
    qInfo() << "写" << JLINK_DEVICE_INFO_FILE << "文件成功";
}

void MainWindow::get_global_config()
{
    QFile file(GLOBAL_CONFIG_FILE);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString value = file.readAll();
    file.close();

    global_config_t *global_config = &globalConf;

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if (!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qCritical() << "解析软件配置错误！";
        QMessageBox::StandardButton btn = (QMessageBox::StandardButton )QMessageBox::critical(this, tr("警告！"), tr("读取配置文件错误，请检查"), tr("确认"), tr("退出"));
        qDebug() << "当前选择的结果：" << btn;
        if (btn == QMessageBox::StandardButton(1)) {
            qCritical() << "缺少配置文件，程序关闭";
            exit(0);
        }
        else if (btn == QMessageBox::StandardButton(0)){
            qDebug() << "按下确认按键";
        }
        return ;
    }

    QJsonObject root = document.object();

    // jlink 路径
    QJsonValue jlink_cmd_path_value = root.value("jlink_cmd_path");
    if (jlink_cmd_path_value.isString())
    {
        QString jlink_cmd_path = jlink_cmd_path_value.toString();
        global_config->jlink_cmd_path = jlink_cmd_path.toStdString();
        qDebug() << "jlink_cmd_path value: " << jlink_cmd_path;
        //qDebug() << QString::fromStdString(global_config.jlink_cmd_path);
    }
    else {
        qCritical() << "读取Jlink路径失败！";
        QMessageBox::critical(this, tr("警告"), tr("读取JLink.exe路径失败，请检查"), tr("确认"), tr("退出"));
        return ;
    }

    // 支持的设备连接方式
    QJsonValue connect_type = root.value("connect_type_list");
    if (connect_type.isArray())
    {
        QJsonArray ary = connect_type.toArray();
        int size = ary.size();
        QStringList textList;
        textList.clear();
        for (int i = 0; i < size; i++)
        {
            QJsonValue value = ary.at(i);
            if (value.isString())
            {
                textList << value.toString();
            }
        }

        ui->comboBox_2_connect_2->addItems(textList);
    }

    // 速率
    QJsonValue speed = root.value("speed_list");
    if (speed.isArray())
    {
        QJsonArray ary = speed.toArray();
        int size = ary.size();
        QStringList textList;
        textList.clear();
        for (int i = 0; i < size; i++)
        {
            QJsonValue value = ary.at(i);
            if (value.isDouble())
            {
                textList << QString::number(value.toDouble());
            }
        }

        ui->comboBox_3_speed_2->addItems(textList);
    }

    // 支持的设备类型
    QJsonValue device_type = root.value("device_type_list");
    if (device_type.isArray())
    {
        QJsonArray ary = device_type.toArray();
        int size = ary.size();
        QStringList textList;
        textList.clear();
        for (int i = 0; i < size; i++)
        {
            QJsonValue value = ary.at(i);
            if (value.isString())
            {
                textList << value.toString();
            }
        }

        ui->comboBox_4_dev_type_2->addItems(textList);
    }
}

void MainWindow::get_connect_sn()
{
    // 生成获取SN的命令文件
    fileArgs.generate_get_sn_config_file();

    // 执行获取动作
    QString cmd;
    cmd = "\"" + QString::fromStdString(globalConf.jlink_cmd_path) + "\"";
    cmd.append(" -CommandFile ");
    cmd.append(GET_SN_CMD_FILE);
    cmd.append(" -Log ");
    cmd.append(JLINK_LOG_FILE);
    qInfo() << cmd;
    // system(cmd.toStdString().c_str());

    // 使用QProcess来执行命令
    QProcess p(0);
    p.start(cmd);
    p.waitForStarted();
    p.waitForFinished();

    ui->textBrowser_log_2->append(QString::fromLocal8Bit(p.readAllStandardOutput()));
    qInfo() << QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString().c_str();

    long sn_ary[10];
    uint32_t sn_num = get_jlink_sn(JLINK_LOG_FILE, sn_ary, 10);
    ui->comboBox_sn->clear();
    for (int i = 0; i < (int)sn_num; i++)
    {
        ui->comboBox_sn->addItem(QString::number(sn_ary[i]));
    }
}

uint32_t MainWindow::get_jlink_sn(const char* file, long* sn_ary, uint32_t len)
{
    using namespace std;
    ifstream fin;
    long* sn = sn_ary;
    string *jlink_sn = new string[10];
    fin.open(file);

    char line[1024] = { 0 };
    char str[] = "]: USB, S/N: ";

    uint32_t count = 0;
    int i = 0;
    while (fin.good())
    {
        fin.getline(line, 1024);
        cout << line << endl;

        string ln = line;
        string::size_type pos;
        pos = ln.find(str);
        if (pos < 10000)
        {
            jlink_sn[i] = &line[pos + strlen(str)];
            i++;
            count++;
            if (count > len)
            {
                break;
            }
        }
    }
    for (int i = 0; i < (int)count; i++)
    {
        sscanf_s(jlink_sn[i].c_str(), "%ld", &sn[i]);
    }
    delete[] jlink_sn;
    fin.close();
    return count;
}

void MainWindow::on_pushButton_dev_flush_clicked()
{
    get_connect_sn();

    if (ui->comboBox_sn->currentText() != "")
    {
        save_current_jlink_config(ui->comboBox_sn->currentText());
    }
}

void MainWindow::on_pushButton_flush_clicked()
{
    get_current_jlink_config();
    fileArgs.generate_flush_config_file(&jlinkArgs);

    if (!is_file_exit(globalConf.jlink_cmd_path)) {
        qCritical() << QString::fromStdString(globalConf.jlink_cmd_path) << " 路径不存在";
        QMessageBox::information(this, tr("警告"), QString::fromStdString(globalConf.jlink_cmd_path) + tr(" 文件或路径不存在，请检查"), tr("确认"), tr("退出"));
        return ;
    }

    // 执行获取动作
    QString cmd;
    cmd = "\"" + QString::fromStdString(globalConf.jlink_cmd_path) + "\"";
    // cmd.append(fileArgs.flush_file.c_str());

    cmd = cmd + " " + QString::fromStdString(fileArgs.flush_file);

    qInfo() << "exec cmd: " << cmd;

    // 使用QProcess来执行命令
    QProcess p(0);
    p.start(cmd);
    p.waitForStarted();
    p.waitForFinished();

    ui->textBrowser_log_2->append(QString::fromLocal8Bit(p.readAllStandardOutput()));
    qInfo() << QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString().c_str();

    save_current_jlink_config(ui->comboBox_sn->currentText());
}

void MainWindow::on_pushButton_2_choose_fw_2_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "选择HEX固件", ui->lineEdit_fw_path_2->displayText());
    jlinkArgs.load_file = filename.toStdString();

    if (filename != "")
    {
        ui->lineEdit_fw_path_2->setText(filename);
    }
    qInfo() << "firmware selected: " << filename;
}

void MainWindow::on_pushButton_reboot_clicked()
{
    if (!is_file_exit(globalConf.jlink_cmd_path)) {
        qCritical() << QString::fromStdString(globalConf.jlink_cmd_path) << " 路径不存在";
        QMessageBox::information(this, tr("警告"), QString::fromStdString(globalConf.jlink_cmd_path) + tr(" 文件或路径不存在，请检查"), tr("确认"), tr("退出"));
        return ;
    }

    get_current_jlink_config();
    fileArgs.generate_reboot_config_file(&jlinkArgs);

    // 执行获取动作
    QString cmd;
    cmd = "\"" + QString::fromStdString(globalConf.jlink_cmd_path) + "\"";
    // cmd.append(fileArgs.flush_file.c_str());

    cmd = cmd + " " + QString::fromStdString(fileArgs.reboot_file);

    qInfo() << "exec cmd: " << cmd;

    // 使用QProcess来执行命令
    QProcess p(0);
    p.start(cmd);
    p.waitForStarted();
    p.waitForFinished();

    ui->textBrowser_log_2->append(QString::fromLocal8Bit(p.readAllStandardOutput()));
    qInfo() << QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString().c_str();
}

void MainWindow::on_pushButton_erase_clicked()
{
    if (!is_file_exit(globalConf.jlink_cmd_path)) {
        qCritical() << QString::fromStdString(globalConf.jlink_cmd_path) << " 路径不存在";
        QMessageBox::information(this, tr("警告"), QString::fromStdString(globalConf.jlink_cmd_path) + tr(" 文件或路径不存在，请检查"), tr("确认"), tr("退出"));
        return ;
    }

    get_current_jlink_config();
    fileArgs.generate_erase_config_file(&jlinkArgs);

    // 执行获取动作
    QString cmd;
    cmd = "\"" + QString::fromStdString(globalConf.jlink_cmd_path) + "\"";
    // cmd.append(fileArgs.flush_file.c_str());

    cmd = cmd + " " + QString::fromStdString(fileArgs.erase_file);

    qInfo() << "exec cmd: " << cmd;

    // 使用QProcess来执行命令
    QProcess p(0);
    p.start(cmd);
    p.waitForStarted();
    p.waitForFinished();

    ui->textBrowser_log_2->append(QString::fromLocal8Bit(p.readAllStandardOutput()));
    qInfo() << QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString().c_str();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (true)
    {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    //获取文件路径 (QString)
    QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty()) return;
    QString qStr = urls.first().toLocalFile();

    //转为char*
    QByteArray qByteArrary = qStr.toLatin1();
    char* filePath = qByteArrary.data();

    qInfo() << "select firmware file: " << filePath;

    ui->lineEdit_fw_path_2->setText(filePath);
}

void MainWindow::on_comboBox_sn_currentTextChanged(const QString &arg1)
{
    set_default_config_display(arg1);
}
