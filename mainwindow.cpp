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
#include <QDebug>
#include <iostream>
#include <QSettings>
#include <string>
#include <QMutex>
#include <QWidget>

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

//    ui->radioButton_choose_loadfile1->setStyleSheet("QRadioButton::indicator {width:15px;height:15px;border-radius:7px}"
//                                   "QRadioButton::indicator:checked {background-color:green;}"
//                                   "QRadioButton::indicator:unchecked {background-color:red;}"
//                                   );

    /* Linux不会弹出进度窗口，因此自己实现进度显示条 */
#if defined(Q_OS_LINUX)
    process = new QProcess(this);
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(on_readoutput()));
    connect(process, SIGNAL(finished(int)), this, SLOT(process_finished(int)));
    pProgressBar = new QProgressBar();
    pProgressBar->setRange(0, 100);
    pProgressBar->setFixedHeight(20);
//    pProgressBar->setStyleSheet("QProgressBar::chunk{background:rgb(102,238,53)}");
    statusBar()->addWidget(pProgressBar);
#endif

    connect_type << JTAG << SWD << cJTAG;

    /* 恢复窗口设置 */
    restoreUiSettings();

    // 加载并设置固件记录，放在JLink配置加载之前
    load_fw_records();
    set_fw_records();

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

void MainWindow::reboot_mainwindow()
{
    qDebug() << "Program rebooting now";
    QString program = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    QString workingDirectory = QDir::currentPath();
    QProcess::startDetached(program, arguments, workingDirectory);
    QApplication::exit();
}

void MainWindow::on_readoutput()
{
    string out = QString::fromLocal8Bit(process->readLine()).toStdString();

    log_buffer += QString::fromStdString(out);

    if (out.find("Programming flash") < out.size())
    {
        program_grogress = 1;
    }

    if (program_grogress == 1)
    {
        long pos = out.find("%");
        int value = std::stoi(out.substr(pos - 3, 3).c_str());
        pProgressBar->setValue(value);
        if (out.find("Done.") < out.size())
        {
            program_grogress = 2;
        }
    }
    else if (program_grogress == 2)
    {
        if (/*out.find("J-Link: Flash download:") < out.size() && */out.find("O.K.") < out.size()) {
            program_grogress = 100;
        }
    }
}

void MainWindow::process_finished(int exit_code)
{
//    if (exit_code) return ;

    ui->textBrowser_log_2->append(log_buffer);

    string out;
    QProcess *p = process;

    out = QString::fromLocal8Bit(p->readAllStandardOutput()).toStdString();
//    ui->textBrowser_log_2->append(QString::fromStdString(out));
//    ui->textBrowser_log_2->moveCursor(QTextCursor::End);

    if (out.find("Connecting to J-Link via USB...FAILED") < out.size())
    {
        qCritical() << "Connecting to J-Link via USB...FAILED";
        QMessageBox::information(this, tr("警告"), tr("当前Jlink没有连接上终端设备，请连接终端设备后再尝试"), tr("退出"));
    }
    else if (out.find("Cannot connect to target.") < out.size())
    {
        qCritical() << "Cannot connect to target.";
        QMessageBox::information(this, tr("警告"), tr("Cannot connect to target. 请检查终端设备JLink是否正常工作"), tr("退出"));
    }
    else if (out.find("Failed to power up DAP") < out.size())
    {
        qCritical() << "Cannot connect to target. Failed to power up DAP";
        QMessageBox::information(this, tr("警告"), tr("Cannot connect to target. Failed to power up DAP, 请检查终端设备供电是否正常"), tr("退出"));
    }
    else if (out.find("J-Link: Flash download:") < out.size() && out.find("O.K.") < out.size()) {
        program_grogress = 100;
    }

    if (program_grogress != 100) {
        QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_RED("  烧录 FAILED !!!");
        ui->textBrowser_log_2->append(str);
        ui->textBrowser_log_2->append("\r\n\r\n");
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
        pProgressBar->setValue(0);
    }
    else if (program_grogress == 100)
    {
        pProgressBar->setValue(100);
        QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_BLUE("  烧录 SUCCESS !!!");
        ui->textBrowser_log_2->append(str);
        ui->textBrowser_log_2->append("\r\n\r\n");
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
    }

    program_grogress = 0;
}

void MainWindow::restoreUiSettings()
{
    /* 当comboBox内容大于size时，会自动拉长comboBox，因此在程序启动的时候还原之前的配置，可解决此问题 */
    if (settings.contains("fw1_geometry"))
    {
        restoreGeometry(settings.value("fw1_geometry").toByteArray());
    }
    if (settings.contains("fw2_geometry"))
    {
        restoreGeometry(settings.value("fw2_geometry").toByteArray());
    }
    if (settings.contains("fw3_geometry"))
    {
        restoreGeometry(settings.value("fw3_geometry").toByteArray());
    }

    if (settings.contains("mainwindow"))
    {
        restoreState(settings.value("mainwindow").toByteArray());
    }

    if (settings.contains("geometry"))
    {
        restoreGeometry(settings.value("geometry").toByteArray());
    }
}

/* 保存窗口配置，以便下次启动载入配置 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    settings.setValue("mainwindow", saveState());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("fw1_geometry", ui->comboBox_fw1_path->saveGeometry());
    settings.setValue("fw2_geometry", ui->comboBox_fw2_path->saveGeometry());
    settings.setValue("fw3_geometry", ui->comboBox_fw3_path->saveGeometry());

    save_current_jlink_config(ui->comboBox_sn->currentText());          // 保存当前的配置
    save_fw_records();                                                  // 保存固件记录

    QWidget::closeEvent(event);
}

void MainWindow::set_fw_records()
{
    ui->comboBox_fw1_path->addItems(fw_records.fw_record_list);
    ui->comboBox_fw2_path->addItems(fw_records.fw2_record_list);
    ui->comboBox_fw3_path->addItems(fw_records.fw3_record_list);
}

void MainWindow::get_fw_records()
{
    fw_records.fw_record_list.clear();
    for (int i= 0; i < ui->comboBox_fw1_path->count(); i++)
    {
        if (ui->comboBox_fw1_path->itemText(i) == "")
            continue;
        fw_records.fw_record_list << ui->comboBox_fw1_path->itemText(i);
    }

    fw_records.fw2_record_list.clear();
    for (int i= 0; i < ui->comboBox_fw2_path->count(); i++)
    {
        if (ui->comboBox_fw2_path->itemText(i) == "")
            continue;
        fw_records.fw2_record_list << ui->comboBox_fw2_path->itemText(i);
    }

    fw_records.fw3_record_list.clear();
    for (int i= 0; i < ui->comboBox_fw3_path->count(); i++)
    {
        if (ui->comboBox_fw3_path->itemText(i) == "")
            continue;
        fw_records.fw3_record_list << ui->comboBox_fw3_path->itemText(i);
    }
}

void MainWindow::load_fw_records()
{
    QFile file(GLOBAL_CONFIG_FILE);
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

    qDebug() << "存在固件记录";

    if (root.contains(QStringLiteral("fw_records"))) {
        QJsonValue value = root.value("fw_records");

        qDebug() << "存在固件记录";

        if (value.isObject())
        {
            QJsonObject record = value.toObject();
            qDebug() << "存在固件记录";

            /* 读取固件1的记录 */
            if (record.contains("fw_list"))
            {

                qDebug() << "存在固件记录";
                QJsonValue fw_record = record.value("fw_list");
                if (fw_record.isArray())
                {
                    QJsonArray fw_record_list = fw_record.toArray();
                    int size = fw_record_list.size();
                    for (int i = 0; i < size; i++)
                    {
                        QJsonValue str = fw_record_list.at(i);
                        if (str.isString())
                        {
                            qDebug() << "读取固件1记录成功";
                            fw_records.fw_record_list << str.toString();
                        }
                    }
                }
            }
            /* 读取固件2的记录 */
            if (record.contains("fw2_list"))
            {
                QJsonValue fw2_record = record.value("fw2_list");
                if (fw2_record.isArray())
                {
                    QJsonArray fw2_record_list = fw2_record.toArray();
                    int size = fw2_record_list.size();
                    for (int i = 0; i < size; i++)
                    {
                        QJsonValue str = fw2_record_list.at(i);
                        if (str.isString())
                        {
                            qDebug() << "读取固件1记录成功";
                            fw_records.fw2_record_list << str.toString();
                        }
                    }
                }
            }

            /* 读取固件3的记录 */
            if (record.contains("fw3_list"))
            {
                QJsonValue fw3_record = record.value("fw3_list");
                if (fw3_record.isArray())
                {
                    QJsonArray fw3_record_list = fw3_record.toArray();
                    int size = fw3_record_list.size();
                    for (int i = 0; i < size; i++)
                    {
                        QJsonValue str = fw3_record_list.at(i);
                        if (str.isString())
                        {
                            qDebug() << "读取固件1记录成功";
                            fw_records.fw3_record_list << str.toString();
                        }
                    }
                }
            }
        }
    }
}

void MainWindow::save_fw_records(void)
{
    /* 读取全局配置文件 */
    QFile file(GLOBAL_CONFIG_FILE);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString value = file.readAll();
    file.close();

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if (!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qCritical() << tr("解析配置错误！");
        QMessageBox::information(this, tr("Information"), tr("保存失败   "), tr("确认"));
        return ;
    }

    get_fw_records();

    QJsonObject root = document.object();

    /* 向json中添加固件记录 */
    QJsonArray fw1_record;
    for (int i = 0; i < fw_records.fw_record_list.size(); i++)
    {
        qDebug() << "添加固件1记录";
        fw1_record.append(QJsonValue(fw_records.fw_record_list[i]));
    }

    QJsonArray fw2_record;
    for (int i = 0; i < fw_records.fw2_record_list.size(); i++)
    {
        fw2_record.append(QJsonValue(fw_records.fw2_record_list[i]));
    }

    QJsonArray fw3_record;
    for (int i = 0; i < fw_records.fw3_record_list.size(); i++)
    {
        fw3_record.append(QJsonValue(fw_records.fw3_record_list[i]));
    }

    QJsonObject record;
    record["fw_list"] = QJsonValue(fw1_record);
    record["fw2_list"] = QJsonValue(fw2_record);
    record["fw3_list"] = QJsonValue(fw3_record);

    root["fw_records"] = record;

    /* 写入修改后的内容到文件中 */
    if (!(file.open(QIODevice::WriteOnly))) {
        qCritical() << "open global config file failed";
    }

    QJsonDocument jsonDoc;
    jsonDoc.setObject(root);

    file.write(jsonDoc.toJson());
    file.close();
    qInfo() << "写global config配置文件成功";
}

void MainWindow::on_actionJLink_settings_triggered()
{
    settings_window = new JLink_settings;

    settings_window->show();

    /* settings windows必须实例化后，才能connect信号槽，否则会段错误导致程序结束 */
    connect(settings_window, SIGNAL(reboot_mainwindow()), this, SLOT(reboot_mainwindow()));
}

void MainWindow::get_current_jlink_config()
{
    jlinkArgs.usb_sn =  ui->comboBox_sn->currentText().toStdString();

    jlinkArgs.si = connect_type[ui->comboBox_2_connect_2->currentIndex()];
    jlinkArgs.speed = ui->comboBox_3_speed_2->currentText().toLong();
    jlinkArgs.dev_type = ui->comboBox_4_dev_type_2->currentText().toStdString();
    jlinkArgs.load_file = ui->comboBox_fw1_path->currentText().toStdString();
    jlinkArgs.load_file2 = ui->comboBox_fw2_path->currentText().toStdString();
    jlinkArgs.load_file3 = ui->comboBox_fw3_path->currentText().toStdString();
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

    get_connect_sn();

    // 读取配置记录的第一个，作为启动默认选项
    if (root.contains("current_dev_sn")) {
        QJsonValue value = root.value("current_dev_sn");
        if (value.isString()) {
            qDebug() << "dev sn count: " << ui->comboBox_sn->count();
            if (ui->comboBox_sn->count() == 0) {
                ui->comboBox_sn->addItem(value.toString());
            }
            else {
                ui->comboBox_sn->setCurrentText(value.toString());
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
                QString path = QString::fromStdString(item.toString().toStdString());
                insert_fw1_path_comboBox(path);
                fw_file1_check(ui->comboBox_fw1_path->currentText());
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
                QString path = QString::fromStdString(item.toString().toStdString());
                insert_fw2_path_comboBox(path);
                fw_file2_check(ui->comboBox_fw2_path->currentText());
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
                QString path = QString::fromStdString(item.toString().toStdString());
                insert_fw3_path_comboBox(path);
                fw_file3_check(ui->comboBox_fw3_path->currentText());
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
        if (p.contains(("fw_enabled"))) {
            item = p.value("fw_enabled");
            if (item.isBool()) {
                ui->radioButton_choose_loadfile1->setChecked(item.toBool());
                bool checked = item.toBool();
                jlinkArgs.load_file1_enable = checked;
                ui->comboBox_fw1_path->setEnabled(checked);
                ui->pushButton_2_choose_fw_2->setEnabled(checked);
            }
        }
        if (p.contains(("fw2_enabled"))) {
            item = p.value("fw2_enabled");
            if (item.isBool()) {
                ui->radioButton_choose_loadfile2->setChecked(item.toBool());
                bool checked = item.toBool();
                jlinkArgs.load_file2_enable = checked;
                ui->comboBox_fw2_path->setEnabled(checked);
                ui->pushButton_2_choose_fw_3->setEnabled(checked);
            }
        }
        if (p.contains(("fw3_enabled"))) {
            item = p.value("fw3_enabled");
            if (item.isBool()) {
                ui->radioButton_choose_loadfile3->setChecked(item.toBool());
                bool checked = item.toBool();
                jlinkArgs.load_file3_enable = checked;
                ui->comboBox_fw3_path->setEnabled(checked);
                ui->pushButton_2_choose_fw_4->setEnabled(checked);
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
    jlink_config.insert("firmware", ui->comboBox_fw1_path->currentText());
    jlink_config.insert("firmware2", ui->comboBox_fw2_path->currentText());
    jlink_config.insert("firmware3", ui->comboBox_fw3_path->currentText());
    jlink_config.insert("erase_chip", ui->checkBox_erase_sector->isChecked());
    jlink_config.insert("fw_start_addr", ui->lineEdit_mem_start_addr_1->displayText());
    jlink_config.insert("fw2_start_addr", ui->lineEdit_mem_start_addr_2->displayText());
    jlink_config.insert("fw3_start_addr", ui->lineEdit_mem_start_addr_3->displayText());
    jlink_config.insert("fw_enabled", ui->radioButton_choose_loadfile1->isChecked());
    jlink_config.insert("fw2_enabled", ui->radioButton_choose_loadfile2->isChecked());
    jlink_config.insert("fw3_enabled", ui->radioButton_choose_loadfile3->isChecked());

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

    // 使用QProcess来执行命令
    QProcess p(0);
    p.start(cmd);
    p.waitForStarted();
    p.waitForFinished();

    QString out = QString::fromLocal8Bit(p.readAllStandardOutput());

    ui->textBrowser_log_2->append(out);
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);
    qInfo() << out.toStdString().c_str();

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
        sscanf(jlink_sn[i].c_str(), "%ld", &sn[i]);
    }
    delete[] jlink_sn;
    fin.close();
    return count;
}

void MainWindow::on_pushButton_dev_flush_clicked()
{
    if (ui->comboBox_sn->currentText() != "")
    {
        save_current_jlink_config(ui->comboBox_sn->currentText());
    }

    get_connect_sn();
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
    if ( ui->radioButton_choose_loadfile1->isChecked() && jlinkArgs.load_file != "")
    {
        string load_file1 = jlinkArgs.load_file;
        if (get_file_format(load_file1) == ".bin") {
            if (ui->lineEdit_mem_start_addr_1->displayText() == "") {
                QMessageBox::information(this, tr("警告"), tr("请输入固件1 BIN文件的起始地址"), tr("退出"));
                return -1;
            }
        }
    }
    if ( ui->radioButton_choose_loadfile2->isChecked() && jlinkArgs.load_file2 != "")
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
    if ( ui->radioButton_choose_loadfile3->isChecked() && jlinkArgs.load_file3 != "")
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

int MainWindow::fw_path_check()
{
    if (ui->radioButton_choose_loadfile1->isChecked()) {
        QString path1 = remove_file_path_head(1);
        insert_fw1_path_comboBox(path1);
        if (fw_file1_check(ui->comboBox_fw1_path->currentText()) == false)
            return 1;
    }

    if (ui->radioButton_choose_loadfile2->isChecked()) {
        QString path2 = remove_file_path_head(2);
        insert_fw2_path_comboBox(path2);
        if (fw_file2_check(ui->comboBox_fw2_path->currentText()) == false)
            return 2;
    }

    if (ui->radioButton_choose_loadfile3->isChecked()) {
        QString path3 = remove_file_path_head(3);
        insert_fw3_path_comboBox(path3);
        if (fw_file3_check(ui->comboBox_fw3_path->currentText()) == false)
            return 3;
    }

    return 0;
}

int MainWindow::fw_file_check(void)
{
    if (ui->radioButton_choose_loadfile1->isChecked() && jlinkArgs.load_file != "" && !is_file_exit(jlinkArgs.load_file)) {
        QMessageBox::information(this, tr("警告"), tr("固件1 不存在"), tr("确认"), tr("退出"));
        return -1;
    }

    if (ui->radioButton_choose_loadfile2->isChecked() && jlinkArgs.load_file2 != "" && !is_file_exit(jlinkArgs.load_file2)) {
        QMessageBox::information(this, tr("警告"), tr("固件2 不存在"), tr("确认"), tr("退出"));
        return -2;
    }

    if (ui->radioButton_choose_loadfile3->isChecked() && jlinkArgs.load_file3 != "" && !is_file_exit(jlinkArgs.load_file3)) {
        QMessageBox::information(this, tr("警告"), tr("固件3 不存在"), tr("确认"), tr("退出"));
        return -3;
    }

    return 0;
}

void MainWindow::on_pushButton_flush_clicked()
{
#if defined(Q_OS_LINUX)
    pProgressBar->setValue(0);
    program_grogress = 0;
#endif
    if (ui->comboBox_sn->currentText() == "") {
        qInfo() << "当前无Jlink设备连接";
        QMessageBox::information(this, tr("警告"), tr("当前没有Jlink设备，请刷新"), tr("退出"));
        return ;
    }

    if (jlinkArgs.load_file1_enable == false && jlinkArgs.load_file2_enable == false && jlinkArgs.load_file3_enable == false)
    {
        qInfo() << "当前所有load file已禁用";
        QMessageBox::information(this, tr("警告"), tr("当前所有固件已禁用，请在设置中勾选固件"), tr("退出"));
        return ;
    }

    /* 校验未保存的路径 */
    fw_path_check();

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

    ui->textBrowser_log_2->append("Programming start ......................");
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);

    log_buffer.clear();

    // 执行获取动作
    QString cmd;
    cmd = "\"" + QString::fromStdString(globalConf.jlink_cmd_path) + "\"";
    // cmd.append(fileArgs.flush_file.c_str());

    cmd = cmd + " " + QString::fromStdString(fileArgs.flush_file);

    qInfo() << "exec cmd: " << cmd;

#if defined(Q_OS_LINUX)
    process->start(cmd);
    process->waitForStarted();
#elif defined(Q_OS_WIN)

    // 使用QProcess来执行命令
    QProcess p(0);
    p.start(cmd);
    p.waitForStarted();
    p.waitForFinished();

    string out = QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString();
    ui->textBrowser_log_2->append(QString::fromStdString(out));
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);

    if (out.find("Connecting to J-Link via USB...FAILED") < out.size())
    {
        qCritical() << "Connecting to J-Link via USB...FAILED";
        QMessageBox::information(this, tr("警告"), tr("当前Jlink没有连接上终端设备，请连接终端设备后再尝试"), tr("退出"));
        QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_RED("  烧录 FAILED !!!");
        ui->textBrowser_log_2->append(str);
        ui->textBrowser_log_2->append("\r\n\r\n");
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
    }
    else if (out.find("J-Link: Flash download:") < out.size() && out.find("O.K.")) {
        QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_BLUE("  烧录 SUCCESS !!!");
        ui->textBrowser_log_2->append(str);
        ui->textBrowser_log_2->append("\r\n\r\n");
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
    }
    else {
        QString str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_RED("  烧录 FAILED !!!");
        ui->textBrowser_log_2->append(str);
        ui->textBrowser_log_2->append("\r\n\r\n");
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
    }
#endif

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
#if defined(Q_OS_LINUX)
    cmd = QString::fromStdString(globalConf.jlink_cmd_path);
#elif defined(Q_OS_WIN)
    cmd = "\"" + QString::fromStdString(globalConf.jlink_cmd_path) + "\"";
#endif

    cmd = cmd + " " + QString::fromStdString(fileArgs.reboot_file);

    qInfo() << "exec cmd: " << cmd;

    // 使用QProcess来执行命令
    QProcess p(0);
    p.start(cmd);
    p.waitForStarted();
    p.waitForFinished();

    string out = QString::fromLocal8Bit(p.readAllStandardOutput()).toStdString();
    ui->textBrowser_log_2->append(QString::fromStdString(out));
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);

    QString str;
    if (out.find("Connecting to J-Link via USB...FAILED") < out.size())
    {
        qCritical() << "Connecting to J-Link via USB...FAILED";
        QMessageBox::information(this, tr("警告"), tr("当前Jlink没有连接上终端设备，请连接终端设备后再尝试"), tr("退出"));
        str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_BLUE("  重启芯片 FAILED !!!");
    }
    else if (out.find("Cannot connect to target.") < out.size())
    {
        qCritical() << "Cannot connect to target.";
        QMessageBox::information(this, tr("警告"), tr("Cannot connect to target. 请检查Jlink是否正常工作"), tr("退出"));
        str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_BLUE("  重启芯片 FAILED !!!");
    }
    else if (out.find("Failed to power up DAP") < out.size())
    {
        qCritical() << "Cannot connect to target. Failed to power up DAP";
        QMessageBox::information(this, tr("警告"), tr("Cannot connect to target. Failed to power up DAP, 请检查终端设备供电是否正常"), tr("退出"));
        str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_BLUE("  重启芯片 FAILED !!!");
    }
    else {
        str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_BLUE("  重启芯片 SUCCESS !!!");
    }

    ui->textBrowser_log_2->append(str);
    ui->textBrowser_log_2->append("\r\n\r\n");
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);
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
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);

    QString str;
    if (out.find("Connecting to J-Link via USB...FAILED") < out.size())
    {
        qCritical() << "Connecting to J-Link via USB...FAILED";
        QMessageBox::information(this, tr("警告"), tr("当前Jlink没有连接上终端设备，请连接终端设备后再尝试"), tr("退出"));
        str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_RED("  擦除芯片 FAILED !!!");
    }
    else if (out.find("Failed to power up DAP") < out.size())
    {
        qCritical() << "Cannot connect to target. Failed to power up DAP";
        QMessageBox::information(this, tr("警告"), tr("Cannot connect to target. Failed to power up DAP, 请检查终端设备供电是否正常"), tr("退出"));
        str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_RED("  擦除芯片 FAILED !!!");
    }
    else if (out.find("Cannot connect to target.") < out.size())
    {
        qCritical() << "Cannot connect to target. Failed to power up DAP";
        QMessageBox::information(this, tr("警告"), tr("Cannot connect to target. 请检查Jlink是否正常工作"), tr("退出"));
        str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_RED("  擦除芯片 FAILED !!!");
    }
    else {
        str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + TEXT_COLOR_BLUE("  擦除芯片 SUCCESS !!!");
    }

    ui->textBrowser_log_2->append(str);
    ui->textBrowser_log_2->append("\r\n\r\n");
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);
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
    /* 部分控件的拖放事件无法当前接口接收 */
#if 0
    //获取文件路径 (QString)
    QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty()) return;
    QString qStr = urls.first().toLocalFile();

    //转为char*
    QByteArray qByteArrary = qStr.toLatin1();
    char* filePath = qByteArrary.data();
    QString path = filePath;
    qInfo() << "select firmware file: " << filePath;

    if (this->focusWidget() == ui->comboBox_fw1_path)
    {
        ui->comboBox_fw1_path->clear();
        insert_fw1_path_comboBox(path);
        save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
    }
    else if (this->focusWidget() == ui->comboBox_fw2_path)
    {
        ui->comboBox_fw2_path->clear();
        insert_fw2_path_comboBox(path);
        save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
    }
    else if (this->focusWidget() == ui->comboBox_fw3_path)
    {
        ui->comboBox_fw3_path->clear();
        insert_fw3_path_comboBox(path);
        save_current_jlink_config(ui->comboBox_sn->currentText()); //保存当前配置
    }
    else {
        qInfo() << "鼠标没有指向 lineEdit";
    }
#endif
}

void MainWindow::on_comboBox_sn_currentTextChanged(const QString &arg1)
{
    set_default_config_display(arg1);         // 读取并设置选择的设备的配置
}

void MainWindow::on_pushButton_rtt_viewer_clicked()
{
    string jlink_exe;
#if defined(Q_OS_LINUX)
    jlink_exe = "JLinkRTTViewerExe";
#elif defined(Q_OS_WIN)
    jlink_exe = "JLinkRTTViewer.exe";
#endif
    std::string rtt_viewer_path = get_file_path(globalConf.jlink_cmd_path) +  "/" + jlink_exe;

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

QString MainWindow::remove_file_path_head(int index)
{
    QString path;
    switch(index)
    {
    case 1: {
        if (ui->comboBox_fw1_path->currentText().toStdString().substr(0, 7) == "file://")
        {
            path = QString::fromStdString(ui->comboBox_fw1_path->currentText().toStdString().substr(7));
            ui->comboBox_fw1_path->setCurrentText(path);
            qDebug() << "去除固件1的文件头：" << ui->comboBox_fw1_path->currentText();
        } else {
            path = ui->comboBox_fw1_path->currentText();
        }
    } break;
    case 2: {
        if (ui->comboBox_fw2_path->currentText().toStdString().substr(0, 7) == "file://")
        {
            path = QString::fromStdString(ui->comboBox_fw2_path->currentText().toStdString().substr(7));
            ui->comboBox_fw2_path->setCurrentText(path);
        } else {
            path = ui->comboBox_fw2_path->currentText();
        }
    } break;
    case 3: {
        if (ui->comboBox_fw3_path->currentText().toStdString().substr(0, 7) == "file://")
        {
            path = QString::fromStdString(ui->comboBox_fw3_path->currentText().toStdString().substr(7));
            ui->comboBox_fw3_path->setCurrentText(path);
        } else {
            path = ui->comboBox_fw3_path->currentText();
        }
    } break;
    }
    return path;
}

void MainWindow::insert_fw1_path_comboBox(QString &path)
{
    if (path == "")
        return ;

    /* 去重 */
    for (int i = 0; i < ui->comboBox_fw1_path->count(); i++)
    {
        if (path == ui->comboBox_fw1_path->itemText(i))
        {
            ui->comboBox_fw1_path->removeItem(i);
        }
    }
    /* 达到容量后移除最后一条 */
    if (ui->comboBox_fw1_path->count() >= FW_RECORDS_MAX_SIZE)
    {
        ui->comboBox_fw1_path->removeItem(ui->comboBox_fw1_path->count() - 1);
    }

    ui->comboBox_fw1_path->insertItem(0, path);
    ui->comboBox_fw1_path->setCurrentIndex(0);
}

void MainWindow::insert_fw2_path_comboBox(QString &path)
{
    if (path == "")
        return ;

    for (int i = 0; i < ui->comboBox_fw2_path->count(); i++)
    {
        if (path == ui->comboBox_fw2_path->itemText(i))
        {
            ui->comboBox_fw2_path->removeItem(i);
        }
    }

    if (ui->comboBox_fw2_path->count() >= FW_RECORDS_MAX_SIZE)
    {
        ui->comboBox_fw2_path->removeItem(ui->comboBox_fw2_path->count() - 1);
    }


    ui->comboBox_fw2_path->insertItem(0, path);
    ui->comboBox_fw2_path->setCurrentIndex(0);
}

void MainWindow::insert_fw3_path_comboBox(QString &path)
{
    if (path == "")
        return ;

    for (int i = 0; i < ui->comboBox_fw3_path->count(); i++)
    {
        if (path == ui->comboBox_fw3_path->itemText(i))
        {
            ui->comboBox_fw3_path->removeItem(i);
        }
    }

    if (ui->comboBox_fw3_path->count() >= FW_RECORDS_MAX_SIZE)
    {
        ui->comboBox_fw3_path->removeItem(ui->comboBox_fw3_path->count() - 1);
    }


    ui->comboBox_fw3_path->insertItem(0, path);
    ui->comboBox_fw3_path->setCurrentIndex(0);
}

void MainWindow::on_pushButton_2_choose_fw_2_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "选择HEX固件1", ui->comboBox_fw1_path->currentText());
    jlinkArgs.load_file = filename.toStdString();

    if (filename != "")
    {
        insert_fw1_path_comboBox(filename);
        fw_file1_check(filename);
    }
    qInfo() << "firmware1 selected: " << filename;
}

void MainWindow::on_pushButton_2_choose_fw_3_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "选择HEX固件2", ui->comboBox_fw2_path->currentText());
    jlinkArgs.load_file2 = filename.toStdString();

    if (filename != "") {
        insert_fw2_path_comboBox(filename);
        fw_file2_check(filename);
    }
    qInfo() << "firmware2 selected: " << filename;
}

void MainWindow::on_pushButton_2_choose_fw_4_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "选择HEX固件3", ui->comboBox_fw3_path->currentText());
    jlinkArgs.load_file3 = filename.toStdString();

    if (filename != "") {
        insert_fw3_path_comboBox(filename);
        fw_file3_check(filename);
    }

    qInfo() << "firmware3 selected: " << filename;
}

bool MainWindow::fw_file1_check(QString filename)
{
    if (filename == "") {
        return true;
    }
    qDebug() << "固件1: " << filename;

    string format = get_file_format(filename.toStdString());
    if (format == ".bin") {
        qDebug() << "已选择Bin文件";
        ui->textBrowser_log_2->append(TEXT_COLOR_BLUE(">> 请输入固件1 Bin文件的起始地址"));
        if (ui->radioButton_choose_loadfile1->isChecked()) ui->lineEdit_mem_start_addr_1->setEnabled(true);
    } else if (format == ".hex") {
        qDebug() << "已选择HEX文件";

        int ret = get_bin_file_start_addr(filename.toStdString(), jlinkArgs.file_start_addr, jlinkArgs.file_end_addr);
        if (ret) {
            QMessageBox::information(this, tr("警告"), tr("HEX文件错误，请检查"), tr("确认"), tr("退出"));
            ui->textBrowser_log_2->append(filename);
            ui->textBrowser_log_2->append(TEXT_COLOR_RED("固件1 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件"));
            ui->textBrowser_log_2->moveCursor(QTextCursor::End);
            ui->comboBox_fw1_path->removeItem(ui->comboBox_fw1_path->currentIndex());
            ui->comboBox_fw1_path->setCurrentText("");
            return false;
        }
        qDebug() << "jlinkArgs.file_start_addr: " << jlinkArgs.file_start_addr.c_str()
                 << "jlinkArgs.file_end_addr: " << jlinkArgs.file_end_addr.c_str();

        ui->lineEdit_mem_start_addr_1->setText(QString::fromStdString(jlinkArgs.file_start_addr));
        ui->lineEdit_mem_start_addr_1->setEnabled(false);
    } else {
        qCritical() << "文件格式不正确，请检查";
        ui->textBrowser_log_2->append(TEXT_COLOR_RED("固件1 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件"));
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
        ui->comboBox_fw1_path->removeItem(ui->comboBox_fw1_path->currentIndex());
        ui->comboBox_fw1_path->setCurrentText("");
        return false;
    }

    return true;
}

bool MainWindow::fw_file2_check(QString filename)
{
    if (filename == "") {
        return true;
    }

    string format = get_file_format(filename.toStdString());
    if (format == ".bin") {
        qDebug() << "已选择Bin文件";
        ui->textBrowser_log_2->append(TEXT_COLOR_BLUE(">> 请输入固件2 Bin文件的起始地址"));
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
        if (ui->radioButton_choose_loadfile1->isChecked()) ui->lineEdit_mem_start_addr_2->setEnabled(true);
    } else if (format == ".hex") {
        qDebug() << "已选择HEX文件";

        int ret = get_bin_file_start_addr(filename.toStdString(), jlinkArgs.file2_start_addr, jlinkArgs.file2_end_addr);
        if (ret) {
            QMessageBox::information(this, tr("警告"), tr("HEX文件错误，请检查"), tr("确认"), tr("退出"));
            ui->textBrowser_log_2->append(filename);
            static QMutex mutex;
            mutex.lock();
            ui->textBrowser_log_2->append(TEXT_COLOR_RED("固件2 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件"));
            mutex.unlock();
            ui->textBrowser_log_2->moveCursor(QTextCursor::End);
            ui->comboBox_fw2_path->removeItem(ui->comboBox_fw2_path->currentIndex());
            ui->comboBox_fw2_path->setCurrentText("");
            return false;
        }
        qDebug() << "jlinkArgs.file2_start_addr: " << jlinkArgs.file2_start_addr.c_str()
                 << "jlinkArgs.file2_end_addr: " << jlinkArgs.file2_end_addr.c_str();

        ui->lineEdit_mem_start_addr_2->setText(QString::fromStdString(jlinkArgs.file2_start_addr));
        ui->lineEdit_mem_start_addr_2->setEnabled(false);
    } else {
        qCritical() << "文件格式不正确，请检查";
        static QMutex mutex;
        mutex.lock();
        ui->textBrowser_log_2->append(TEXT_COLOR_RED("固件2 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件"));
        mutex.unlock();
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
        ui->comboBox_fw2_path->removeItem(ui->comboBox_fw2_path->currentIndex());
        ui->comboBox_fw2_path->setCurrentText("");
        return false;
    }

    return true;
}

bool MainWindow::fw_file3_check(QString filename)
{
    if (filename == "") {
        return true;
    }

    qDebug() << "filename: " << filename;

    string format = get_file_format(filename.toStdString());
    if (format == ".bin") {
        qDebug() << "已选择Bin文件";
        ui->textBrowser_log_2->append(TEXT_COLOR_BLUE(">> 请输入固件3 Bin文件的起始地址"));
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
        if (ui->radioButton_choose_loadfile1->isChecked()) ui->lineEdit_mem_start_addr_3->setEnabled(true);
    } else if (format == ".hex") {
        qDebug() << "已选择HEX文件";
        int ret = get_bin_file_start_addr(filename.toStdString(), jlinkArgs.file3_start_addr, jlinkArgs.file3_end_addr);
        if (ret) {
            QMessageBox::information(this, tr("警告"), tr("HEX文件错误，请检查"), tr("确认"), tr("退出"));
            ui->textBrowser_log_2->append(filename);
            ui->textBrowser_log_2->append(TEXT_COLOR_RED("固件3 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件"));
            ui->textBrowser_log_2->moveCursor(QTextCursor::End);
            ui->comboBox_fw3_path->removeItem(ui->comboBox_fw3_path->currentIndex());
            ui->comboBox_fw3_path->setCurrentText("");
            return false;
        }
        qDebug() << "jlinkArgs.file3_start_addr: " << jlinkArgs.file3_start_addr.c_str()
                 << "jlinkArgs.file3_end_addr: " << jlinkArgs.file3_end_addr.c_str();

        ui->lineEdit_mem_start_addr_3->setText(QString::fromStdString(jlinkArgs.file3_start_addr));
        ui->lineEdit_mem_start_addr_3->setEnabled(false);
    } else {
        printf("test4");
        qCritical() << "文件格式不正确，请检查";
        ui->textBrowser_log_2->append(TEXT_COLOR_RED("固件3 选择的文件格式不正确，请选择.bin或者.hex后缀格式的文件"));
        ui->textBrowser_log_2->moveCursor(QTextCursor::End);
        ui->comboBox_fw3_path->removeItem(ui->comboBox_fw3_path->currentIndex());
        ui->comboBox_fw3_path->setCurrentText("");
        return false;
    }

    return true;
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
#if 0
#define JLINK_VERSION_TXT   "./config/get_jlink_version.txt"

        QFileInfo file(JLINK_VERSION_TXT);
        if(file.exists()==false) {
            qDebug() << JLINK_VERSION_TXT << "不存在";
            QFile file_create(JLINK_VERSION_TXT);
            file_create.open(QIODevice::ReadWrite | QIODevice::Text);
            file_create.close();

            using namespace std;
            ofstream get_jlink_version_s;
            get_jlink_version_s.open(JLINK_VERSION_TXT);

            get_jlink_version_s << "?\n";
            get_jlink_version_s.close();
        }

        QObject *parent=NULL;
        QString program = QString::fromStdString(globalConf.jlink_cmd_path);
        QStringList arguments;
        arguments << "\"" JLINK_VERSION_TXT "\"";

        QProcess *myProcess = new QProcess(parent);
        myProcess->start(program, arguments);
        myProcess->waitForStarted();
        myProcess->waitForFinished();

        string out = QString::fromLocal8Bit(myProcess->readAllStandardOutput()).toStdString();
        qDebug() << out.c_str();

#define DLL_VERSION_STR "DLL version V"

        string dll_version = DLL_VERSION_STR;

        long long unsigned pos = out.find(DLL_VERSION_STR);
        if (pos >= out.size()) {
            qCritical() << "命令执行错误，无法获取到Jlink版本";
        }
        string ver = out.substr(pos + dll_version.size());
        char ver_str[32] = {0};
        float ver_float = 0.0;
        sscanf(ver.c_str(), "%[^d]", ver_str);
        sscanf(ver_str, "%f", &ver_float);

        if (ver_float < 6.9) {
            qCritical() << "当前Jlink版本不支持擦除选区";
            QMessageBox::information(this, tr("警告"), tr("当前Jlink版本不支持擦除选区"), tr("退出"));
            ui->checkBox_erase_sector->toggle();
        }
#endif
    } else {
        jlinkArgs.erase_chip = false;
        qInfo() << "取消选择擦除整个芯片";
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return)
    {
        qDebug() << "回车按下，检测固件格式是否正确";

        if (this->focusWidget() == ui->comboBox_fw1_path) {
            qDebug() << "当前聚焦于固件1输入框";
            QString path = ui->comboBox_fw1_path->currentText();
            /* 去掉路径头 */
            QString path1 = remove_file_path_head(1);
            if (fw_file1_check(ui->comboBox_fw1_path->currentText()))
                insert_fw1_path_comboBox(path1);
        }

        if (this->focusWidget() == ui->comboBox_fw2_path) {
            qDebug() << "当前聚焦于固件2输入框";
            QString path = ui->comboBox_fw2_path->currentText();
            /* 去掉路径头 */
            QString path1 = remove_file_path_head(2);
            if (fw_file2_check(ui->comboBox_fw2_path->currentText()))
                insert_fw2_path_comboBox(path1);
        }

        if (this->focusWidget() == ui->comboBox_fw3_path) {
            qDebug() << "当前聚焦于固件3输入框";
            QString path = ui->comboBox_fw3_path->currentText();
            /* 去掉路径头 */
            QString path1 = remove_file_path_head(3);
            if (fw_file3_check(ui->comboBox_fw3_path->currentText()))
                insert_fw3_path_comboBox(path1);
        }
    }

    QWidget::keyPressEvent(event);
}

void MainWindow::on_radioButton_choose_loadfile1_clicked(bool checked)
{
    jlinkArgs.load_file1_enable = checked;
    ui->comboBox_fw1_path->setEnabled(checked);
    if (checked)
        fw_file1_check(ui->comboBox_fw1_path->currentText());
    ui->pushButton_2_choose_fw_2->setEnabled(checked);
    QString log = checked ? "启用" : "禁用";
    ui->textBrowser_log_2->append("固件1 已" + log);
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);
}


void MainWindow::on_radioButton_choose_loadfile2_clicked(bool checked)
{
    jlinkArgs.load_file2_enable = checked;
    ui->comboBox_fw2_path->setEnabled(checked);
    if (checked)
        fw_file2_check(ui->comboBox_fw2_path->currentText());
    ui->pushButton_2_choose_fw_3->setEnabled(checked);
    QString log = checked ? "启用" : "禁用";
    ui->textBrowser_log_2->append("固件2 已" + log);
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);
}

void MainWindow::on_radioButton_choose_loadfile3_clicked(bool checked)
{
    jlinkArgs.load_file3_enable = checked;
    ui->comboBox_fw3_path->setEnabled(checked);
    if (checked)
        fw_file3_check(ui->comboBox_fw3_path->currentText());
    ui->pushButton_2_choose_fw_4->setEnabled(checked);
    QString log = checked ? "启用" : "禁用";
    ui->textBrowser_log_2->append("固件3 已" + log);
    ui->textBrowser_log_2->moveCursor(QTextCursor::End);
}

void MainWindow::on_actionSave_triggered()
{
    if (ui->comboBox_sn->currentText() != "")
    {
        save_current_jlink_config(ui->comboBox_sn->currentText());
    }
}

