#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xbase64.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xtestutils.h>

static const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

xsize x_base64_encode_step(const xuchar *in, xsize len, xboolean break_lines, xchar *out, xint *state, xint *save)
{
    char *outptr;
    const xuchar *inptr;

    x_return_val_if_fail(in != NULL || len == 0, 0);
    x_return_val_if_fail(out != NULL, 0);
    x_return_val_if_fail(state != NULL, 0);
    x_return_val_if_fail(save != NULL, 0);

    if (len == 0) {
        return 0;
    }

    inptr = in;
    outptr = out;

    if (len + ((char *)save)[0] > 2) {
        int already;
        int c1, c2, c3;
        const xuchar *inend = in + len - 2;

        already = *state;

        switch (((char *)save)[0]) {
            case 1:
                c1 = ((unsigned char *)save)[1];
                goto skip1;

            case 2:
                c1 = ((unsigned char *)save)[1];
                c2 = ((unsigned char *)save)[2];
                goto skip2;
        }

        while (inptr < inend) {
            c1 = *inptr++;
skip1:
            c2 = *inptr++;
skip2:
            c3 = *inptr++;
            *outptr++ = base64_alphabet[c1 >> 2];
            *outptr++ = base64_alphabet[c2 >> 4 | ((c1 & 0x3) << 4)];
            *outptr++ = base64_alphabet[((c2 & 0x0f) << 2) | (c3 >> 6)];
            *outptr++ = base64_alphabet[c3 & 0x3f];

            if (break_lines && (++already) >= 19) {
                *outptr++ = '\n';
                already = 0;
            }
        }

        ((char *)save)[0] = 0;
        len = 2 - (inptr - inend);
        *state = already;
    }

    x_assert(len == 0 || len == 1 || len == 2);

    {
        char *saveout;
        saveout = &(((char *)save)[1]) + ((char *)save)[0];

        switch (len) {
            case 2:
                *saveout++ = *inptr++;
                X_GNUC_FALLTHROUGH;

            case 1:
                *saveout++ = *inptr++;
        }

        ((char *)save)[0] += len;
    }

    return outptr - out;
}

xsize x_base64_encode_close(xboolean break_lines, xchar *out, xint *state, xint *save)
{
    int c1, c2;
    char *outptr = out;

    x_return_val_if_fail(out != NULL, 0);
    x_return_val_if_fail(state != NULL, 0);
    x_return_val_if_fail(save != NULL, 0);

    c1 = ((unsigned char *)save)[1];
    c2 = ((unsigned char *)save)[2];

    switch (((char *)save)[0]) {
        case 2:
            outptr [2] = base64_alphabet[((c2 & 0x0f) << 2)];
            x_assert(outptr [2] != 0);
            goto skip;

        case 1:
            outptr[2] = '=';
            c2 = 0;
skip:
            outptr[0] = base64_alphabet[c1 >> 2];
            outptr[1] = base64_alphabet[c2 >> 4 | ((c1 & 0x3) << 4)];
            outptr[3] = '=';
            outptr += 4;
            break;
    }

    if (break_lines) {
        *outptr++ = '\n';
    }

    *save = 0;
    *state = 0;

    return outptr - out;
}

xchar *x_base64_encode(const xuchar *data, xsize len)
{
    xchar *out;
    xint save = 0;
    xint state = 0, outlen;

    x_return_val_if_fail(data != NULL || len == 0, NULL);
    x_return_val_if_fail(len < ((X_MAXSIZE - 1) / 4 - 1) * 3, NULL);

    out = (xchar *)x_malloc((len / 3 + 1) * 4 + 1);

    outlen = x_base64_encode_step(data, len, FALSE, out, &state, &save);
    outlen += x_base64_encode_close(FALSE, out + outlen, &state, &save);
    out[outlen] = '\0';

    return (xchar *)out;
}

static const unsigned char mime_base64_rank[256] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
    255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
    255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

xsize x_base64_decode_step(const xchar *in, xsize len, xuchar *out, xint *state, xuint *save)
{
    int i;
    xuchar c, rank;
    xuchar last[2];
    unsigned int v;
    xuchar *outptr;
    const xuchar *inend;
    const xuchar *inptr;

    x_return_val_if_fail(in != NULL || len == 0, 0);
    x_return_val_if_fail(out != NULL, 0);
    x_return_val_if_fail(state != NULL, 0);
    x_return_val_if_fail(save != NULL, 0);

    if (len == 0) {
        return 0;
    }

    inend = (const xuchar *)in + len;
    outptr = out;

    v = *save;
    i = *state;

    last[0] = last[1] = 0;

    if (i < 0) {
        i = -i;
        last[0] = '=';
    }

    inptr = (const xuchar *)in;
    while (inptr < inend) {
        c = *inptr++;
        rank = mime_base64_rank [c];
        if (rank != 0xff) {
            last[1] = last[0];
            last[0] = c;
            v = (v << 6) | rank;

            i++;
            if (i == 4) {
                *outptr++ = v >> 16;
                if (last[1] != '=') {
                    *outptr++ = v >> 8;
                }

                if (last[0] != '=') {
                    *outptr++ = v;
                }

                i = 0;
            }
        }
    }

    *save = v;
    *state = last[0] == '=' ? -i : i;

    return outptr - out;
}

xuchar *x_base64_decode(const xchar *text, xsize *out_len)
{
    xuchar *ret;
    xint state = 0;
    xuint save = 0;
    xsize input_length;

    x_return_val_if_fail(text != NULL, NULL);
    x_return_val_if_fail(out_len != NULL, NULL);

    input_length = strlen(text);

    ret = (xuchar *)x_malloc0((input_length / 4) * 3 + 1);

    *out_len = x_base64_decode_step(text, input_length, ret, &state, &save);

    return ret;
}

xuchar *x_base64_decode_inplace(xchar *text, xsize *out_len)
{
    xuint save = 0;
    xint input_length, state = 0;

    x_return_val_if_fail(text != NULL, NULL);
    x_return_val_if_fail(out_len != NULL, NULL);

    input_length = strlen(text);

    x_return_val_if_fail(input_length > 1, NULL);

    *out_len = x_base64_decode_step(text, input_length, (xuchar *)text, &state, &save);

    return (xuchar *)text;
}
