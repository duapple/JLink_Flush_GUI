#ifndef HJ_FILE_WRITE_H
#define HJ_FILE_WRITE_H

#include <iostream>
#include "jlink_settings.h"


#define CONFIG_PATH                 "config/"
#define ERASE_CONFIG_FILE           CONFIG_PATH "erase_config.txt"
#define FLUSH_CONFIG_FILE           CONFIG_PATH "flush_config.txt"
#define REBOOT_CONFIG_FILE          CONFIG_PATH "reboot_config.txt"
#define GET_SN_CMD_FILE             CONFIG_PATH "get_sn_log_cmd.txt"

using namespace std;

typedef struct {
    string usb_sn;
    specify_interface_t si;
    uint32_t speed;
    string dev_type;
    string load_file;
} jlink_flush_config_file_args_t;

class JlinkFlushConfigFileArgs
{

public:

    JlinkFlushConfigFileArgs();
    ~JlinkFlushConfigFileArgs();

    string erase_file = ERASE_CONFIG_FILE;
    string flush_file = FLUSH_CONFIG_FILE;
    string reboot_file= REBOOT_CONFIG_FILE;

    void generate_erase_config_file(jlink_flush_config_file_args_t *p);
    void generate_flush_config_file(jlink_flush_config_file_args_t *p);
    void generate_reboot_config_file(jlink_flush_config_file_args_t *p);

    void generate_get_sn_config_file(void);
};

#endif // HJ_FILE_WRITE_H
