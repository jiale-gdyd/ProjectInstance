#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>

namespace axpi {
bool file_exist(const std::string &path)
{
    auto flag = false;

    std::fstream fs(path, std::ios::in | std::ios::binary);
    flag = fs.is_open();
    fs.close();

    return flag;
}

bool read_file(const std::string &path, std::vector<char> &data)
{
    std::fstream fs(path, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        return false;
    }

    fs.seekg(std::ios::end);
    auto fs_end = fs.tellg();
    fs.seekg(std::ios::beg);
    auto fs_beg = fs.tellg();

    auto file_size = static_cast<size_t>(fs_end - fs_beg);
    auto vector_size = data.size();

    data.reserve(vector_size + file_size);
    data.insert(data.end(), std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());

    fs.close();
    return true;
}

bool dump_file(const std::string &path, std::vector<char> &data)
{
    std::fstream fs(path, std::ios::out | std::ios::binary);
    if (!fs.is_open() || fs.fail()) {
        return false;
    }

    fs.write(data.data(), data.size());
    return true;
}

bool dump_file(const std::string &path, char *data, int size)
{
    std::fstream fs(path, std::ios::out | std::ios::binary);
    if (!fs.is_open() || fs.fail()) {
        return false;
    }

    fs.write(data, size);
    return true;
}
}
