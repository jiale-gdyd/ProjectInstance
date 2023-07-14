#ifndef TBOX_FLOW_ACTIONS_FUNCTION_ACTION_H
#define TBOX_FLOW_ACTIONS_FUNCTION_ACTION_H

#include "../action.h"

namespace tbox {
namespace flow {

class FunctionAction : public Action {
  public:
    using Func = std::function<bool()>;
    explicit FunctionAction(event::Loop &loop, const Func &func);

  protected:
    virtual bool onStart() override;

  private:
    Func func_;
};

}
}

#endif //TBOX_FLOW_FUNCTION_ACTION_H_20221003
