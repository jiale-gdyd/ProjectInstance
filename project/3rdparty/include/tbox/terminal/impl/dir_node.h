#ifndef TBOX_TERMINAL_IMPL_DIR_NODE_H
#define TBOX_TERMINAL_IMPL_DIR_NODE_H

#include "node.h"
#include <map>

namespace tbox {
namespace terminal {

class DirNode : public Node {
  public:
    using Node::Node;

    virtual NodeType type() const override { return NodeType::kDir; }

    bool addChild(const NodeToken &nt, const std::string &child_name);
    NodeToken findChild(const std::string &child_name) const;
    void children(std::vector<NodeInfo> &vec) const;

  private:
    std::map<std::string, NodeToken> children_;
};

}
}

#endif //TBOX_TERMINAL_DIR_NODE_H_20220207
