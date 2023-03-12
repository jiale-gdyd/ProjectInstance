#pragma once

#include <vector>

namespace axpi {
template <typename T>
class SimpleRingBuffer {
public:
    SimpleRingBuffer() {

    }

    SimpleRingBuffer(int size){
        this->resize(size);
    }

    ~SimpleRingBuffer() {
        // mBuffer.swap(std::vector<T>());
    }

    void resize(int size) {
        mBuffer.resize(size);
    }

    T &next() {
        mCurIdx = (mCurIdx + 1) % mBuffer.size();
        return get(mCurIdx++ % mBuffer.size());
    }

    T &get(int idx) {
        return mBuffer[idx];
    }
    
    size_t size() {
        return mBuffer.size();
    }

private:
    std::vector<T> mBuffer;
    int            mCurIdx = 0;
};
}
