#ifndef AX620A_FUNCIMPL_EMS_DEMO_HPP
#define AX620A_FUNCIMPL_EMS_DEMO_HPP

#include <vector>
#include <memory>
#include <string.h>
#include <unistd.h>
#include <mpi/axmpi/mediaBase.hpp>

API_BEGIN_NAMESPACE(EMS)

class EMSDemoImpl : public media::MediaBase {
public:
    EMSDemoImpl();
    ~EMSDemoImpl();

    virtual int init() override;

private:
    int mediaInit();
    void mediaDeinit();

private:
    bool                         mThreadFin;        // 线程退出标志
    std::vector<media::axpipe_t> mPipelines;        // pipeline信息
};

API_END_NAMESPACE(EMS)

#endif
