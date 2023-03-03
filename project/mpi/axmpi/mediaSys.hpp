#ifndef AXERA_MEDIA_SYS_HPP
#define AXERA_MEDIA_SYS_HPP

#if !defined(__AXERA_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <vector>
#include <stdint.h>
#include <stdbool.h>
#include <utils/export.h>

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaSys {
public:
    MediaSys();
    ~MediaSys();

    // 删除了复制构造函数，删除了移动构造函数
    MediaSys(const MediaSys &other) = delete;
    MediaSys(MediaSys &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaSys &operator=(const MediaSys &other) = delete;
    MediaSys &operator=(MediaSys &&other) = delete;

    int init();
    void deinit();
};

API_END_NAMESPACE(media)

#endif
