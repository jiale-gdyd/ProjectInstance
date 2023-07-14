#include <tbox/http/respond.h>
#include <sstream>

namespace tbox {
namespace http {

bool Respond::isValid() const
{
    return status_code != StatusCode::kUnset && http_ver != HttpVer::kUnset;
}

std::string Respond::toString() const
{
    std::ostringstream oss;
    oss << HttpVerToString(http_ver) << " " << StatusCodeToString(status_code) << CRLF;
    for (auto &head : headers)
        oss << head.first << ": " << head.second << CRLF;
    oss << "Content-Length: " << body.length() << CRLF;
    oss << CRLF;
    oss << body;

    return oss.str();
}

}
}

