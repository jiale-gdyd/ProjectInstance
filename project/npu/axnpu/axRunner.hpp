#ifndef NPU_AXNPU_AXRUNNER_HPP
#define NPU_AXNPU_AXRUNNER_HPP

#include "runnerBase.hpp"

API_BEGIN_NAMESPACE(Ai)

struct AxJointRunnerHandler;

class AxRunner : public AxRunnerBase {
public:
    virtual ~AxRunner();

public:
    int init(std::string model) override;
    void deinit() override;

    int getColorSpace() override;
    int getModelWidth() override;
    int getModelHeight() override;

    int forward(axframe_t *mediaFrame, const axbbox_t *cropReizeBBox) override;

protected:
    struct AxJointRunnerHandler *mHandler;
};

API_END_NAMESPACE(Ai)

#endif
