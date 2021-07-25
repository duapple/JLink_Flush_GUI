#ifndef UTILS_H
#define UTILS_H

#include <iostream>

using namespace std;

bool is_file_exit(string fileName);
long get_file_size(string fileName);
std::string get_file_path(std::string filename);
std::string get_file_format(std::string path);
int get_bin_file_start_addr(std::string filename, std::string &start_addr, std::string &end_addr);
int format_bin_to_hex(std::string &filename);
int bin_to_hex(std::string filename, string &offset);

#endif // UTILS_H
