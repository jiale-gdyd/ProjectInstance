#ifndef AXERA_MEDIA_VDEC_HPP
#define AXERA_MEDIA_VDEC_HPP

#if !defined(__AXERA_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <vector>
#include <stdint.h>
#include <stdbool.h>
#include <utils/export.h>

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaVdec {
public:
    MediaVdec();
    ~MediaVdec();

    // 删除了复制构造函数，删除了移动构造函数
    MediaVdec(const MediaVdec &other) = delete;
    MediaVdec(MediaVdec &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaVdec &operator=(const MediaVdec &other) = delete;
    MediaVdec &operator=(MediaVdec &&other) = delete;
};

API_END_NAMESPACE(media)

#endif
