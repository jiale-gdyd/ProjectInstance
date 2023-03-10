#ifndef RTSPSERVER_MD5_MD5_HPP
#define RTSPSERVER_MD5_MD5_HPP

#include <string>
#include <chrono>
#include <random>
#include <cstdint>
#include <cstring>
#include <stddef.h>

namespace md5 {
typedef unsigned int md5_word_t;
typedef unsigned char md5_byte_t;

typedef struct md5_state_s {
    md5_word_t count[2];
    md5_word_t abcd[4];
    md5_byte_t buf[64];
} md5_state_t;

inline void md5_init(md5_state_t *pms);
inline void md5_finish(md5_state_t *pms, md5_byte_t digest[16]);
inline void md5_append(md5_state_t *pms, md5_byte_t const * data, size_t nbytes);

#undef ZSW_MD5_BYTE_ORDER
#ifdef ARCH_IS_BIG_ENDIAN
#define ZSW_MD5_BYTE_ORDER      (ARCH_IS_BIG_ENDIAN ? 1 : -1)
#else
#define ZSW_MD5_BYTE_ORDER      0
#endif

#define ZSW_MD5_T_MASK          ((md5_word_t)~0)
#define ZSW_MD5_T1              (ZSW_MD5_T_MASK ^ 0x28955b87)
#define ZSW_MD5_T2              (ZSW_MD5_T_MASK ^ 0x173848a9)
#define ZSW_MD5_T3              0x242070db
#define ZSW_MD5_T4              (ZSW_MD5_T_MASK ^ 0x3e423111)
#define ZSW_MD5_T5              (ZSW_MD5_T_MASK ^ 0x0a83f050)
#define ZSW_MD5_T6              0x4787c62a
#define ZSW_MD5_T7              (ZSW_MD5_T_MASK ^ 0x57cfb9ec)
#define ZSW_MD5_T8              (ZSW_MD5_T_MASK ^ 0x02b96afe)
#define ZSW_MD5_T9              0x698098d8
#define ZSW_MD5_T10             (ZSW_MD5_T_MASK ^ 0x74bb0850)
#define ZSW_MD5_T11             (ZSW_MD5_T_MASK ^ 0x0000a44e)
#define ZSW_MD5_T12             (ZSW_MD5_T_MASK ^ 0x76a32841)
#define ZSW_MD5_T13             0x6b901122
#define ZSW_MD5_T14             (ZSW_MD5_T_MASK ^ 0x02678e6c)
#define ZSW_MD5_T15             (ZSW_MD5_T_MASK ^ 0x5986bc71)
#define ZSW_MD5_T16             0x49b40821
#define ZSW_MD5_T17             (ZSW_MD5_T_MASK ^ 0x09e1da9d)
#define ZSW_MD5_T18             (ZSW_MD5_T_MASK ^ 0x3fbf4cbf)
#define ZSW_MD5_T19             0x265e5a51
#define ZSW_MD5_T20             (ZSW_MD5_T_MASK ^ 0x16493855)
#define ZSW_MD5_T21             (ZSW_MD5_T_MASK ^ 0x29d0efa2)
#define ZSW_MD5_T22             0x02441453
#define ZSW_MD5_T23             (ZSW_MD5_T_MASK ^ 0x275e197e)
#define ZSW_MD5_T24             (ZSW_MD5_T_MASK ^ 0x182c0437)
#define ZSW_MD5_T25             0x21e1cde6
#define ZSW_MD5_T26             (ZSW_MD5_T_MASK ^ 0x3cc8f829)
#define ZSW_MD5_T27             (ZSW_MD5_T_MASK ^ 0x0b2af278)
#define ZSW_MD5_T28             0x455a14ed
#define ZSW_MD5_T29             (ZSW_MD5_T_MASK ^ 0x561c16fa)
#define ZSW_MD5_T30             (ZSW_MD5_T_MASK ^ 0x03105c07)
#define ZSW_MD5_T31             0x676f02d9
#define ZSW_MD5_T32             (ZSW_MD5_T_MASK ^ 0x72d5b375)
#define ZSW_MD5_T33             (ZSW_MD5_T_MASK ^ 0x0005c6bd)
#define ZSW_MD5_T34             (ZSW_MD5_T_MASK ^ 0x788e097e)
#define ZSW_MD5_T35             0x6d9d6122
#define ZSW_MD5_T36             (ZSW_MD5_T_MASK ^ 0x021ac7f3)
#define ZSW_MD5_T37             (ZSW_MD5_T_MASK ^ 0x5b4115bb)
#define ZSW_MD5_T38             0x4bdecfa9
#define ZSW_MD5_T39             (ZSW_MD5_T_MASK ^ 0x0944b49f)
#define ZSW_MD5_T40             (ZSW_MD5_T_MASK ^ 0x4140438f)
#define ZSW_MD5_T41             0x289b7ec6
#define ZSW_MD5_T42             (ZSW_MD5_T_MASK ^ 0x155ed805)
#define ZSW_MD5_T43             (ZSW_MD5_T_MASK ^ 0x2b10cf7a)
#define ZSW_MD5_T44             0x04881d05
#define ZSW_MD5_T45             (ZSW_MD5_T_MASK ^ 0x262b2fc6)
#define ZSW_MD5_T46             (ZSW_MD5_T_MASK ^ 0x1924661a)
#define ZSW_MD5_T47             0x1fa27cf8
#define ZSW_MD5_T48             (ZSW_MD5_T_MASK ^ 0x3b53a99a)
#define ZSW_MD5_T49             (ZSW_MD5_T_MASK ^ 0x0bd6ddbb)
#define ZSW_MD5_T50             0x432aff97
#define ZSW_MD5_T51             (ZSW_MD5_T_MASK ^ 0x546bdc58)
#define ZSW_MD5_T52             (ZSW_MD5_T_MASK ^ 0x036c5fc6)
#define ZSW_MD5_T53             0x655b59c3
#define ZSW_MD5_T54             (ZSW_MD5_T_MASK ^ 0x70f3336d)
#define ZSW_MD5_T55             (ZSW_MD5_T_MASK ^ 0x00100b82)
#define ZSW_MD5_T56             (ZSW_MD5_T_MASK ^ 0x7a7ba22e)
#define ZSW_MD5_T57             0x6fa87e4f
#define ZSW_MD5_T58             (ZSW_MD5_T_MASK ^ 0x01d3191f)
#define ZSW_MD5_T59             (ZSW_MD5_T_MASK ^ 0x5cfebceb)
#define ZSW_MD5_T60             0x4e0811a1
#define ZSW_MD5_T61             (ZSW_MD5_T_MASK ^ 0x08ac817d)
#define ZSW_MD5_T62             (ZSW_MD5_T_MASK ^ 0x42c50dca)
#define ZSW_MD5_T63             0x2ad7d2bb
#define ZSW_MD5_T64             (ZSW_MD5_T_MASK ^ 0x14792c6e)

const char hexval[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static void md5_process(md5_state_t *pms, md5_byte_t const *data)
{
    md5_word_t t;
    md5_word_t a = pms->abcd[0], b = pms->abcd[1], c = pms->abcd[2], d = pms->abcd[3];

#if ZSW_MD5_BYTE_ORDER > 0
    md5_word_t X[16];
#else
    md5_word_t xbuf[16];
    md5_word_t const * X;
#endif

    {
#if ZSW_MD5_BYTE_ORDER == 0
    static int const w = 1;

    if (*((md5_byte_t const *)&w))
#endif
#if ZSW_MD5_BYTE_ORDER <= 0
    {
        if (!((data - (md5_byte_t const *)0) & 3)) {
            X = (md5_word_t const *)data;
        } else {
            std::memcpy(xbuf, data, 64);
            X = xbuf;
        }
    }
#endif
#if ZSW_MD5_BYTE_ORDER == 0
    else
#endif
#if ZSW_MD5_BYTE_ORDER >= 0
    {
        int i;
        const md5_byte_t *xp = data;

#if ZSW_MD5_BYTE_ORDER == 0
        X = xbuf;
#else
#define xbuf    X
#endif
        for (i = 0; i < 16; ++i, xp += 4) {
            xbuf[i] = xp[0] + (xp[1] << 8) + (xp[2] << 16) + (xp[3] << 24);
        }
    }
#endif
    }

#define ZSW_MD5_ROTATE_LEFT(x, n)   (((x) << (n)) | ((x) >> (32 - (n))))

#define ZSW_MD5_F(x, y, z)          (((x) & (y)) | (~(x) & (z)))
#define SET(a, b, c, d, k, s, Ti)   t = a + ZSW_MD5_F(b,c,d) + X[k] + Ti; a = ZSW_MD5_ROTATE_LEFT(t, s) + b

    SET(a, b, c, d,  0,  7,  ZSW_MD5_T1);
    SET(d, a, b, c,  1, 12,  ZSW_MD5_T2);
    SET(c, d, a, b,  2, 17,  ZSW_MD5_T3);
    SET(b, c, d, a,  3, 22,  ZSW_MD5_T4);
    SET(a, b, c, d,  4,  7,  ZSW_MD5_T5);
    SET(d, a, b, c,  5, 12,  ZSW_MD5_T6);
    SET(c, d, a, b,  6, 17,  ZSW_MD5_T7);
    SET(b, c, d, a,  7, 22,  ZSW_MD5_T8);
    SET(a, b, c, d,  8,  7,  ZSW_MD5_T9);
    SET(d, a, b, c,  9, 12, ZSW_MD5_T10);
    SET(c, d, a, b, 10, 17, ZSW_MD5_T11);
    SET(b, c, d, a, 11, 22, ZSW_MD5_T12);
    SET(a, b, c, d, 12,  7, ZSW_MD5_T13);
    SET(d, a, b, c, 13, 12, ZSW_MD5_T14);
    SET(c, d, a, b, 14, 17, ZSW_MD5_T15);
    SET(b, c, d, a, 15, 22, ZSW_MD5_T16);
#undef SET

#define ZSW_MD5_G(x, y, z)          (((x) & (z)) | ((y) & ~(z)))
#define SET(a, b, c, d, k, s, Ti)   t = a + ZSW_MD5_G(b,c,d) + X[k] + Ti; a = ZSW_MD5_ROTATE_LEFT(t, s) + b

    SET(a, b, c, d,  1,  5, ZSW_MD5_T17);
    SET(d, a, b, c,  6,  9, ZSW_MD5_T18);
    SET(c, d, a, b, 11, 14, ZSW_MD5_T19);
    SET(b, c, d, a,  0, 20, ZSW_MD5_T20);
    SET(a, b, c, d,  5,  5, ZSW_MD5_T21);
    SET(d, a, b, c, 10,  9, ZSW_MD5_T22);
    SET(c, d, a, b, 15, 14, ZSW_MD5_T23);
    SET(b, c, d, a,  4, 20, ZSW_MD5_T24);
    SET(a, b, c, d,  9,  5, ZSW_MD5_T25);
    SET(d, a, b, c, 14,  9, ZSW_MD5_T26);
    SET(c, d, a, b,  3, 14, ZSW_MD5_T27);
    SET(b, c, d, a,  8, 20, ZSW_MD5_T28);
    SET(a, b, c, d, 13,  5, ZSW_MD5_T29);
    SET(d, a, b, c,  2,  9, ZSW_MD5_T30);
    SET(c, d, a, b,  7, 14, ZSW_MD5_T31);
    SET(b, c, d, a, 12, 20, ZSW_MD5_T32);
#undef SET

#define ZSW_MD5_H(x, y, z)          ((x) ^ (y) ^ (z))
#define SET(a, b, c, d, k, s, Ti)   t = a + ZSW_MD5_H(b,c,d) + X[k] + Ti; a = ZSW_MD5_ROTATE_LEFT(t, s) + b

    SET(a, b, c, d,  5,  4, ZSW_MD5_T33);
    SET(d, a, b, c,  8, 11, ZSW_MD5_T34);
    SET(c, d, a, b, 11, 16, ZSW_MD5_T35);
    SET(b, c, d, a, 14, 23, ZSW_MD5_T36);
    SET(a, b, c, d,  1,  4, ZSW_MD5_T37);
    SET(d, a, b, c,  4, 11, ZSW_MD5_T38);
    SET(c, d, a, b,  7, 16, ZSW_MD5_T39);
    SET(b, c, d, a, 10, 23, ZSW_MD5_T40);
    SET(a, b, c, d, 13,  4, ZSW_MD5_T41);
    SET(d, a, b, c,  0, 11, ZSW_MD5_T42);
    SET(c, d, a, b,  3, 16, ZSW_MD5_T43);
    SET(b, c, d, a,  6, 23, ZSW_MD5_T44);
    SET(a, b, c, d,  9,  4, ZSW_MD5_T45);
    SET(d, a, b, c, 12, 11, ZSW_MD5_T46);
    SET(c, d, a, b, 15, 16, ZSW_MD5_T47);
    SET(b, c, d, a,  2, 23, ZSW_MD5_T48);
#undef SET

#define ZSW_MD5_I(x, y, z)          ((y) ^ ((x) | ~(z)))
#define SET(a, b, c, d, k, s, Ti)   t = a + ZSW_MD5_I(b,c,d) + X[k] + Ti; a = ZSW_MD5_ROTATE_LEFT(t, s) + b

    SET(a, b, c, d,  0,  6, ZSW_MD5_T49);
    SET(d, a, b, c,  7, 10, ZSW_MD5_T50);
    SET(c, d, a, b, 14, 15, ZSW_MD5_T51);
    SET(b, c, d, a,  5, 21, ZSW_MD5_T52);
    SET(a, b, c, d, 12,  6, ZSW_MD5_T53);
    SET(d, a, b, c,  3, 10, ZSW_MD5_T54);
    SET(c, d, a, b, 10, 15, ZSW_MD5_T55);
    SET(b, c, d, a,  1, 21, ZSW_MD5_T56);
    SET(a, b, c, d,  8,  6, ZSW_MD5_T57);
    SET(d, a, b, c, 15, 10, ZSW_MD5_T58);
    SET(c, d, a, b,  6, 15, ZSW_MD5_T59);
    SET(b, c, d, a, 13, 21, ZSW_MD5_T60);
    SET(a, b, c, d,  4,  6, ZSW_MD5_T61);
    SET(d, a, b, c, 11, 10, ZSW_MD5_T62);
    SET(c, d, a, b,  2, 15, ZSW_MD5_T63);
    SET(b, c, d, a,  9, 21, ZSW_MD5_T64);
#undef SET

    pms->abcd[0] += a;
    pms->abcd[1] += b;
    pms->abcd[2] += c;
    pms->abcd[3] += d;
}

void md5_init(md5_state_t *pms)
{
    pms->count[0] = pms->count[1] = 0;
    pms->abcd[0] = 0x67452301;
    pms->abcd[1] = ZSW_MD5_T_MASK ^ 0x10325476;
    pms->abcd[2] = ZSW_MD5_T_MASK ^ 0x67452301;
    pms->abcd[3] = 0x10325476;
}

void md5_append(md5_state_t *pms, md5_byte_t const *data, size_t nbytes)
{
    size_t left = nbytes;
    md5_byte_t const *p = data;
    int offset = (pms->count[0] >> 3) & 63;
    md5_word_t nbits = (md5_word_t)(nbytes << 3);

    if (nbytes <= 0) {
        return;
    }

    pms->count[1] += (md5_word_t)(nbytes >> 29);
    pms->count[0] += nbits;
    if (pms->count[0] < nbits) {
        pms->count[1]++;
    }

    if (offset) {
        int copy = (offset + nbytes > 64 ? 64 - offset : static_cast<int>(nbytes));
        std::memcpy(pms->buf + offset, p, copy);
        if ((offset + copy) < 64) {
            return;
        }

        p += copy;
        left -= copy;
        md5_process(pms, pms->buf);
    }

    for (; left >= 64; p += 64, left -= 64) {
        md5_process(pms, p);
    }

    if (left) {
        std::memcpy(pms->buf, p, left);
    }
}

void md5_finish(md5_state_t *pms, md5_byte_t digest[16])
{
    int i;
    md5_byte_t data[8];
    static md5_byte_t const pad[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    for (i = 0; i < 8; ++i) {
        data[i] = (md5_byte_t)(pms->count[i >> 2] >> ((i & 3) << 3));
    }

    md5_append(pms, pad, ((55 - (pms->count[0] >> 3)) & 63) + 1);
    md5_append(pms, data, 8);

    for (i = 0; i < 16; ++i) {
        digest[i] = (md5_byte_t)(pms->abcd[i >> 2] >> ((i & 3) << 3));
    }
}

inline std::string md5_hash_string(std::string const &s)
{
    char digest[16];
    md5_state_t state;

    md5_init(&state);
    md5_append(&state, (md5_byte_t const *)s.c_str(), s.size());
    md5_finish(&state, (md5_byte_t *)digest);

    std::string ret;
    ret.resize(16);
    std::copy(digest,digest+16,ret.begin());

    return ret;
}

inline std::string md5_hash_hex(std::string const &input)
{
    std::string hash = md5_hash_string(input);
    std::string hex;

    for (size_t i = 0; i < hash.size(); i++) {
        hex.push_back(hexval[((hash[i] >> 4) & 0xF)]);
        hex.push_back(hexval[(hash[i]) & 0x0F]);
    }

    return hex;
}

inline std::string generate_nonce()
{
    std::random_device rd;

    auto timePoint = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    uint32_t timestamp = (uint32_t)timePoint.time_since_epoch().count();

    return md5_hash_hex(std::to_string(timestamp + rd()));
}
}

#endif
