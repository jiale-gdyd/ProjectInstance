#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <ax_vo_api.h>
#include <ax_sys_api.h>
#include <ax_base_type.h>

#include "common_vo_pattern.hpp"

namespace axpi {
typedef struct COLOR_RGB24 {
    AX_U32 u32Value: 24;
} __attribute__((__packed__)) COLOR_RGB24_S;

typedef struct COLOR_YUV {
    AX_U8 u8Y;
    AX_U8 u8U;
    AX_U8 u8V;
} COLOR_YUV_S;

typedef struct UTIL_COLOR_COMPONENT {
    AX_U32 u32Length;
    AX_U32 u32Offset;
} UTIL_COLOR_COMPONENT_S;

typedef struct UTIL_RGB_INFO {
    UTIL_COLOR_COMPONENT_S stRed;
    UTIL_COLOR_COMPONENT_S stGreen;
    UTIL_COLOR_COMPONENT_S stBlue;
    UTIL_COLOR_COMPONENT_S stAlpha;
} UTIL_RGB_INFO_S;

static inline AX_U32 SHIFT_COLOR8(const UTIL_COLOR_COMPONENT_S *pstComp, AX_U32 u32Value)
{
    u32Value &= 0xff;
    u32Value = (u32Value << 8) | u32Value;
    u32Value = u32Value >> (16 - pstComp->u32Length);
    return u32Value << pstComp->u32Offset;
}

#define MAKE_RGBA(stRGB, u8R, u8G, u8B, u8A) \
    (SHIFT_COLOR8(&(stRGB)->stRed, (u8R)) | SHIFT_COLOR8(&(stRGB)->stGreen, (u8G)) | SHIFT_COLOR8(&(stRGB)->stBlue, (u8B)) | SHIFT_COLOR8(&(stRGB)->stAlpha, (u8A)))

#define MAKE_RGB24(stRGB, u8R, u8G, u8B) \
    { .u32Value = MAKE_RGBA(stRGB, u8R, u8G, u8B, 0) }

#define MAKE_YUV_601_Y(u8R, u8G, u8B) \
    ((( 66 * (u8R) + 129 * (u8G) +  25 * (u8B) + 128) >> 8) + 16)

#define MAKE_YUV_601_U(u8R, u8G, u8B) \
    (((-38 * (u8R) -  74 * (u8G) + 112 * (u8B) + 128) >> 8) + 128)

#define MAKE_YUV_601_V(u8R, u8G, u8B) \
    (((112 * (u8R) -  94 * (u8G) -  18 * (u8B) + 128) >> 8) + 128)

#define MAKE_YUV_601(u8R, u8G, u8B) \
    { .u8Y = MAKE_YUV_601_Y(u8R, u8G, u8B), .u8U = MAKE_YUV_601_U(u8R, u8G, u8B), .u8V = MAKE_YUV_601_V(u8R, u8G, u8B) }

#define MAKE_RGB_INFO(u32RedLen, u32RedOffs, u32GreenLen, u32GreenOffs, u32BlueLen, u32BlueOffs, u32AlphaLen, u32AlphaOffs) \
    { .stRed = { (u32RedLen), (u32RedOffs) }, .stGreen = { (u32GreenLen), (u32GreenOffs) }, .stBlue = { (u32BlueLen), (u32BlueOffs) }, .stAlpha = { (u32AlphaLen), (u32AlphaOffs) } }

static AX_VOID Fill_Smpte_YUV(AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Stride, AX_U8 *u8YMem, AX_U8 *u8UMem, AX_U8 *u8VMem)
{
    const COLOR_YUV_S stCororsTop[] = {
        MAKE_YUV_601(191, 192, 192),
        MAKE_YUV_601(192, 192, 0),
        MAKE_YUV_601(0, 192, 192),
        MAKE_YUV_601(0, 192, 0),
        MAKE_YUV_601(192, 0, 192),
        MAKE_YUV_601(192, 0, 0),
        MAKE_YUV_601(0, 0, 192),
    };

    const COLOR_YUV_S stCororsMiddle[] = {
        MAKE_YUV_601(0, 0, 192),
        MAKE_YUV_601(19, 19, 19),
        MAKE_YUV_601(192, 0, 192),
        MAKE_YUV_601(19, 19, 19),
        MAKE_YUV_601(0, 192, 192),
        MAKE_YUV_601(19, 19, 19),
        MAKE_YUV_601(192, 192, 192),
    };

    const COLOR_YUV_S stCororsBottom[] = {
        MAKE_YUV_601(0, 33, 76),
        MAKE_YUV_601(255, 255, 255),
        MAKE_YUV_601(50, 0, 106),
        MAKE_YUV_601(19, 19, 19),
        MAKE_YUV_601(9, 9, 9),
        MAKE_YUV_601(19, 19, 19),
        MAKE_YUV_601(29, 29, 29),
        MAKE_YUV_601(19, 19, 19),
    };

    AX_U32 u32X;
    AX_U32 u32Y;
    AX_U32 u32CS = 2;
    AX_U32 u32XSub = 2;
    AX_U32 u32YSub = 2;

    for (u32Y = 0; u32Y < u32Height * 6 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X) {
            u8YMem[u32X] = stCororsTop[u32X * 7 / u32Width].u8Y;
        }

        u8YMem += u32Stride;
    }

    for (; u32Y < u32Height * 7 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X) {
            u8YMem[u32X] = stCororsMiddle[u32X * 7 / u32Width].u8Y;
        }

        u8YMem += u32Stride;
    }

    for (; u32Y < u32Height; ++u32Y) {
        for (u32X = 0; u32X < u32Width * 5 / 7; ++u32X) {
            u8YMem[u32X] = stCororsBottom[u32X * 4 / (u32Width * 5 / 7)].u8Y;
        }

        for (; u32X < u32Width * 6 / 7; ++u32X) {
            u8YMem[u32X] = stCororsBottom[(u32X - u32Width * 5 / 7) * 3 / (u32Width / 7) + 4].u8Y;
        }

        for (; u32X < u32Width; ++u32X) {
            u8YMem[u32X] = stCororsBottom[7].u8Y;
        }

        u8YMem += u32Stride;
    }

    for (u32Y = 0; u32Y < u32Height / u32YSub * 6 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; u32X += u32XSub) {
            u8UMem[u32X * u32CS / u32XSub] = stCororsTop[u32X * 7 / u32Width].u8U;
            u8VMem[u32X * u32CS / u32XSub] = stCororsTop[u32X * 7 / u32Width].u8V;
        }

        u8UMem += u32Stride * u32CS / u32XSub;
        u8VMem += u32Stride * u32CS / u32XSub;
    }

    for (; u32Y < u32Height / u32YSub * 7 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; u32X += u32XSub) {
            u8UMem[u32X * u32CS / u32XSub] = stCororsMiddle[u32X * 7 / u32Width].u8U;
            u8VMem[u32X * u32CS / u32XSub] = stCororsMiddle[u32X * 7 / u32Width].u8V;
        }

        u8UMem += u32Stride * u32CS / u32XSub;
        u8VMem += u32Stride * u32CS / u32XSub;
    }

    for (; u32Y < u32Height / u32YSub; ++u32Y) {
        for (u32X = 0; u32X < u32Width * 5 / 7; u32X += u32XSub) {
            u8UMem[u32X * u32CS / u32XSub] = stCororsBottom[u32X * 4 / (u32Width * 5 / 7)].u8U;
            u8VMem[u32X * u32CS / u32XSub] = stCororsBottom[u32X * 4 / (u32Width * 5 / 7)].u8V;
        }

        for (; u32X < u32Width * 6 / 7; u32X += u32XSub) {
            u8UMem[u32X * u32CS / u32XSub] = stCororsBottom[(u32X - u32Width * 5 / 7) * 3 / (u32Width / 7) + 4].u8U;
            u8VMem[u32X * u32CS / u32XSub] = stCororsBottom[(u32X - u32Width * 5 / 7) * 3 / (u32Width / 7) + 4].u8V;
        }

        for (; u32X < u32Width; u32X += u32XSub) {
            u8UMem[u32X * u32CS / u32XSub] = stCororsBottom[7].u8U;
            u8VMem[u32X * u32CS / u32XSub] = stCororsBottom[7].u8V;
        }

        u8UMem += u32Stride * u32CS / u32XSub;
        u8VMem += u32Stride * u32CS / u32XSub;
    }
}

static AX_VOID Fill_Smpte_RGB16(const UTIL_RGB_INFO_S *stRGB, AX_VOID *pMem, AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Stride)
{
    const AX_U16 u16ColorsTop[] = {
        MAKE_RGBA(stRGB, 192, 192, 192, 255),
        MAKE_RGBA(stRGB, 192, 192, 0, 255),
        MAKE_RGBA(stRGB, 0, 192, 192, 255),
        MAKE_RGBA(stRGB, 0, 192, 0, 255),
        MAKE_RGBA(stRGB, 192, 0, 192, 255),
        MAKE_RGBA(stRGB, 192, 0, 0, 255),
        MAKE_RGBA(stRGB, 0, 0, 192, 255),
    };

    const AX_U16 u16ColorsMiddle[] = {
        MAKE_RGBA(stRGB, 0, 0, 192, 127),
        MAKE_RGBA(stRGB, 19, 19, 19, 127),
        MAKE_RGBA(stRGB, 192, 0, 192, 127),
        MAKE_RGBA(stRGB, 19, 19, 19, 127),
        MAKE_RGBA(stRGB, 0, 192, 192, 127),
        MAKE_RGBA(stRGB, 19, 19, 19, 127),
        MAKE_RGBA(stRGB, 192, 192, 192, 127),
    };

    const AX_U16 u16ColorsBottom[] = {
        MAKE_RGBA(stRGB, 0, 33, 76, 255),
        MAKE_RGBA(stRGB, 255, 255, 255, 255),
        MAKE_RGBA(stRGB, 50, 0, 106, 255),
        MAKE_RGBA(stRGB, 19, 19, 19, 255),
        MAKE_RGBA(stRGB, 9, 9, 9, 255),
        MAKE_RGBA(stRGB, 19, 19, 19, 255),
        MAKE_RGBA(stRGB, 29, 29, 29, 255),
        MAKE_RGBA(stRGB, 19, 19, 19, 255),
    };

    AX_U32 u32X;
    AX_U32 u32Y;

    for (u32Y = 0; u32Y < u32Height * 6 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X) {
            ((AX_U16 *)pMem)[u32X] = u16ColorsTop[u32X * 7 / u32Width];
        }

        pMem += u32Stride;
    }

    for (; u32Y < u32Height * 7 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X) {
            ((AX_U16 *)pMem)[u32X] = u16ColorsMiddle[u32X * 7 / u32Width];
        }

        pMem += u32Stride;
    }

    for (; u32Y < u32Height; ++u32Y) {
        for (u32X = 0; u32X < u32Width * 5 / 7; ++u32X) {
            ((AX_U16 *)pMem)[u32X] = u16ColorsBottom[u32X * 4 / (u32Width * 5 / 7)];
        }

        for (; u32X < u32Width * 6 / 7; ++u32X) {
            ((AX_U16 *)pMem)[u32X] = u16ColorsBottom[(u32X - u32Width * 5 / 7) * 3 / (u32Width / 7) + 4];
        }

        for (; u32X < u32Width; ++u32X) {
            ((AX_U16 *)pMem)[u32X] = u16ColorsBottom[7];
        }

        pMem += u32Stride;
    }
}

static AX_VOID Fill_Smpte_RGB24(const UTIL_RGB_INFO_S *stRGB, AX_VOID *pMem, AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Stride)
{
    const COLOR_RGB24_S stColorsTop[] = {
        MAKE_RGB24(stRGB, 192, 192, 192),
        MAKE_RGB24(stRGB, 192, 192, 0),
        MAKE_RGB24(stRGB, 0, 192, 192),
        MAKE_RGB24(stRGB, 0, 192, 0),
        MAKE_RGB24(stRGB, 192, 0, 192),
        MAKE_RGB24(stRGB, 192, 0, 0),
        MAKE_RGB24(stRGB, 0, 0, 192),
    };

    const COLOR_RGB24_S stColorsMiddle[] = {
        MAKE_RGB24(stRGB, 0, 0, 192),
        MAKE_RGB24(stRGB, 19, 19, 19),
        MAKE_RGB24(stRGB, 192, 0, 192),
        MAKE_RGB24(stRGB, 19, 19, 19),
        MAKE_RGB24(stRGB, 0, 192, 192),
        MAKE_RGB24(stRGB, 19, 19, 19),
        MAKE_RGB24(stRGB, 192, 192, 192),
    };

    const COLOR_RGB24_S stColorsBottom[] = {
        MAKE_RGB24(stRGB, 0, 33, 76),
        MAKE_RGB24(stRGB, 255, 255, 255),
        MAKE_RGB24(stRGB, 50, 0, 106),
        MAKE_RGB24(stRGB, 19, 19, 19),
        MAKE_RGB24(stRGB, 9, 9, 9), 
        MAKE_RGB24(stRGB, 19, 19, 19),
        MAKE_RGB24(stRGB, 29, 29, 29),
        MAKE_RGB24(stRGB, 19, 19, 19),
    };

    AX_U32 u32X;
    AX_U32 u32Y;

    for (u32Y = 0; u32Y < u32Height * 6 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X) {
            ((COLOR_RGB24_S *)pMem)[u32X] = stColorsTop[u32X * 7 / u32Width];
        }

        pMem += u32Stride;
    }

    for (; u32Y < u32Height * 7 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X) {
            ((COLOR_RGB24_S *)pMem)[u32X] = stColorsMiddle[u32X * 7 / u32Width];
        }

        pMem += u32Stride;
    }

    for (; u32Y < u32Height; ++u32Y) {
        for (u32X = 0; u32X < u32Width * 5 / 7; ++u32X) {
            ((COLOR_RGB24_S *)pMem)[u32X] = stColorsBottom[u32X * 4 / (u32Width * 5 / 7)];
        }

        for (; u32X < u32Width * 6 / 7; ++u32X) {
            ((COLOR_RGB24_S *)pMem)[u32X] = stColorsBottom[(u32X - u32Width * 5 / 7) * 3 / (u32Width / 7) + 4];
        }

        for (; u32X < u32Width; ++u32X) {
            ((COLOR_RGB24_S *)pMem)[u32X] = stColorsBottom[7];
        }

        pMem += u32Stride;
    }
}

static AX_VOID Fill_Smpte_RGB32(const UTIL_RGB_INFO_S *stRGB, AX_VOID *pMem, AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Stride)
{
    const AX_U32 u32ColorsTop[] = {
        MAKE_RGBA(stRGB, 192, 192, 192, 255),
        MAKE_RGBA(stRGB, 192, 192, 0, 255),
        MAKE_RGBA(stRGB, 0, 192, 192, 255),
        MAKE_RGBA(stRGB, 0, 192, 0, 255),
        MAKE_RGBA(stRGB, 192, 0, 192, 255),
        MAKE_RGBA(stRGB, 192, 0, 0, 255),
        MAKE_RGBA(stRGB, 0, 0, 192, 255),
    };

    const AX_U32 u32ColorsMiddle[] = {
        MAKE_RGBA(stRGB, 0, 0, 192, 127),
        MAKE_RGBA(stRGB, 19, 19, 19, 127),
        MAKE_RGBA(stRGB, 192, 0, 192, 127),
        MAKE_RGBA(stRGB, 19, 19, 19, 127),
        MAKE_RGBA(stRGB, 0, 192, 192, 127),
        MAKE_RGBA(stRGB, 19, 19, 19, 127),
        MAKE_RGBA(stRGB, 192, 192, 192, 127),
    };

    const AX_U32 u32ColorsBottom[] = {
        MAKE_RGBA(stRGB, 0, 33, 76, 255),
        MAKE_RGBA(stRGB, 255, 255, 255, 255),
        MAKE_RGBA(stRGB, 50, 0, 106, 255),
        MAKE_RGBA(stRGB, 19, 19, 19, 255),
        MAKE_RGBA(stRGB, 9, 9, 9, 255),
        MAKE_RGBA(stRGB, 19, 19, 19, 255),
        MAKE_RGBA(stRGB, 29, 29, 29, 255),
        MAKE_RGBA(stRGB, 19, 19, 19, 255),
    };

    AX_U32 u32X;
    AX_U32 u32Y;

    for (u32Y = 0; u32Y < u32Height * 6 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X) {
            ((AX_U32 *)pMem)[u32X] = u32ColorsTop[u32X * 7 / u32Width];
        }

        pMem += u32Stride;
    }

    for (; u32Y < u32Height * 7 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X) {
            ((AX_U32 *)pMem)[u32X] = u32ColorsMiddle[u32X * 7 / u32Width];
        }

        pMem += u32Stride;
    }

    for (; u32Y < u32Height; ++u32Y) {
        for (u32X = 0; u32X < u32Width * 5 / 7; ++u32X) {
            ((AX_U32 *)pMem)[u32X] = u32ColorsBottom[u32X * 4 / (u32Width * 5 / 7)];
        }

        for (; u32X < u32Width * 6 / 7; ++u32X) {
            ((AX_U32 *)pMem)[u32X] = u32ColorsBottom[(u32X - u32Width * 5 / 7) * 3 / (u32Width / 7) + 4];
        }

        for (; u32X < u32Width; ++u32X) {
            ((AX_U32 *)pMem)[u32X] = u32ColorsBottom[7];
        }

        pMem += u32Stride;
    }
}

void common_axvo_fill_color(int enPixFmt, uint32_t u32Width, uint32_t u32Height, uint32_t u32Stride, uint8_t *u8Mem)
{
    UTIL_RGB_INFO_S stAR12_Info = MAKE_RGB_INFO(4, 8, 4, 4, 4, 0, 4, 12);
    UTIL_RGB_INFO_S stAR15_Info = MAKE_RGB_INFO(5, 10, 5, 5, 5, 0, 1, 15);
    UTIL_RGB_INFO_S stRG16_Info = MAKE_RGB_INFO(5, 11, 6, 5, 5, 0, 0, 0);
    UTIL_RGB_INFO_S stRG24_Info = MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 0, 0);
    UTIL_RGB_INFO_S stAR24_Info = MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 8, 24);

    if (enPixFmt == AX_YUV420_SEMIPLANAR) {
        Fill_Smpte_YUV(u32Width, u32Height, u32Stride, u8Mem, u8Mem + u32Stride * u32Height, u8Mem + u32Stride * u32Height + 1);
    } else if (enPixFmt == AX_FORMAT_RGB565) {
        Fill_Smpte_RGB16(&stRG16_Info, u8Mem, u32Width, u32Height, u32Stride);
    } else if (enPixFmt == AX_FORMAT_RGB888) {
        Fill_Smpte_RGB24(&stRG24_Info, u8Mem, u32Width, u32Height, u32Stride);
    } else if (enPixFmt == AX_FORMAT_ARGB1555) {
        Fill_Smpte_RGB16(&stAR15_Info, u8Mem, u32Width, u32Height, u32Stride);
    } else if (enPixFmt == AX_FORMAT_ARGB4444) {
        Fill_Smpte_RGB16(&stAR12_Info, u8Mem, u32Width, u32Height, u32Stride);
    } else if (enPixFmt == AX_FORMAT_ARGB8888) {
        Fill_Smpte_RGB32(&stAR24_Info, u8Mem, u32Width, u32Height, u32Stride);
    }
}
}
