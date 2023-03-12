#ifndef LOGC_ARRAYLIST_H
#define LOGC_ARRAYLIST_H

#define ARRAY_LIST_DEFAULT_SIZE         (32)

typedef void (*arraylist_del_func)(void *data);
typedef int (*arraylist_cmp_func)(void *data1, void *data2);

struct logc_arraylist {
    void               **array;
    int                len;
    int                size;
    arraylist_del_func del;
};

void logc_arraylist_del(struct logc_arraylist *a_list);
struct logc_arraylist *logc_arraylist_new(arraylist_del_func del);

int logc_arraylist_add(struct logc_arraylist *a_list, void *data);
int logc_arraylist_set(struct logc_arraylist *a_list, int idx, void *data);
int logc_arraylist_sortadd(struct logc_arraylist *a_list, arraylist_cmp_func cmp, void *data);

#define logc_arraylist_len(a_list)                (a_list->len)
#define logc_arraylist_get(a_list, i)             ((i >= a_list->len) ? NULL : a_list->array[i])
#define logc_arraylist_foreach(a_list, i, a_unit) for (i = 0, a_unit = (typeof(a_unit))a_list->array[0]; (i < a_list->len) && (a_unit = (typeof(a_unit))a_list->array[i], 1); i++)

#endif
