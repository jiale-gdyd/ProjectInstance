#ifndef TBOX_TERMINAL_TYPES_H
#define TBOX_TERMINAL_TYPES_H

#include <vector>
#include <string>
#include <functional>
#include "../base/cabinet_token.h"

namespace tbox {
namespace terminal {

class Session;

using SessionToken = cabinet::Token;
using NodeToken    = cabinet::Token;

using Args = std::vector<std::string>;
using Func = std::function<void (const Session &s, const Args &)>;

}
}

#endif //TBOX_TERMINAL_TYPES_H_20220128
