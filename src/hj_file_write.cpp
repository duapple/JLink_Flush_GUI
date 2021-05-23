#include "hj_file_write.h"
#include "utils/utils.h"
#include <fstream>

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
    flush_config_file_s << "r \n" << "h \n" << "erase \n";
    flush_config_file_s << "loadfile " << p->load_file << endl;
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
