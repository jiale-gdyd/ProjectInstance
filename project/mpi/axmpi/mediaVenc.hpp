#ifndef AXERA_MEDIA_VENC_HPP
#define AXERA_MEDIA_VENC_HPP

#if !defined(__AXERA_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <vector>
#include <stdint.h>
#include <stdbool.h>
#include <utils/export.h>

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaVenc {
public:
    MediaVenc();
    ~MediaVenc();

    // 删除了复制构造函数，删除了移动构造函数
    MediaVenc(const MediaVenc &other) = delete;
    MediaVenc(MediaVenc &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaVenc &operator=(const MediaVenc &other) = delete;
    MediaVenc &operator=(MediaVenc &&other) = delete;
};

API_END_NAMESPACE(media)

#endif