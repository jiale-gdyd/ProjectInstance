#ifndef AXERA_MEDIA_ISP_HPP
#define AXERA_MEDIA_ISP_HPP

#if !defined(__AXERA_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <vector>
#include <stdint.h>
#include <stdbool.h>
#include <utils/export.h>

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaIsp {
public:
    MediaIsp();
    ~MediaIsp();

    // 删除了复制构造函数，删除了移动构造函数
    MediaIsp(const MediaIsp &other) = delete;
    MediaIsp(MediaIsp &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaIsp &operator=(const MediaIsp &other) = delete;
    MediaIsp &operator=(MediaIsp &&other) = delete;
};

API_END_NAMESPACE(media)

#endif
