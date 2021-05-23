#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "jlink_settings.h"
#include "src/hj_file_write.h"

#define GLOBAL_CONFIG_FILE      CONFIG_PATH "global_config.json"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    JLink_settings *settings_window;
    jlink_flush_config_file_args_t jlinkArgs;
    JlinkFlushConfigFileArgs fileArgs;

    void get_global_config();
    void get_current_jlink_config();
    uint32_t get_jlink_sn(const char* file, long* sn_ary, uint32_t len);
    void get_connect_sn();
    void set_default_config_display();
    void set_default_config_display(QString usb_sn);
    void save_current_jlink_config(QString usb_sn);

    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);

private slots:
    void on_actionJLink_settings_triggered();

    void on_pushButton_dev_flush_clicked();

    void on_pushButton_flush_clicked();

    void on_pushButton_2_choose_fw_2_clicked();

    void on_pushButton_reboot_clicked();

    void on_pushButton_erase_clicked();

    void on_comboBox_sn_currentTextChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
