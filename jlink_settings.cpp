#include "jlink_settings.h"
#include "ui_jlink_settings.h"
#include "src/hj_file_write.h"
#include "utils/utils.h"
#include "mainwindow.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QTextEdit>

extern global_config_t globalConf;

JLink_settings::JLink_settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JLink_settings)
{
    ui->setupUi(this);

    // 设置关闭销毁
    this->setAttribute(Qt::WA_DeleteOnClose);

    // 设置应用模态对话框
    this->setWindowModality(Qt::ApplicationModal);

    ui->lineEdit_JLink_path->setText(QString::fromStdString(globalConf.jlink_cmd_path));
    ui->pushButton_choose_JLink_path->setToolTip(tr("选择JLink.exe"));

    /* 载入global config配置文件 */
    load_global_config_file();
    ui->textEdit_global_config->setReadOnly(true);
    ui->textEdit_global_config->setLineWrapMode(QTextEdit::NoWrap);
}

JLink_settings::~JLink_settings()
{
    delete ui;
}

void JLink_settings::load_global_config_file()
{
    QFile file(GLOBAL_CONFIG_FILE);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString value = file.readAll();
    file.close();

    ui->textEdit_global_config->setText(value);
}

void JLink_settings::on_pushButton_3_cancle_clicked()
{
    this->close();
}

void JLink_settings::on_pushButton_choose_JLink_path_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "选择JLink可执行文件", ui->lineEdit_JLink_path->displayText());
    if (filename == "") {
        return ;
    }
    qDebug("file path: %s", qPrintable(filename));

    ui->lineEdit_JLink_path->setText(filename);
}

void JLink_settings::on_pushButton_2_save_clicked()
{
    if (!is_file_exit(ui->lineEdit_JLink_path->displayText().toStdString()))
    {
        QMessageBox::information(this, tr("Information"), tr("JLink可执行文件不存在，请选择正确路径    "), tr("确认"));
        return ;
    }

    QFile file(GLOBAL_CONFIG_FILE);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString value = file.readAll();
    file.close();

     global_config_t *global_config = &globalConf;

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if (!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qCritical() << tr("解析配置错误！");
        QMessageBox::information(this, tr("Information"), tr("保存失败   "), tr("确认"));
        return ;
    }

    QJsonObject root = document.object();

    root["jlink_cmd_path"] = ui->lineEdit_JLink_path->displayText();
    global_config->jlink_cmd_path = ui->lineEdit_JLink_path->displayText().toStdString();

    if (!(file.open(QIODevice::WriteOnly))) {
        qCritical() << "open global config file failed";
    }

    QJsonDocument jsonDoc;
    jsonDoc.setObject(root);

    file.write(jsonDoc.toJson());
    file.close();
    qInfo() << "写global config配置文件成功";
    QMessageBox::StandardButton btn = (QMessageBox::StandardButton )QMessageBox::information(this, tr("Information"), tr("保存成功    "), tr("确认"));
    if (btn == QMessageBox::NoButton) {
        this->close();
        qDebug() << "保存成功，关闭JLink配置窗口";
    }
}

void JLink_settings::on_pushButton_4_reset_default_clicked()
{
    remove(GLOBAL_CONFIG_FILE);

    /* 恢复备份文件 */
    QFile file_reset(GLOBAL_CONFIG_FILE_BACKUP);
    file_reset.copy(GLOBAL_CONFIG_FILE);

    QFile file(GLOBAL_CONFIG_FILE);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString value = file.readAll();
    file.close();

    ui->textEdit_global_config->setText(value);

     global_config_t *global_config = &globalConf;

    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(value.toUtf8(), &parseJsonErr);
    if (!(parseJsonErr.error == QJsonParseError::NoError))
    {
        qCritical() << tr("解析配置错误！");
        return ;
    }

    QJsonObject root = document.object();

    QString default_jlink_cmd_path = root["jlink_cmd_path"].toString();
    global_config->jlink_cmd_path = default_jlink_cmd_path.toStdString();

    ui->lineEdit_JLink_path->setText(default_jlink_cmd_path);

    if (!(file.open(QIODevice::WriteOnly))) {
        qCritical() << "open global config file failed";
    }

    QJsonDocument jsonDoc;
    jsonDoc.setObject(root);

    file.write(jsonDoc.toJson());
    file.close();
    qInfo() << "写global config配置文件成功";

    QMessageBox::StandardButton btn = (QMessageBox::StandardButton )QMessageBox::information(this, tr("Information"), tr("恢复默认成功，需要重启程序生效，是否重启程序？ "), tr("确认"), tr("取消"));
    qDebug() << "button: " << btn;
    /* 选择重启程序 */
    if (btn == QMessageBox::NoButton) {
        this->close();
        emit reboot_mainwindow();
        qDebug() << "保存成功，关闭JLink配置窗口";
    }
    else
    {
        qDebug() << "保存成功，下一次再重启";
    }
}

void JLink_settings::on_pushButton_global_config_edit_enable_clicked()
{
    ui->textEdit_global_config->setReadOnly(false);
}


void JLink_settings::on_pushButton_save_global_config_clicked()
{
    QString config_str = ui->textEdit_global_config->toPlainText();

    ui->textEdit_global_config->setReadOnly(true);

    QJsonObject root;
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(config_str.toUtf8(), &parseJsonErr);

    if (parseJsonErr.error != QJsonParseError::NoError)
    {
        qWarning() << tr("解析配置错误！");
        QMessageBox::information(this, tr("Error"), tr("JSON格式错误，请检查输入格式是否正确    "), tr("确认"));
        return;
    }

    QFile file(GLOBAL_CONFIG_FILE);
    if (!(file.open(QIODevice::WriteOnly))) {
        qCritical() << "open global config file failed";
        QMessageBox::information(this, tr("Information"), tr("保存失败   "), tr("确认"));
        return ;
    }

    file.write(document.toJson());
    file.close();
    qInfo() << "写global config配置文件成功";

    QMessageBox::StandardButton btn = (QMessageBox::StandardButton )QMessageBox::information(this, tr("Information"), tr("保存成功，需要重启程序生效，是否重启程序？ "), tr("确认"), tr("取消"));
    qDebug() << "button: " << btn;
    /* 选择重启程序 */
    if (btn == QMessageBox::NoButton) {
        this->close();
        emit reboot_mainwindow();
        qDebug() << "保存成功，关闭JLink配置窗口";
    }
    else
    {
        this->close();
        qDebug() << "保存成功，下一次再重启";
    }
}

