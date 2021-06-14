#include "hj_file_write.h"
#include "utils/utils.h"
#include <fstream>
#include <QDebug>
#include <QFileInfo>

JlinkFlushConfigFileArgs::JlinkFlushConfigFileArgs()
{

}

JlinkFlushConfigFileArgs::~JlinkFlushConfigFileArgs()
{

}

void JlinkFlushConfigFileArgs::generate_erase_config_file(jlink_flush_config_file_args_t *p)
{
    using namespace std;
    ofstream erase_config_file_s;
    erase_config_file_s.open(erase_file);

    erase_config_file_s << "usb " << p->usb_sn << endl;
    erase_config_file_s << "si " << p->si << endl;
    erase_config_file_s << "speed " << p->speed << endl;
    erase_config_file_s << "device " << p->dev_type << endl;
    erase_config_file_s << "r \n" << "h \n" << "erase \n" << "q \n";
    erase_config_file_s.close();
}

void JlinkFlushConfigFileArgs::generate_flush_config_file(jlink_flush_config_file_args_t *p)
{
    using namespace std;
    ofstream flush_config_file_s;
    flush_config_file_s.open(flush_file);

    flush_config_file_s << "usb " << p->usb_sn << endl;
    flush_config_file_s << "si " << p->si << endl;
    flush_config_file_s << "speed " << p->speed << endl;
    flush_config_file_s << "device " << p->dev_type << endl;
//    flush_config_file_s << "r \n" << "h \n" << "erase \n";
    flush_config_file_s << "r \n" << "h \n";
    if (p->erase_chip) {
        flush_config_file_s << "erase \n";
    }

    if (p->load_file != "")
    {
        string load_file1 = p->load_file;
        if (get_file_format(load_file1) == ".hex") {
            flush_config_file_s << "loadfile \"" << load_file1 << "\"" << endl;
        } else {
            format_bin_to_hex(load_file1);
            flush_config_file_s << "loadfile \"" << load_file1 << "\"" << endl;
            qDebug() << "load_file name: " << load_file1.c_str();
            bin_to_hex(p->load_file, p->file_start_addr);
        }
    } else {
        qDebug() << "firmware 不存在";
    }

    if (p->load_file2 != "")
    {
        string load_file2 = p->load_file2;
        if (get_file_format(load_file2) == ".hex") {
            flush_config_file_s << "loadfile \"" << load_file2 << "\"" << endl;
        } else {
            format_bin_to_hex(load_file2);
            flush_config_file_s << "loadfile \"" << load_file2 << "\"" << endl;
            qDebug() << "load_file2 name: " << load_file2.c_str();
            bin_to_hex(p->load_file2, p->file2_start_addr);
        }
    } else {
        qDebug() << "firmware2 不存在";
    }

    if (p->load_file3 != "")
    {
        string load_file3 = p->load_file3;
        if (get_file_format(load_file3) == ".hex") {
            flush_config_file_s << "loadfile \"" << load_file3 << "\"" << endl;
        } else {
            format_bin_to_hex(load_file3);
            flush_config_file_s << "loadfile \"" << load_file3 << "\"" << endl;
            qDebug() << "load_file3 name: " << load_file3.c_str();
            bin_to_hex(p->load_file3, p->file3_start_addr);
        }
    } else {
        qDebug() << "firmware3 不存在";
    }

    flush_config_file_s << "r \n" << "q \n";
    flush_config_file_s.close();
}

void JlinkFlushConfigFileArgs::generate_reboot_config_file(jlink_flush_config_file_args_t *p)
{
    using namespace std;
    ofstream reboot_config_file_s;
    reboot_config_file_s.open(REBOOT_CONFIG_FILE);

    reboot_config_file_s << "usb " << p->usb_sn << endl;
    reboot_config_file_s << "si " << p->si << endl;
    reboot_config_file_s << "speed " << p->speed << endl;
    reboot_config_file_s << "device " << p->dev_type << endl;
    reboot_config_file_s << "r \n" << "q \n";
    reboot_config_file_s.close();
}

void JlinkFlushConfigFileArgs::generate_get_sn_config_file()
{
    using namespace std;
    ofstream reboot_config_file_s;
    reboot_config_file_s.open(GET_SN_CMD_FILE);

    reboot_config_file_s << "showemulist\nq\n";
    reboot_config_file_s.close();
}
