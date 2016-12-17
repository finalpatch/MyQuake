#pragma once

extern "C"
{
#include "quakedef.h"
}

inline std::vector<uint8_t> readDataFile(const std::string& filename)
{
    // try open file and get length
    int handle;
    auto len = COM_OpenFile (filename.c_str(), &handle);
    if (handle == -1)
        return {};
    
    // create buffer
    std::vector<uint8_t> buf(len);

    // read into buffer
    Sys_FileRead (handle, buf.data(), len);
    COM_CloseFile(handle);
    
    return buf;
}

inline std::string readTextFile(const std::string& filename)
{
    auto buf = readDataFile(filename);
    return std::string(reinterpret_cast<const char*>(buf.data()), buf.size());
}
