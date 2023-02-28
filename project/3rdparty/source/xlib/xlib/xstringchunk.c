#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xslist.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xstringchunk.h>
#include <xlib/xlib/xutilsprivate.h>

struct _XStringChunk {
    XHashTable *const_table;
    XSList     *storage_list;
    xsize      storage_next;
    xsize      this_size;
    xsize      default_size;
};

XStringChunk *x_string_chunk_new(xsize size)
{
    xsize actual_size = 1;
    XStringChunk *new_chunk = x_new(XStringChunk, 1);

    actual_size = x_nearest_pow(MAX (1, size));

    new_chunk->const_table = NULL;
    new_chunk->storage_list = NULL;
    new_chunk->storage_next = actual_size;
    new_chunk->default_size = actual_size;
    new_chunk->this_size  = actual_size;

    return new_chunk;
}

void x_string_chunk_free(XStringChunk *chunk)
{
    x_return_if_fail(chunk != NULL);

    if (chunk->storage_list) {
        x_slist_free_full(chunk->storage_list, x_free);
    }

    if (chunk->const_table) {
        x_hash_table_destroy(chunk->const_table);
    }

    x_free(chunk);
}

void x_string_chunk_clear(XStringChunk *chunk)
{
    x_return_if_fail(chunk != NULL);

    if (chunk->storage_list) {
        x_slist_free_full(chunk->storage_list, x_free);

        chunk->storage_list = NULL;
        chunk->storage_next = chunk->default_size;
        chunk->this_size = chunk->default_size;
    }

    if (chunk->const_table) {
        x_hash_table_remove_all(chunk->const_table);
    }
}

xchar *x_string_chunk_insert(XStringChunk *chunk, const xchar *string)
{
    x_return_val_if_fail(chunk != NULL, NULL);
    return x_string_chunk_insert_len(chunk, string, -1);
}

xchar *x_string_chunk_insert_const(XStringChunk *chunk, const xchar *string)
{
    char *lookup;

    x_return_val_if_fail(chunk != NULL, NULL);

    if (!chunk->const_table) {
        chunk->const_table = x_hash_table_new(x_str_hash, x_str_equal);
    }

    lookup = (char *)x_hash_table_lookup(chunk->const_table, (xchar *)string);
    if (!lookup) {
        lookup = x_string_chunk_insert(chunk, string);
        x_hash_table_add(chunk->const_table, lookup);
    }

    return lookup;
}

xchar *x_string_chunk_insert_len(XStringChunk *chunk, const xchar *string, xssize len)
{
    xsize size;
    xchar *pos;

    x_return_val_if_fail(chunk != NULL, NULL);

    if (len < 0) {
        size = strlen(string);
    } else {
        size = (xsize)len;
    }

    if ((X_MAXSIZE - chunk->storage_next < size + 1) || (chunk->storage_next + size + 1) > chunk->this_size) {
        xsize new_size = x_nearest_pow(MAX(chunk->default_size, size + 1));

        if (new_size == 0) {
            new_size = size + 1;
        }

        chunk->storage_list = x_slist_prepend(chunk->storage_list, x_new(xchar, new_size));

        chunk->this_size = new_size;
        chunk->storage_next = 0;
    }

    pos = ((xchar *)chunk->storage_list->data) + chunk->storage_next;
    *(pos + size) = '\0';
    memcpy(pos, string, size);
    chunk->storage_next += size + 1;

    return pos;
}
