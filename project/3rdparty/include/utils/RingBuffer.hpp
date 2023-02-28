#ifndef UTILS_RINGBUFFER_HPP
#define UTILS_RINGBUFFER_HPP

#include <time.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>

#include <limits>
#include <atomic>
#include <string>

#include "export.h"

API_BEGIN_NAMESPACE(utils)

/**
 * 无锁，不浪费插槽ringbuffer实现
 *
 * @param T              缓冲元素的类型
 * @param buffer_size    缓冲区的大小。必须是2的幂
 * @param fake_tso       省略生成显式屏障代码以避免tso场景中不必要的指令(例如简单的微控制器/单核)
 * @param cacheline_size 缓存行的大小，在索引和缓冲区之间插入适当的填充
 * @param index_t        数组索引类型的类型。也用作未来实施的占位符
 */
template<typename T, size_t buffer_size = 16, bool fake_tso = false, size_t cacheline_size = 0, typename index_t = size_t>
class RingBuffer {
public:
    // 默认构造函数，将初始化头尾索引
    RingBuffer() : head(0), tail(0) {

    }

    /**
     * 当在.bss部分中实例化对象时，特例构造函数会提前删除不必要的初始化代码
     * @warning     如果对象在堆栈、堆或noinit部分内实例化，则必须在使用前明确清除内容
     * @param dummy 忽略
     */
    RingBuffer(int dummy) {
        (void)(dummy);
    }

    /**
     * 从生产者端清除缓冲区
     * @warning 如果消费者试图同时读取数据，函数可能会返回而不执行任何操作
     */
    void producerClear(void) {
        // 如果在消费者读取期间清除头部修改将导致下溢，如果不修改消费者代码，则无法使用CAS正确执行此操作
        consumerClear();
    }

    // 从消费者端清除缓冲区
    void consumerClear() {
        tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    /**
     * 检查缓冲区是否为空
     * @return 如果缓冲区为空，则为true
     */
    bool isEmpty(void) const {
        return (readAvailable() == 0);
    }

    bool empty(void) const {
        return isEmpty();
    }

    /**
     * 检查缓冲区是否已满
     * @return 如果缓冲区已满，则为true
     */
    bool isFull(void) const {
        return (writeAvailable() == 0);
    }

    bool full(void) const {
        return isFull();
    }

    /**
     * 检查可以从缓冲区中读取多少元素
     * @return 可读取的元素数
     */
    index_t readAvailable(void) const {
        return (head.load(index_acquire_barrier) - tail.load(std::memory_order_relaxed));
    }

    /**
     * 检查有多少元素可以写入缓冲区
     * @return 可写入的空闲槽数
     */
    index_t writeAvailable(void) const {
        return (buffer_size - (head.load(std::memory_order_relaxed) - tail.load(index_acquire_barrier)));
    }

    /**
     * 将数据插入内部缓冲区，不阻塞
     * @param data 要插入内部缓冲区的元素
     * @return     如果插入数据则为true
     */
    bool insert(T data) {
        index_t tmp_head = head.load(std::memory_order_relaxed);
        if ((tmp_head - tail.load(index_acquire_barrier)) == buffer_size) {
            return false;
        } else {
            data_buff[tmp_head++ & buffer_mask] = data;
            std::atomic_signal_fence(std::memory_order_release);
            head.store(tmp_head, index_release_barrier);
        }

        return true;
    }

    bool put(T data) {
        return insert(data);
    }

    /**
     * 将数据插入内部缓冲区，不阻塞
     * @param[in] data 指向要插入内部缓冲区的元素所在的内存位置的指针
     * @return         如果插入数据则为true
     */
    bool insert(const T *data) {
        index_t tmp_head = head.load(std::memory_order_relaxed);
        if ((tmp_head - tail.load(index_acquire_barrier)) == buffer_size) {
            return false;
        } else {
            data_buff[tmp_head++ & buffer_mask] = *data;
            std::atomic_signal_fence(std::memory_order_release);
            head.store(tmp_head, index_release_barrier);
        }

        return true;
    }

    bool put(const T *data) {
        return insert(data);
    }

    /**
     * 将回调函数返回的数据插入内部缓冲区，不阻塞
     * 这是一个特殊用途的功能，可用于避免冗余可用性检查，以防在获取数据时产生副作用(如通过读取外设数据寄存器清除状态标志)
     *
     * @param get_data_callback 指向回调函数的指针，该函数返回要插入缓冲区的元素
     * @return                  如果插入了数据并调用了回调，则为true
     */
    bool insertFromCallbackWhenAvailable(T (*get_data_callback)(void)) {
        index_t tmp_head = head.load(std::memory_order_relaxed);
        if ((tmp_head - tail.load(index_acquire_barrier)) == buffer_size) {
            return false;
        } else {
            // 仅当缓冲区中有空间时才执行回调
            data_buff[tmp_head++ & buffer_mask] = get_data_callback();
            std::atomic_signal_fence(std::memory_order_release);
            head.store(tmp_head, index_release_barrier);
        }

        return true;
    }

    /**
     * 删除单个元素而不读取
     * @return 如果删除了一个元素，则为true
     */
    bool remove() {
        index_t tmp_tail = tail.load(std::memory_order_relaxed);
        if (tmp_tail == head.load(std::memory_order_relaxed)) {
            return false;
        } else {
            // 在数据之前加载/使用的情况下释放
            tail.store(++tmp_tail, index_release_barrier);
        }

        return true;
    }

    bool get() {
        return remove();
    }

    /**
     * 删除多个元素而不读取并将其存储在其他地方
     * @param cnt 要删除的最大元素数
     * @return    移除元素的数量
     */
    size_t removeCnt(size_t cnt) {
        index_t tmp_tail = tail.load(std::memory_order_relaxed);
        index_t avail = head.load(std::memory_order_relaxed) - tmp_tail;

        cnt = (cnt > avail) ? avail : cnt;
        tail.store(tmp_tail + cnt, index_release_barrier);

        return cnt;
    }

    /**
     * 从内部缓冲区读取一个元素而不阻塞
     * @param[out] data 引用将存储已删除元素的内存位置
     * @return          如果数据是从内部缓冲区获取的，则为true
     */
    bool remove(T &data) {
        // 无论如何引用都实现为指针
        return remove(&data);
    }

    bool get(T &data) {
        return remove(data);
    }

    /**
     * 从内部缓冲区读取一个元素而不阻塞
     * @param[out] data 指向将存储已删除元素的内存位置的指针
     * @return          如果数据是从内部缓冲区获取的，则为true
     */
    bool remove(T *data) {
        index_t tmp_tail = tail.load(std::memory_order_relaxed);
        if (tmp_tail == head.load(index_acquire_barrier)) {
            return false;
        } else {
            *data = data_buff[tmp_tail++ & buffer_mask];
            std::atomic_signal_fence(std::memory_order_release);
            tail.store(tmp_tail, index_release_barrier);
        }

        return true;
    }

    bool get(T *data) {
        return remove(data);
    }

    /**
     * 获取消费端缓冲区中的第一个元素。仅在消费者端使用和修改项目内容是安全的
     * @return 指向第一个元素的指针，如果缓冲区为空，则为nullptr
     */
    T *peek() {
        index_t tmp_tail = tail.load(std::memory_order_relaxed);
        if (tmp_tail == head.load(index_acquire_barrier)) {
            return nullptr;
        } else {
            return &data_buff[tmp_tail & buffer_mask];
        }
    }

    /**
     * 获取消费端的第n个元素。仅在消费者端使用和修改项目内容是安全的
     *
     * @param index 从消费端开始的项目偏移量
     * @return      指向请求元素的指针，如果索引超过存储计数，则为nullptr
     */
    T *at(size_t index) {
        index_t tmp_tail = tail.load(std::memory_order_relaxed);
        if ((head.load(index_acquire_barrier) - tmp_tail) <= index) {
            return nullptr;
        } else {
            return &data_buff[(tmp_tail + index) & buffer_mask];
        }
    }

    /**
     * 获取消费端的第n个元素。未经检查的操作，假设软件已经知道该元素是否可以使用，如果请求的索引超出范围，
     * 则引用将指向缓冲区内的某个位置isEmpty()和readAvailable()将放置适当的内存屏障，
     * 如果用作循环限制器它仅在消费者端可以安全使用和修改T内容
     *
     * @param index 从消费端开始的项目偏移量
     * @return      对请求元素的引用，如果索引超过存储计数，则未定义
     */
    T &operator[](size_t index) {
        return data_buff[(tail.load(std::memory_order_relaxed) + index) & buffer_mask];
    }

    /**
     * 将多个元素插入内部缓冲区而不会阻塞。此函数将从给定缓冲区插入尽可能多的数据
     *
     * @param[in] buff  指向缓冲区的指针，其中包含要插入的数据
     * @param     count 从给定缓冲区写入的元素数
     * @return          写入内部缓冲区的元素数
     */
    size_t writeBuff(const T *buff, size_t count) {
        index_t available = 0;
        size_t to_write = count;
        index_t tmp_head = head.load(std::memory_order_relaxed);

        available = buffer_size - (tmp_head - tail.load(index_acquire_barrier));
        if (available < count) {
            to_write = available;
        }

        // 也许将它分成2个单独的写入
        for (size_t i = 0; i < to_write; i++) {
            data_buff[tmp_head++ & buffer_mask] = buff[i];
        }

        std::atomic_signal_fence(std::memory_order_release);
        head.store(tmp_head, index_release_barrier);

        return to_write;
    }

    /**
     * 将多个元素插入内部缓冲区而不会阻塞。此函数将继续写入新条目，直到写入所有数据或没有更多空间。
     * 回调函数可用于向消费者指示它可以开始获取数据
     *
     * @warning                     这个函数不是确定性的
     *
     * @param[in] buff              指向缓冲区的指针，其中包含要插入的数据
     * @param count                 从给定缓冲区写入的元素数
     * @param count_to_callback     在第一个循环中调用回调函数之前要写入的元素数
     * @param execute_data_callback 指向每次循环迭代后执行的回调函数的指针
     * @return                      写入内部缓冲区的元素数
     */
    size_t writeBuff(const T *buff, size_t count, size_t count_to_callback, void (*execute_data_callback)(void)) {
        size_t written = 0;
        index_t available = 0;
        size_t to_write = count;
        index_t tmp_head = head.load(std::memory_order_relaxed);

        if ((count_to_callback != 0) && (count_to_callback < count)) {
            to_write = count_to_callback;
        }

        while (written < count) {
            available = buffer_size - (tmp_head - tail.load(index_acquire_barrier));
            if (available == 0) {
                break;
            }

            if (to_write > available) {
                to_write = available;
            }

            while (to_write--) {
                data_buff[tmp_head++ & buffer_mask] = buff[written++];
            }

            std::atomic_signal_fence(std::memory_order_release);
            head.store(tmp_head, index_release_barrier);

            if (execute_data_callback != nullptr) {
                execute_data_callback();
            }

            to_write = count - written;
        }

        return written;
    }

    /**
     * 从内部缓冲区加载多个元素而不会阻塞。此函数将读取指定数量的数据
     *
     * @param[out] buff  指向将加载数据的缓冲区的指针
     * @param      count 要加载到给定缓冲区中的元素数
     * @return           从内部缓冲区读取的元素数
     */
    size_t readBuff(T *buff, size_t count) {
        index_t available = 0;
        size_t to_read = count;
        index_t tmp_tail = tail.load(std::memory_order_relaxed);

        available = head.load(index_acquire_barrier) - tmp_tail;
        if (available < count) {
            to_read = available;
        }

        // 也许将它分成2个单独的读取
        for (size_t i = 0; i < to_read; i++) {
            buff[i] = data_buff[tmp_tail++ & buffer_mask];
        }

        std::atomic_signal_fence(std::memory_order_release);
        tail.store(tmp_tail, index_release_barrier);

        return to_read;
    }

    /**
     * 从内部缓冲区加载多个元素而不会阻塞。此函数将继续读取新条目，直到读取所有请求的数据或没有更多内容可读取。
     * 回调函数可用于向生产者指示它可以开始写入新数据。
     *
     * @warning                     这个函数不是确定性的
     *
     * @param[out] buff             指向将加载数据的缓冲区的指针
     * @param count                 要加载到给定缓冲区中的元素数
     * @param count_to_callback     在第一次迭代中调用回调函数之前要加载的元素数
     * @param execute_data_callback 指向每次循环迭代后执行的回调函数的指针
     * @return                      从内部缓冲区读取的元素数
     */
    size_t readBuff(T *buff, size_t count, size_t count_to_callback, void (*execute_data_callback)(void)) {
        size_t read = 0;
        index_t available = 0;
        size_t to_read = count;
        index_t tmp_tail = tail.load(std::memory_order_relaxed);

        if ((count_to_callback != 0) && (count_to_callback < count)) {
            to_read = count_to_callback;
        }

        while (read < count) {
            available = head.load(index_acquire_barrier) - tmp_tail;
            if (available == 0) {
                break;
            }

            if (to_read > available) {
                to_read = available;
            }

            while (to_read--) {
                buff[read++] = data_buff[tmp_tail++ & buffer_mask];
            }

            std::atomic_signal_fence(std::memory_order_release);
            tail.store(tmp_tail, index_release_barrier);

            if (execute_data_callback != nullptr) {
                execute_data_callback();
            }

            to_read = count - read;
        }

        return read;
    }

private:
    // 给定缓冲区大小的按位掩码
    constexpr static index_t buffer_mask = buffer_size - 1;

    // 在对方确认之前不要从缓冲区加载或存储到缓冲区
    constexpr static std::memory_order index_acquire_barrier = fake_tso ? std::memory_order_relaxed : std::memory_order_acquire;

    // 在对data_buf的所有操作提交之前不要更新自己的一方
    constexpr static std::memory_order index_release_barrier = fake_tso ? std::memory_order_relaxed : std::memory_order_release;

    // 头索引
    alignas(cacheline_size) std::atomic<index_t> head;

    // 尾索引
    alignas(cacheline_size) std::atomic<index_t> tail;

    // 将缓冲区放在变量之后，以便可以通过短偏移量达到所有内容
    alignas(cacheline_size) T data_buff[buffer_size];

    // 让我们断言不会编译任何UB
    static_assert((buffer_size != 0), "buffer cannot be of zero size");
    static_assert((buffer_size & buffer_mask) == 0, "buffer size is not a power of 2");
    static_assert(sizeof(index_t) <= sizeof(size_t), "indexing type size is larger than size_t, operation is not lock free and doesn't make sense");

    static_assert(std::numeric_limits<index_t>::is_integer, "indexing type is not integral type");
    static_assert(!(std::numeric_limits<index_t>::is_signed), "indexing type shall not be signed");
    static_assert(buffer_mask <= ((std::numeric_limits<index_t>::max)() >> 1), "buffer size is too large for a given indexing type (maximum size for n-bit type is 2^(n-1))");
};

API_END_NAMESPACE(utils)

#endif
