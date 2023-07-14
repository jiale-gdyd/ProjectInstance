#ifndef TBOX_BASE_DEFINES_H
#define TBOX_BASE_DEFINES_H

//! 获取固定数组成员个数
#define NUMBER_OF_ARRAY(arr)    (sizeof(arr) / sizeof(*arr))

//! 资源释放相关宏
#define CHECK_DELETE_RESET_OBJ(obj) do { if (obj != nullptr) { delete obj; obj = nullptr; } } while (0)
#define CHECK_DELETE_OBJ(obj)   do { if (obj != nullptr) { delete obj; } } while (0)
#define CHECK_DELETE_RESET_ARRAY(arr)   do { if (arr != nullptr) { delete [] arr; arr = nullptr; } } while (0)
#define CHECK_DELETE_ARRAY(arr) do { if (arr != nullptr) { delete [] arr; } } while (0)
#define CHECK_FREE_RESET_PTR(ptr)   do { if (ptr != nullptr) { free(ptr); ptr = nullptr; } } while (0)
#define CHECK_FREE_PTR(ptr) do { if (ptr != nullptr) { free(ptr); } } while (0)
#define CHECK_CLOSE_RESET_FD(fd)    do { if (fd != -1) { close(fd); fd = -1; } } while (0)
#define CHECK_CLOSE_FD(fd)  do { if (fd != -1) { close(fd); } } while (0)

//! 在类中禁用复制特性
#define NONCOPYABLE(class_name) \
    class_name(const class_name&) = delete; \
    class_name& operator = (const class_name &) = delete

//! 在类中禁用移动特性
#define IMMOVABLE(class_name) \
    class_name(class_name &&) = delete; \
    class_name& operator = (class_name &&) = delete

//! 基于无参构造、析构、swap，实现reset与移动函数
#define IMP_MOVE_RESET_FUNC_BASE_ON_SWAP(class_name) \
    class_name::class_name(class_name&& other) \
    { \
        swap(other); \
    } \
    class_name& class_name::operator = (class_name&& other) \
    { \
        if (this != &other) { \
            reset(); \
            swap(other); \
        } \
        return *this; \
    } \
    void class_name::reset() \
    { \
        class_name tmp; \
        swap(tmp); \
    }

//! 条件预加载宏
#ifndef LIKELY
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#endif

#ifndef UNLIKELY
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#endif //TBOX_BASE_DEFINES_H_20171030
