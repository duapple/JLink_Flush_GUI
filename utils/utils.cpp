#include "utils.h"
#include <iostream>
#include <sys/stat.h>

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
