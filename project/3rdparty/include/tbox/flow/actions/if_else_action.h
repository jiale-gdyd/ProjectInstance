#ifndef TBOX_FLOW_ACTIONS_IF_ELSE_H
#define TBOX_FLOW_ACTIONS_IF_ELSE_H

#include "../action.h"

namespace tbox {
namespace flow {

/**
 * bool IfElseAction(if_action, succ_action, fail_acton) {
 *   if (if_action())
 *     return succ_action();
 *   else
 *     return fail_acton();
 * }
 */
class IfElseAction : public Action {
  public:
    explicit IfElseAction(event::Loop &loop, Action *if_action,
                          Action *succ_action, Action *fail_action);
    virtual ~IfElseAction();

    virtual void toJson(Json &js) const;

  protected:
    virtual bool onStart() override;
    virtual bool onStop() override;
    virtual bool onPause() override;
    virtual bool onResume() override;
    virtual void onReset() override;

  protected:
    void onCondActionFinished(bool is_succ);

  private:
    Action *if_action_;
    Action *succ_action_;
    Action *fail_action_;
};

}
}

#endif //TBOX_FLOW_IF_ELSE_H_20221022
