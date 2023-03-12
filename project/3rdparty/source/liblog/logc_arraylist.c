#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "logc_defs.h"

struct logc_arraylist *logc_arraylist_new(arraylist_del_func del)
{
    struct logc_arraylist *a_list;

    a_list = (struct logc_arraylist *)calloc(1, sizeof(*a_list));
    if (!a_list) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    a_list->size = ARRAY_LIST_DEFAULT_SIZE;
    a_list->len = 0;

    a_list->del = del;
    a_list->array = (void **)calloc(a_list->size, sizeof(void *));
    if (!a_list->array) {
        logc_error("calloc fail, errno[%d]", errno);
        free(a_list);

        return NULL;
    }

    return a_list;
}

void logc_arraylist_del(struct logc_arraylist *a_list)
{
    int i;

    if (!a_list) {
        return;
    }

    if (a_list->del) {
        for (i = 0; i < a_list->len; i++) {
            if (a_list->array[i]) {
                a_list->del(a_list->array[i]);
            }
        }
    }

    if (a_list->array) {
        free(a_list->array);
    }
    free(a_list);
}

static int logc_arraylist_expand_inner(struct logc_arraylist *a_list, int max)
{
    void *tmp;
    int new_size;
    int diff_size;

    new_size = logc_max(a_list->size * 2, max);
    tmp = realloc(a_list->array, new_size * sizeof(void *));
    if (!tmp) {
        logc_error("realloc fail, errno[%d]", errno);
        return -1;
    }

    a_list->array = (void **)tmp;
    diff_size = new_size - a_list->size;
    if (diff_size) {
        memset(a_list->array + a_list->size, 0x00, diff_size * sizeof(void *));
    }
    a_list->size = new_size;

    return 0;
}

int logc_arraylist_set(struct logc_arraylist *a_list, int idx, void *data)
{
    if (idx > (a_list->size - 1)) {
        if (logc_arraylist_expand_inner(a_list, idx)) {
            logc_error("expand_internal fail");
            return -1;
        }
    }

    if (a_list->array[idx] && a_list->del) {
        a_list->del(a_list->array[idx]);
    }

    a_list->array[idx] = data;
    if (a_list->len <= idx) {
        a_list->len = idx + 1;
    }

    return 0;
}

int logc_arraylist_add(struct logc_arraylist *a_list, void *data)
{
    return logc_arraylist_set(a_list, a_list->len, data);
}

static int logc_arraylist_insert_inner(struct logc_arraylist *a_list, int idx, void *data)
{
    if (a_list->array[idx] == NULL) {
        a_list->array[idx] = data;
        return 0;
    }

    if (a_list->len > (a_list->size - 1)) {
        if (logc_arraylist_expand_inner(a_list, 0)) {
            logc_error("expand_internal fail");
            return -1;
        }
    }

    memmove(a_list->array + idx + 1, a_list->array + idx, (a_list->len - idx) * sizeof(void *));
    a_list->array[idx] = data;
    a_list->len++;

    return 0;
}

int logc_arraylist_sortadd(struct logc_arraylist *a_list, arraylist_cmp_func cmp, void *data)
{
    int i;

    for (i = 0; i < a_list->len; i++) {
        if ((*cmp)(a_list->array[i], data) > 0) {
            break;
        }
    }

    if (i == a_list->len) {
        return logc_arraylist_add(a_list, data);
    } else {
        return logc_arraylist_insert_inner(a_list, i, data);
    }
}
