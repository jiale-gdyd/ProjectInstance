#ifndef TBOX_TERMINAL_CONNECTION_H
#define TBOX_TERMINAL_CONNECTION_H

#include "types.h"

namespace tbox {
namespace terminal {

class Connection {
  public:
    virtual bool send(const SessionToken &st, char ch) = 0;
    virtual bool send(const SessionToken &st, const std::string &str) = 0;
    virtual bool endSession(const SessionToken &st) = 0;
    virtual bool isValid(const SessionToken &st) const = 0;

  protected:
    virtual ~Connection() { }
};

}
}

#endif //TBOX_TERMINAL_CONNECTION_H_20220214
