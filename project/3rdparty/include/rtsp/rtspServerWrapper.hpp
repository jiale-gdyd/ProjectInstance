#ifndef RTSP_RTSPSERVERWRAPPER_HPP
#define RTSP_RTSPSERVERWRAPPER_HPP

#include <cstdint>
#include <cstdbool>

namespace rtsp {
#define BUFF_FRAME_I   (0x01)       // I帧
#define BUFF_FRAME_P   (0x02)       // P帧
#define BUFF_FRAME_B   (0x03)       // B帧

typedef struct {
    uint32_t btype;                 // 缓存帧类型

    void     *vbuff;                // 视频帧数据指针
    uint32_t vsize;                 // 视频帧数据字节数
    uint64_t vtimstamp;             // 视频帧时间戳

    void     *abuff;                // 音频帧数据指针
    uint32_t asize;                 // 音频帧数据字节数
    uint64_t atimestamp;            // 音频帧时间戳
} rtsp_buff_t;

using rtsp_server_t = void *;
using rtsp_session_t = uint32_t;

rtsp_server_t rtsp_new_server(uint16_t port);
void rtsp_release_server(rtsp_server_t *server);

void rtsp_release_session(rtsp_server_t server, rtsp_session_t session);
rtsp_session_t rtsp_new_session(rtsp_server_t server, const char *url, bool bH265);

int rtsp_push(rtsp_server_t server, rtsp_session_t session, rtsp_buff_t *buff);
}

#endif
