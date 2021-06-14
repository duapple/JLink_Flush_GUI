#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jlink_settings.h"
#include "src/hj_file_write.h"
#include "utils/utils.h"
#include <stdio.h>
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
#include <QFileInfo>
#include <QFile>
#include <QDateTime>

#define JLINK_LOG_FILE              CONFIG_PATH "jlink_log.txt"
#define JLINK_DEVICE_INFO_FILE      CONFIG_PATH "jlink_device_info.json"

global_config_t globalConf;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mythread(new Mythread)
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
    delete mythread;
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
    jlinkArgs.load_file2 = ui->lineEdit_fw_path_3->displayText().toStdString();
    jlinkArgs.load_file3 = ui->lineEdit_fw_path_4->displayText().toStdString();
    jlinkArgs.file_start_addr = ui->lineEdit_mem_start_addr_1->displayText().toStdString();
    jlinkArgs.file2_start_addr = ui->lineEdit_mem_start_addr_2->displayText().toStdString();
    jlinkArgs.file3_start_addr = ui->lineEdit_mem_start_addr_3->displayText().toStdString();
    jlinkArgs.erase_chip = ui->checkBox_erase_sector->isChecked();
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
            qDebug() << "dev sn count: " << ui->comboBox_sn->count();
            if (ui->comboBox_sn->count() == 0) {
                ui->comboBox_sn->addItem(value.toString());
            }
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
                fw_file1_check(ui->lineEdit_fw_path_2->displayText());
                if (get_file_format(item.toString().toStdString()) == ".bin")
                {
                    qDebug() << "固件1 为BIN文件，显示其保存的Start Address";
                    if (p.contains("fw_start_addr")) {
                        item = p.value("fw_start_addr");
                        if (item.isString()) {
                            ui->lineEdit_mem_start_addr_1->setText(item.toString());
                        }
                    }
                }
            }
        }
        if (p.contains("firmware2")) {
            item = p.value("firmware2");
            if (item.isString()) {
                ui->lineEdit_fw_path_3->setText(item.toString());
                fw_file2_check(ui->lineEdit_fw_path_3->displayText());
                if (get_file_format(item.toString().toStdString()) == ".bin")
                {
                    qDebug() << "固件2 为BIN文件，显示其保存的Start Address";
                    if (p.contains("fw2_start_addr")) {
                        item = p.value("fw2_start_addr");
                        if (item.isString()) {
                            ui->lineEdit_mem_start_addr_2->setText(item.toString());
                        }
                    }
                }
            }
        }
        if (p.contains("firmware3")) {
            item = p.value("firmware3");
            if (item.isString()) {
                ui->lineEdit_fw_path_4->setText(item.toString());
                fw_file3_check(ui->lineEdit_fw_path_4->displayText());
                if (get_file_format(item.toString().toStdString()) == ".bin")
                {
                    qDebug() << "固件3 为BIN文件，显示其保存的Start Address";
                    if (p.contains("fw3_start_addr")) {
                        item = p.value("fw3_start_addr");
                        if (item.isString()) {
                            ui->lineEdit_mem_start_addr_3->setText(item.toString());
                        }
                    }
                }
            }
        }
        if (p.contains("erase_chip")) {
            item = p.value("erase_chip");
            if (item.isBool()) {
                ui->checkBox_erase_sector->setChecked(item.toBool());
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
    jlink_config.insert("firmware2", ui->lineEdit_fw_path_3->displayText());
    jlink_config.insert("firmware3", ui->lineEdit_fw_path_4->displayText());
    jlink_config.insert("erase_chip", ui->checkBox_erase_sector->isChecked());
    jlink_config.insert("fw_start_addr", ui->lineEdit_mem_start_addr_1->displayText());
    jlink_config.insert("fw2_start_addr", ui->lineEdit_mem_start_addr_2->displayText());
    jlink_config.insert("fw3_start_addr", ui->lineEdit_mem_start_addr_3->displayText());

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

void MainWindow::clear_fw_file_hex(void)
{
    if (jlinkArgs.load_file != "")
    {
        string load_file1 = jlinkArgs.load_file;
        if (get_file_format(load_file1) == ".bin") {
            format_bin_to_hex(load_file1);
            remove(load_file1.c_str());
        }
    }
    if (jlinkArgs.load_file2 != "")
    {
        string load_file2 = jlinkArgs.load_file2;
        if (get_file_format(load_file2) == ".bin") {
            format_bin_to_hex(load_file2);
            remove(load_file2.c_str());
        }
    }
    if (jlinkArgs.load_file3 != "")
    {
        string load_file3 = jlinkArgs.load_file3;
        if (get_file_format(load_file3) == ".bin") {
            format_bin_to_hex(load_file3);
            remove(load_file3.c_str());
        }
    }
}

int MainWindow::bin_start_addr_check(void)
{
    if ( jlinkArgs.load_file != "")
    {
        string load_file1 = jlinkArgs.load_file;
        if (get_file_format(load_file1) == ".bin") {
            if (ui->lineEdit_mem_start_addr_1->displayText() == "") {
                QMessageBox::information(this, tr("警告"), tr("请输入固件1 BIN文件的起始地址"), tr("退出"));
                return -1;
            }
        }
    }
    if ( jlinkArgs.load_file2 != "")
    {
        qDebug() << "load_file2 存在";
        string load_file2 = jlinkArgs.load_file2;
        if (get_file_format(load_file2) == ".bin") {
            qDebug() << "load file2 格式为 .bin";
            if (ui->lineEdit_mem_start_addr_2->displayText() == "") {
                QMessageBox::information(this, tr("警告"), tr("请输入固件2 BIN文件的起始地址"), tr("退出"));
                return -1;
            }
        }
    }
    if ( jlinkArgs.load_file3 != "")
    {
        string load_file3 = jlinkArgs.load_file3;
        if (get_file_format(load_file3) == ".bin") {
            if (ui->lineEdit_mem_start_addr_3->displayText() == "") {
                QMessageBox::information(this, tr("警告"), tr("请输入固件3 BIN文件的起始地址"), tr("退出"));
                return -1;
            }
        }
    }

    return 0;
}

int MainWindow::fw_file_check(void)
{
    if (jlinkArgs.load_file != "" && !is_file_exit(jlinkArgs.load_file)) {
        QMessageBox::information(this, tr("警告"), tr("固件1 不存在"), tr("确认"), tr("退出"));
        return -1;
    }

    if (jlinkArgs.load_file2 != "" && !is_file_exit(jlinkArgs.load_file2)) {
        QMessageBox::information(this, tr("警告"), tr("固件2 不存在"), tr("确认"), tr("退出"));
        return -2;
    }

    if (jlinkArgs.load_file3 != "" && !is_file_exit(jlinkArgs.load_file3)) {
        QMessageBox::information(this, tr("警告"), tr("固件3 不存在"), tr("确认"), tr("退出"));
        return -3;
    }

    return 0;
}

void MainWindow::on_pushButton_flush_clicked()
{
    if (ui->comboBox_sn->currentText() == "") {
        qInfo() << "当前无Jlink设备连接";
        QMessageBox::information(this, tr("警告"), tr("当前没有Jlink设备，请刷新"), tr("退出"));
        return ;
    }

    get_current_jlink_config();
    if (bin_start_addr_check() || fw_file_check()) {
        return ;
    }

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

    string out = QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString();
    ui->textBrowser_log_2->append(QString::fromStdString(out));

    if (out.find("Connecting to J-Link via USB...FAILED") < out.size())
    {
        qCritical() << "Connecting to J-Link via USB...FAILED";
        QMessageBox::information(this, tr("警告"), tr("当前Jlink没有连接上终端设备，请连接终端设备后再尝试"), tr("退出"));
    } else {
        QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "<font color=\"#FF0000\"> 烧录 success !!! </font>";
        ui->textBrowser_log_2->append(str);
        ui->textBrowser_log_2->append("\r\n\r\n");
    }

    save_current_jlink_config(ui->comboBox_sn->currentText());
}

void MainWindow::on_pushButton_reboot_clicked()
{
    if (ui->comboBox_sn->currentText() == "") {
        qInfo() << "当前无Jlink设备连接";
        QMessageBox::information(this, tr("警告"), tr("当前没有Jlink设备，请刷新"), tr("退出"));
        return ;
    }

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

    string out = QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString();
    ui->textBrowser_log_2->append(QString::fromStdString(out));

    if (out.find("Connecting to J-Link via USB...FAILED") < out.size())
    {
        qCritical() << "Connecting to J-Link via USB...FAILED";
        QMessageBox::information(this, tr("警告"), tr("当前Jlink没有连接上终端设备，请连接终端设备后再尝试"), tr("退出"));
    } else {
        QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "<font color=\"#FF0000\"> 重启芯片 success !!! </font>";
        ui->textBrowser_log_2->append(str);
        ui->textBrowser_log_2->append("\r\n\r\n");
    }
}

void MainWindow::on_pushButton_erase_clicked()
{
    if (ui->comboBox_sn->currentText() == "") {
        qInfo() << "当前无Jlink设备连接";
        QMessageBox::information(this, tr("警告"), tr("当前没有Jlink设备，请刷新"), tr("退出"));
        return ;
    }

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

    string out = QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString();
    ui->textBrowser_log_2->append(QString::fromStdString(out));

    if (out.find("Connecting to J-Link via USB...FAILED") < out.size())
    {
        qCritical() << "Connecting to J-Link via USB...FAILED";
        QMessageBox::information(this, tr("警告"), tr("当前Jlink没有连接上终端设备，请连接终端设备后再尝试"), tr("退出"));
    } else {
        QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "<font color=\"#FF0000\"> 擦除芯片 success !!! </font>";
        ui->textBrowser_log_2->append(str);
        ui->textBrowser_log_2->append("\r\n\r\n");
    }
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

    if (ui->lineEdit_fw_path_2->underMouse())
    {
        fw_file1_check(QString::fromStdString(filePath));
        ui->lineEdit_fw_path_2->setText(filePath);
        save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
    }
    else if (ui->lineEdit_fw_path_3->underMouse())
    {
        fw_file2_check(QString::fromStdString(filePath));
        ui->lineEdit_fw_path_3->setText(filePath);
        save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
    }
    else if (ui->lineEdit_fw_path_4->underMouse())
    {
        fw_file3_check(QString::fromStdString(filePath));
        ui->lineEdit_fw_path_4->setText(filePath);
        save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
    }
    else {
        qInfo() << "鼠标没有指向 lineEdit";
    }
}

void MainWindow::on_comboBox_sn_currentTextChanged(const QString &arg1)
{
    set_default_config_display(arg1);
}

void MainWindow::on_pushButton_rtt_viewer_clicked()
{
    std::string rtt_viewer_path = get_file_path(globalConf.jlink_cmd_path) + "./JLinkRTTViewer.exe";

    if (!is_file_exit(rtt_viewer_path)) {
        qCritical() << QString::fromStdString(rtt_viewer_path) << " 路径不存在";
        QMessageBox::information(this, tr("警告"), QString::fromStdString(rtt_viewer_path) + tr(" 文件或路径不存在，请检查"), tr("确认"), tr("退出"));
        return ;
    }

    // 执行获取动作
    QString cmd;
    cmd = "\"" + QString::fromStdString(rtt_viewer_path) + "\"";

    qInfo() << "exec cmd: " << cmd;

    mythread->rtt_viewer_exec_cmd = rtt_viewer_path;
    mythread->start();
}

void Mythread::run()
{
    QObject *parent=NULL;
    string cmd = "\"" + rtt_viewer_exec_cmd + "\"";
    QString program = QString::fromStdString(cmd);
    qDebug() << "开启JLink RTT Viewer：" << program;

    QProcess *myProcess = new QProcess(parent);
    myProcess->start(program);
    myProcess->waitForStarted();
    myProcess->waitForFinished();

    string out = QString::fromLocal8Bit(myProcess->readAllStandardOutput()).toStdString();
    qDebug() << "out: \r\n" << out.c_str();
    qInfo() << "RTT Viewer process destroy!!!";

    delete parent;
}

Mythread::Mythread(QObject * parent)
{
}

void MainWindow::on_pushButton_2_choose_fw_2_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "选择HEX固件1", ui->lineEdit_fw_path_2->displayText());
    jlinkArgs.load_file = filename.toStdString();

    if (filename != "")
    {
        fw_file1_check(filename);
    }
    qInfo() << "firmware1 selected: " << filename;
}

void MainWindow::on_pushButton_2_choose_fw_3_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "选择HEX固件2", ui->lineEdit_fw_path_3->displayText());
    jlinkArgs.load_file2 = filename.toStdString();

    if (filename != "") {
        fw_file2_check(filename);
    }
    qInfo() << "firmware2 selected: " << filename;
}

void MainWindow::on_pushButton_2_choose_fw_4_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "选择HEX固件3", ui->lineEdit_fw_path_4->displayText());
    jlinkArgs.load_file3 = filename.toStdString();

    if (filename != "") {
        fw_file3_check(filename);
    }

    qInfo() << "firmware3 selected: " << filename;
}

void MainWindow::fw_file1_check(QString filename)
{
    if (filename == "") {
        return ;
    }

    string format = get_file_format(filename.toStdString());
    if (format == ".bin") {
        qDebug() << "已选择Bin文件";
        ui->textBrowser_log_2->append(">> 请输入Bin文件的起始地址");
        ui->lineEdit_mem_start_addr_1->clear();
        ui->lineEdit_mem_start_addr_1->setEnabled(true);
    } else if (format == ".hex") {
        qDebug() << "已选择HEX文件";

        int ret = get_bin_file_start_addr(filename.toStdString(), jlinkArgs.file_start_addr, jlinkArgs.file_end_addr);
        if (ret) {
            QMessageBox::information(this, tr("警告"), tr("HEX文件错误，请检查"), tr("确认"), tr("退出"));
            ui->textBrowser_log_2->append("固件1 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件");
            ui->lineEdit_fw_path_2->clear();
            ui->lineEdit_mem_start_addr_1->clear();
            return ;
        }
        qDebug() << "jlinkArgs.file_start_addr: " << jlinkArgs.file_start_addr.c_str()
                 << "jlinkArgs.file_end_addr: " << jlinkArgs.file_end_addr.c_str();

        ui->lineEdit_mem_start_addr_1->setText(QString::fromStdString(jlinkArgs.file_start_addr));
        ui->lineEdit_mem_start_addr_1->setEnabled(false);
    } else {
        qCritical() << "文件格式不正确，请检查";
        ui->textBrowser_log_2->append("固件1 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件");
        ui->lineEdit_fw_path_2->clear();
        ui->lineEdit_mem_start_addr_1->clear();
        return ;
    }

    ui->lineEdit_fw_path_2->setText(filename);
    save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
}

void MainWindow::fw_file2_check(QString filename)
{
    if (filename == "") {
        return ;
    }

    string format = get_file_format(filename.toStdString());
    if (format == ".bin") {
        qDebug() << "已选择Bin文件";
        ui->textBrowser_log_2->append(">> 请输入Bin文件的起始地址");
        ui->lineEdit_mem_start_addr_2->clear();
        ui->lineEdit_mem_start_addr_2->setEnabled(true);
    } else if (format == ".hex") {
        qDebug() << "已选择HEX文件";

        int ret = get_bin_file_start_addr(filename.toStdString(), jlinkArgs.file2_start_addr, jlinkArgs.file2_end_addr);
        if (ret) {
            QMessageBox::information(this, tr("警告"), tr("HEX文件错误，请检查"), tr("确认"), tr("退出"));
            ui->textBrowser_log_2->append("固件2 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件");
            ui->lineEdit_fw_path_3->clear();
            ui->lineEdit_mem_start_addr_2->clear();
            return ;
        }
        qDebug() << "jlinkArgs.file2_start_addr: " << jlinkArgs.file2_start_addr.c_str()
                 << "jlinkArgs.file2_end_addr: " << jlinkArgs.file2_end_addr.c_str();

        ui->lineEdit_mem_start_addr_2->setText(QString::fromStdString(jlinkArgs.file2_start_addr));
        ui->lineEdit_mem_start_addr_2->setEnabled(false);
    } else {
        qCritical() << "文件格式不正确，请检查";
        ui->textBrowser_log_2->append("固件2 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件");
        ui->lineEdit_fw_path_3->clear();
        ui->lineEdit_mem_start_addr_2->clear();
        return ;
    }

    ui->lineEdit_fw_path_3->setText(filename);
    save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
}

void MainWindow::fw_file3_check(QString filename)
{
    if (filename == "") {
        return ;
    }

    string format = get_file_format(filename.toStdString());
    printf("test1");
    if (format == ".bin") {
        qDebug() << "已选择Bin文件";
        ui->textBrowser_log_2->append(">> 请输入Bin文件的起始地址");
        ui->lineEdit_mem_start_addr_3->clear();
        ui->lineEdit_mem_start_addr_3->setEnabled(true);
    } else if (format == ".hex") {
        qDebug() << "已选择HEX文件";
        int ret = get_bin_file_start_addr(filename.toStdString(), jlinkArgs.file3_start_addr, jlinkArgs.file3_end_addr);
        if (ret) {
            QMessageBox::information(this, tr("警告"), tr("HEX文件错误，请检查"), tr("确认"), tr("退出"));
            ui->textBrowser_log_2->append("固件3 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件");
            ui->lineEdit_fw_path_4->clear();
            ui->lineEdit_mem_start_addr_3->clear();
            return ;
        }
        qDebug() << "jlinkArgs.file3_start_addr: " << jlinkArgs.file3_start_addr.c_str()
                 << "jlinkArgs.file3_end_addr: " << jlinkArgs.file3_end_addr.c_str();

        ui->lineEdit_mem_start_addr_3->setText(QString::fromStdString(jlinkArgs.file3_start_addr));
        ui->lineEdit_mem_start_addr_3->setEnabled(false);
    } else {
        printf("test4");
        qCritical() << "文件格式不正确，请检查";
        ui->textBrowser_log_2->append("固件3 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件");
        ui->lineEdit_fw_path_4->clear();
        ui->lineEdit_mem_start_addr_3->clear();
        return ;
    }

    ui->lineEdit_fw_path_4->setText(filename);
    save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
}

void MainWindow::on_lineEdit_fw_path_2_editingFinished()
{
    fw_file1_check(ui->lineEdit_fw_path_2->displayText());
    qDebug() << "fw file1 changed";
}

void MainWindow::on_lineEdit_fw_path_3_editingFinished()
{
    QString filename = ui->lineEdit_fw_path_3->displayText();
    fw_file2_check(filename);
    qDebug() << "fw file2 changed";
}

void MainWindow::on_lineEdit_fw_path_4_editingFinished()
{
    fw_file3_check(ui->lineEdit_fw_path_4->displayText());
    qDebug() << "fw file3 changed";
}

void MainWindow::on_action_clear_log_triggered()
{
    ui->textBrowser_log_2->clear();
    qDebug() << "清除log";
}

void MainWindow::on_checkBox_erase_sector_toggled(bool checked)
{
    if (checked) {
        jlinkArgs.erase_chip = true;
        qInfo() << "选择擦除整个芯片";

//#define JLINK_VERSION_TXT   "./config/get_jlink_version.txt"

//        QFileInfo file(JLINK_VERSION_TXT);
//        if(file.exists()==false) {
//            qDebug() << JLINK_VERSION_TXT << "不存在";
//            QFile file_create(JLINK_VERSION_TXT);
//            file_create.open(QIODevice::ReadWrite | QIODevice::Text);
//            file_create.close();

//            using namespace std;
//            ofstream get_jlink_version_s;
//            get_jlink_version_s.open(JLINK_VERSION_TXT);

//            get_jlink_version_s << "?\n";
//            get_jlink_version_s.close();
//        }

//        QObject *parent=NULL;
//        QString program = QString::fromStdString(globalConf.jlink_cmd_path);
//        QStringList arguments;
//        arguments << "\"" JLINK_VERSION_TXT "\"";

//        QProcess *myProcess = new QProcess(parent);
//        myProcess->start(program, arguments);
//        myProcess->waitForStarted();
//        myProcess->waitForFinished();

//        string out = QString::fromLocal8Bit(myProcess->readAllStandardOutput()).toStdString();
//        qDebug() << out.c_str();

//#define DLL_VERSION_STR "DLL version V"

//        string dll_version = DLL_VERSION_STR;

//        long long unsigned pos = out.find(DLL_VERSION_STR);
//        if (pos >= out.size()) {
//            qCritical() << "命令执行错误，无法获取到Jlink版本";
//        }
//        string ver = out.substr(pos + dll_version.size());
//        char ver_str[32] = {0};
//        float ver_float = 0.0;
//        sscanf(ver.c_str(), "%[^d]", ver_str);
//        sscanf(ver_str, "%f", &ver_float);

//        if (ver_float < 6.9) {
//            qCritical() << "当前Jlink版本不支持擦除选区";
//            QMessageBox::information(this, tr("警告"), tr("当前Jlink版本不支持擦除选区"), tr("退出"));
//            ui->checkBox_erase_sector->toggle();
//        }
    } else {
        jlinkArgs.erase_chip = false;
        qInfo() << "取消选择擦除整个芯片";
    }
}
