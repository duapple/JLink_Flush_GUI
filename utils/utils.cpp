#include "utils.h"
#include <iostream>
#include <sys/stat.h>
#include <QDebug>
#include <QProcess>
#include <stdio.h>

#define SREC_INFO_EXEC       "./bin/srec_info.exe"
#define SREC_CAT_EXEC        "./bin/srec_cat.exe"

using namespace std;

bool is_file_exit(string fileName)
{
    struct stat buffer;
    return (stat (fileName.c_str(), &buffer) == 0);
}

long get_file_size(string fileName)
{
    struct stat info;
    if (stat(fileName.c_str(), &info)) {
        return -1;
    }

    return info.st_size;
}

std::string get_file_path(std::string filename)
{
    string directory;
    const size_t last_slash_idx = filename.rfind('/');
    if (std::string::npos != last_slash_idx)
    {
        directory = filename.substr(0, last_slash_idx);
    }

    qDebug() << directory.c_str();

    return directory;
}

std::string get_file_format(std::string filename)
{
    if (filename.find_last_of('.') >= filename.size())
    {
        return "";
    }

    std::string s = filename.substr(filename.find_last_of('.'));
    qDebug() << "file format: " << s.c_str();

    return s;
}

int format_bin_to_hex(std::string &filename)
{
    unsigned long long pos = filename.find_last_of(".bin");
    if ( pos >= filename.size())
    {
        return -1;
    }

    filename = filename.replace(filename.find(".bin"), 4, ".hex");
    return 0;
}

int bin_to_hex(std::string filename, string &offset)
{
    string filename_tmp = filename;
    format_bin_to_hex(filename_tmp);
    // qDebug() << "filename_tmp: " << filename_tmp.c_str();
    string offset_tmp = "0x" + offset;
    QObject *parent=NULL;
    QString program = SREC_CAT_EXEC;
    QStringList arguments;
    arguments << QString::fromStdString(filename) << "-Binary" << "-offset" << QString::fromStdString("0x" + offset) << "-o" << QString::fromStdString(filename_tmp) << "-Intel" ;

    qInfo() << "arguments: " << arguments;

    QProcess *myProcess = new QProcess(parent);
    myProcess->start(program, arguments);
    myProcess->waitForStarted();
    myProcess->waitForFinished();

    qInfo() << QString::fromLocal8Bit(myProcess->readAllStandardOutput());
    return 0;
}

int get_bin_file_start_addr(std::string filename, std::string &start_addr, std::string &end_addr)
{
    QObject *parent=NULL;
    QString program = SREC_INFO_EXEC;
    QStringList arguments;
    arguments << QString::fromStdString(filename) << "-Intel" ;

    QProcess *myProcess = new QProcess(parent);
    myProcess->start(program, arguments);
    myProcess->waitForStarted();
    myProcess->waitForFinished();

    string out = QString::fromLocal8Bit(myProcess->readAllStandardOutput()).toStdString();
    qDebug() << "out: \r\n" << out.c_str();

    string data;
    if (out.find("Data:   ") < out.size())
    {
        data = out.substr(out.find("Data:   "));
    } else {
        data = "";
    }

    if (data == "") {
        qCritical() << "HEX文件错误，请检查HEX文件是否是标准HEX文件";
        return -1;
    }
    qDebug() << "data: \r\n" << data.c_str();

    char start_addr_str[32] = {0};
    char end_addr_str[32] = {0};
    sscanf(data.c_str() + strlen("Data:   "), "%s%*[^-]-%s", start_addr_str, end_addr_str);

    qDebug() << "start addr: " << start_addr_str << ",  end_addr: " << end_addr_str;
    start_addr = start_addr_str;
    end_addr = end_addr_str;

    return 0;
}
