#include <string.h>

#include <xlib/xlib/config.h>

#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xhmac.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xalloca.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

struct _XHmac {
    int           ref_count;
    XChecksumType digest_type;
    XChecksum     *digesti;
    XChecksum     *digesto;
};

XHmac *x_hmac_new(XChecksumType digest_type, const xuchar *key, xsize key_len)
{
    XHmac *hmac;
    xuchar *pad;
    xsize i, len;
    xuchar *buffer;
    xsize block_size;
    XChecksum *checksum;
    xssize block_size_signed, key_len_signed;

    x_return_val_if_fail(key_len <= X_MAXSSIZE, NULL);

    checksum = x_checksum_new(digest_type);
    x_return_val_if_fail(checksum != NULL, NULL);

    switch (digest_type) {
        case X_CHECKSUM_MD5:
        case X_CHECKSUM_SHA1:
            block_size = 64;
            break;

        case X_CHECKSUM_SHA256:
            block_size = 64;
            break;

        case X_CHECKSUM_SHA384:
        case X_CHECKSUM_SHA512:
            block_size = 128;
            break;

        default:
        x_return_val_if_reached(NULL);
    }

    hmac = x_slice_new0(XHmac);
    hmac->ref_count = 1;
    hmac->digest_type = digest_type;
    hmac->digesti = checksum;
    hmac->digesto = x_checksum_new(digest_type);

    buffer = (xuchar *)x_alloca0(block_size);
    pad = (xuchar *)x_alloca(block_size);

    if (key_len > block_size) {
        len = block_size;
        x_assert(key_len <= X_MAXSSIZE);

        key_len_signed = key_len;
        x_checksum_update(hmac->digesti, key, key_len_signed);
        x_checksum_get_digest(hmac->digesti, buffer, &len);
        x_checksum_reset(hmac->digesti);
    } else {
        memcpy(buffer, key, key_len);
    }

    x_assert(block_size <= X_MAXSSIZE);
    block_size_signed = block_size;

    for (i = 0; i < block_size; i++) {
        pad[i] = 0x36 ^ buffer[i];
    }
    x_checksum_update(hmac->digesti, pad, block_size_signed);

    for (i = 0; i < block_size; i++) {
        pad[i] = 0x5c ^ buffer[i];
    }
    x_checksum_update(hmac->digesto, pad, block_size_signed);

    return hmac;
}

XHmac *x_hmac_copy(const XHmac *hmac)
{
    XHmac *copy;

    x_return_val_if_fail(hmac != NULL, NULL);

    copy = x_slice_new(XHmac);
    copy->ref_count = 1;
    copy->digest_type = hmac->digest_type;
    copy->digesti = x_checksum_copy(hmac->digesti);
    copy->digesto = x_checksum_copy(hmac->digesto);

    return copy;
}

XHmac *x_hmac_ref (XHmac *hmac)
{
    x_return_val_if_fail(hmac != NULL, NULL);

    x_atomic_int_inc(&hmac->ref_count);
    return hmac;
}

void x_hmac_unref (XHmac *hmac)
{
    x_return_if_fail(hmac != NULL);

    if (x_atomic_int_dec_and_test(&hmac->ref_count)) {
        x_checksum_free(hmac->digesti);
        x_checksum_free(hmac->digesto);
        x_slice_free(XHmac, hmac);
    }
}

void x_hmac_update(XHmac *hmac, const xuchar *data, xssize length)
{
    x_return_if_fail(hmac != NULL);
    x_return_if_fail(length == 0 || data != NULL);
    x_checksum_update(hmac->digesti, data, length);
}

const xchar *x_hmac_get_string(XHmac *hmac)
{
    xuint8 *buffer;
    xsize digest_len;
    xssize digest_len_signed;

    x_return_val_if_fail(hmac != NULL, NULL);

    digest_len_signed = x_checksum_type_get_length(hmac->digest_type);
    x_assert(digest_len_signed >= 0);
    digest_len = digest_len_signed;

    buffer = (xuint8 *)x_alloca(digest_len);
    x_hmac_get_digest(hmac, buffer, &digest_len);

    return x_checksum_get_string(hmac->digesto);
}

void x_hmac_get_digest(XHmac *hmac, xuint8 *buffer, xsize *digest_len)
{
    xsize len;
    xssize len_signed;

    x_return_if_fail(hmac != NULL);

    len_signed = x_checksum_type_get_length(hmac->digest_type);
    x_assert(len_signed >= 0);
    len = len_signed;

    x_return_if_fail(*digest_len >= len);

    x_checksum_get_digest(hmac->digesti, buffer, &len);
    x_assert(len <= X_MAXSSIZE);
    len_signed = len;
    x_checksum_update(hmac->digesto, buffer, len_signed);
    x_checksum_get_digest(hmac->digesto, buffer, digest_len);
}

xchar *x_compute_hmac_for_data(XChecksumType digest_type, const xuchar *key, xsize key_len, const xuchar *data, xsize length)
{
    XHmac *hmac;
    xchar *retval;

    x_return_val_if_fail(length == 0 || data != NULL, NULL);

    hmac = x_hmac_new(digest_type, key, key_len);
    if (!hmac) {
        return NULL;
    }

    x_hmac_update(hmac, data, length);
    retval = x_strdup(x_hmac_get_string(hmac));
    x_hmac_unref(hmac);

    return retval;
}

xchar *x_compute_hmac_for_bytes(XChecksumType digest_type, XBytes *key, XBytes *data)
{
    xsize length;
    xsize key_len;
    xconstpointer key_data;
    xconstpointer byte_data;

    x_return_val_if_fail(data != NULL, NULL);
    x_return_val_if_fail(key != NULL, NULL);

    byte_data = x_bytes_get_data(data, &length);
    key_data = x_bytes_get_data(key, &key_len);
    return x_compute_hmac_for_data(digest_type, (const xuchar *)key_data, key_len, (const xuchar *)byte_data, length);
}

xchar *x_compute_hmac_for_string(XChecksumType digest_type, const xuchar *key, xsize key_len, const xchar *str, xssize length)
{
    x_return_val_if_fail(length == 0 || str != NULL, NULL);

    if (length < 0) {
        length = strlen(str);
    }

    return x_compute_hmac_for_data(digest_type, key, key_len, (const xuchar *)str, length);
}
