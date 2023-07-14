#ifndef TBOX_TERMINAL_IMPL_SESSION_CONTEXT_H
#define TBOX_TERMINAL_IMPL_SESSION_CONTEXT_H

#include <deque>

#include "key_event_scanner.h"
#include "../session.h"

namespace tbox {
namespace terminal {

struct SessionContext {
    Connection *wp_conn = nullptr;
    SessionToken token;

    uint32_t options = 0;

    std::string curr_input;
    size_t cursor = 0;

    Path path;  //! 当前路径
    std::deque<std::string> history;   //! 历史命令
    size_t history_index = 0;   //! 0表示不指定历史命令

    KeyEventScanner key_event_scanner_;

    uint16_t window_width = 0;
    uint16_t window_height = 0;
};

}
}

#endif //TBOX_TERMINAL_SESSION_IMP_H_20220204
