#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xatomicarray.h>

X_LOCK_DEFINE_STATIC(array);

typedef struct _FreeListNode FreeListNode;

struct _FreeListNode {
    FreeListNode *next;
};

static FreeListNode *freelist = NULL;

static xpointer freelist_alloc(xsize size, xboolean reuse)
{
    xpointer mem;
    xsize real_size;
    FreeListNode *free, **prev;

    if (reuse) {
        for (free = freelist, prev = &freelist; free != NULL; prev = &free->next, free = free->next) {
            if (X_ATOMIC_ARRAY_DATA_SIZE(free) == size) {
                *prev = free->next;
                return (xpointer)free;
            }
        }
    }

    real_size = sizeof(XAtomicArrayMetadata) + MAX(size, sizeof(FreeListNode));
    mem = x_slice_alloc(real_size);
    mem = ((char *)mem) + sizeof(XAtomicArrayMetadata);
    X_ATOMIC_ARRAY_DATA_SIZE(mem) = size;

    return mem;
}

static void freelist_free(xpointer mem)
{
    FreeListNode *freet;

    freet = (FreeListNode *)mem;
    freet->next = freelist;
    freelist = freet;
}

void _x_atomic_array_init(XAtomicArray *array)
{
    array->data = NULL;
}

xpointer _x_atomic_array_copy(XAtomicArray *array, xsize header_size, xsize additional_element_size)
{
    xuint8 *newt, *old;
    xsize old_size, new_size;

    X_LOCK(array);
    old = (xuint8 *)x_atomic_pointer_get(&array->data);
    if (old) {
        old_size = X_ATOMIC_ARRAY_DATA_SIZE(old);
        new_size = old_size + additional_element_size;

        newt = (xuint8 *)freelist_alloc(new_size, additional_element_size != 0);
        memcpy(newt, old, old_size);
    } else if (additional_element_size != 0) {
        new_size = header_size + additional_element_size;
        newt = (xuint8 *)freelist_alloc(new_size, TRUE);
    } else {
        newt = NULL;
    }
    X_UNLOCK(array);

    return newt;
}

void _x_atomic_array_update(XAtomicArray *array, xpointer new_data)
{
    xuint8 *old;

    X_LOCK(array);
    old = (xuint8 *)x_atomic_pointer_exchange(&array->data, new_data);

    x_assert(old == NULL || X_ATOMIC_ARRAY_DATA_SIZE(old) <= X_ATOMIC_ARRAY_DATA_SIZE(new_data));

    if (old) {
        freelist_free(old);
    }
    X_UNLOCK(array);
}

