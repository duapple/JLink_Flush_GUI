#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "jlink_settings.h"
#include "src/hj_file_write.h"
#include <QThread>
#include <iostream>
#include <QProcess>
#include <QSettings>
#include <QProgressBar>


#define GLOBAL_CONFIG_FILE      CONFIG_PATH "global_config.json"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Mythread: public QThread
{
    Q_OBJECT
public:
    std::string rtt_viewer_exec_cmd;
    Mythread(QObject * parent = 0);

protected:
    void run();
};

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

    int bin_start_addr_check(void);
    int fw_file_check(void);
    void fw_file1_check(QString filename);
    void fw_file2_check(QString filename);
    void fw_file3_check(QString filename);
    void clear_fw_file_hex(void);

private slots:
    void on_actionJLink_settings_triggered();

    void on_pushButton_dev_flush_clicked();

    void on_pushButton_flush_clicked();

    void on_pushButton_2_choose_fw_2_clicked();

    void on_pushButton_reboot_clicked();

    void on_pushButton_erase_clicked();

    void on_comboBox_sn_currentTextChanged(const QString &arg1);

    void on_pushButton_rtt_viewer_clicked();

    void on_pushButton_2_choose_fw_3_clicked();

    void on_pushButton_2_choose_fw_4_clicked();

    void on_lineEdit_fw_path_3_editingFinished();

    void on_lineEdit_fw_path_4_editingFinished();

    void on_lineEdit_fw_path_2_editingFinished();

    void on_action_clear_log_triggered();

    void on_checkBox_erase_sector_toggled(bool checked);

    void on_choose_load_file2_triggered(bool checked);

    void on_choose_load_file3_triggered(bool checked);

    void on_choose_load_file1_triggered(bool checked);

    void on_readoutput();

    void process_finished(int exit_code);

    void on_radioButton_choose_loadfile1_clicked(bool checked);

    void on_radioButton_choose_loadfile2_clicked(bool checked);

    void on_radioButton_choose_loadfile3_clicked(bool checked);

private:
    Ui::MainWindow *ui;

    Mythread *mythread;

    QProcess *process;

    QSettings settings;

    QProgressBar *pProgressBar;

    int program_grogress;

    void restoreUiSettings();

protected:
    void closeEvent(QCloseEvent *event);
};
#endif // MAINWINDOW_H
