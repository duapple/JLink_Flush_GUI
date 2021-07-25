#include "jlink_settings.h"
#include "ui_jlink_settings.h"
#include "src/hj_file_write.h"
#include "mainwindow.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QMessageBox>

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

}

JLink_settings::~JLink_settings()
{
    delete ui;
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
        return ;
    }

    QJsonObject root = document.object();

    QString default_jlink_cmd_path = "C:/Program Files (x86)/SEGGER/JLink/JLink.exe";
    root["jlink_cmd_path"] = default_jlink_cmd_path;
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
}
