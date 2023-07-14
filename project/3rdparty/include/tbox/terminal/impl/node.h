#ifndef TBOX_TERMINAL_IMPL_NODE_H
#define TBOX_TERMINAL_IMPL_NODE_H

#include "inner_types.h"

namespace tbox {
namespace terminal {

class Node {
  public:
    explicit Node(const std::string &help) : help_(help) { }
    virtual ~Node() { }

    virtual NodeType type() const = 0;
    std::string help() const { return help_; }

  private:
    std::string help_;
};

}
}

#endif //TBOX_TERMINAL_NODE_H_20220207
