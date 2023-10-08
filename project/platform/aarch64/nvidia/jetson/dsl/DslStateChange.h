#ifndef DSL_DSL_STATE_CHANGE_H
#define DSL_DSL_STATE_CHANGE_H

#include "Dsl.h"

namespace DSL {
/* 将状态更改定义为"以前和新的"状态 */
struct DslStateChange {
    DslStateChange(GstState previousState, GstState newState)
        : m_previousState(previousState), m_newState(newState)
    {
        LOG_FUNC();
    }

    ~DslStateChange()
    {
        LOG_FUNC();
    };

    GstState m_previousState;       // 定义前一个状态值
    GstState m_newState;            // 定义新的状态值
};
}

#endif
