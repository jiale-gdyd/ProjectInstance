#include <stdio.h>
#include <mosquitto/mosquitto.h>

#include "config.h"

int mosquitto_validate_utf8(const char *str, int len)
{
    int i;
    int j;
    int codelen;
    int codepoint;
    const unsigned char *ustr = (const unsigned char *)str;

    if (!str) {
        return MOSQ_ERR_INVAL;
    }

    if ((len < 0) || (len > 65536)) {
        return MOSQ_ERR_INVAL;
    }

    for (i = 0; i < len; i++) {
        if (ustr[i] == 0) {
            return MOSQ_ERR_MALFORMED_UTF8;
        } else if (ustr[i] <= 0x7f) {
            codelen = 1;
            codepoint = ustr[i];
        } else if ((ustr[i] & 0xE0) == 0xC0) {
            // 110xxxxx - 2字节序列
            if ((ustr[i] == 0xC0) || (ustr[i] == 0xC1)) {
                return MOSQ_ERR_MALFORMED_UTF8;
            }

            codelen = 2;
            codepoint = (ustr[i] & 0x1F);
        } else if ((ustr[i] & 0xF0) == 0xE0) {
            // 1110xxxx - 3字节序列
            codelen = 3;
            codepoint = (ustr[i] & 0x0F);
        } else if ((ustr[i] & 0xF8) == 0xF0) {
            // 11110xxx - 4字节序列
            if (ustr[i] > 0xF4) {
                return MOSQ_ERR_MALFORMED_UTF8;
            }

            codelen = 4;
            codepoint = (ustr[i] & 0x07);
        } else {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        // 重建完整的代码点
        if (i >= (len - codelen + 1)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        for (j = 0; j < (codelen - 1); j++) {
            if ((ustr[++i] & 0xC0) != 0x80){
                return MOSQ_ERR_MALFORMED_UTF8;
            }

            codepoint = (codepoint << 6) | (ustr[i] & 0x3F);
        }

        // 检查UTF-16高/低替代
        if ((codepoint >= 0xD800) && (codepoint <= 0xDFFF)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        if ((codelen == 3) && (codepoint < 0x0800)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        } else if ((codelen == 4) && ((codepoint < 0x10000) || (codepoint > 0x10FFFF))) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        if ((codepoint >= 0xFDD0) && (codepoint <= 0xFDEF)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        if (((codepoint & 0xFFFF) == 0xFFFE) || ((codepoint & 0xFFFF) == 0xFFFF)) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }

        if ((codepoint <= 0x001F) || ((codepoint >= 0x007F) && (codepoint <= 0x009F))) {
            return MOSQ_ERR_MALFORMED_UTF8;
        }
    }

    return MOSQ_ERR_SUCCESS;
}
