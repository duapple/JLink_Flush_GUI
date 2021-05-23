#ifndef JLINK_SETTINGS_H
#define JLINK_SETTINGS_H

#include <QDialog>

namespace Ui {
class JLink_settings;
}

typedef enum {
    JTAG = 0,
    SWD,
    cJTAG
} specify_interface_t;

typedef struct {
    std::string jlink_cmd_path;
    std::string connect_type;
    std::string device_type;
} global_config_t;

class JLink_settings : public QDialog
{
    Q_OBJECT

public:

    explicit JLink_settings(QWidget *parent = nullptr);
    ~JLink_settings();


signals:


private slots:
    void on_pushButton_3_cancle_clicked();

    void on_pushButton_choose_JLink_path_clicked();

    void on_pushButton_2_save_clicked();

    void on_pushButton_4_reset_default_clicked();

private:
    Ui::JLink_settings *ui;
};

#endif // JLINK_SETTINGS_H
