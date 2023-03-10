#include <iomanip>
#include <sstream>
#include <iostream>

#include "Timestamp.h"

namespace xop {
std::string Timestamp::Localtime()
{
    std::ostringstream stream;
    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);

    char buffer[200] = {0};
    std::string timeString;
    std::strftime(buffer, 200, "%F %T", std::localtime(&tt));
    stream << buffer;

    return stream.str();
}
}
