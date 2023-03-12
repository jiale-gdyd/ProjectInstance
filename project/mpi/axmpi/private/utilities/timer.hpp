#pragma once

#include <chrono>

namespace axpi {
class Timer {
public:
    Timer() {
        start();
    }

    void start() {
        stop();
        this->mStartTime = this->mEndTime;
    }

    void stop() {
        this->mEndTime = std::chrono::high_resolution_clock::now();
    }

    float cost() {
        if (this->mEndTime <= this->mStartTime) {
            this->stop();
        }

        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(this->mEndTime - this->mStartTime).count();
        return static_cast<float>(ms) / 1000.f;
    }

private:
    std::chrono::system_clock::time_point mStartTime;
    std::chrono::system_clock::time_point mEndTime;
};
}