#ifndef __x_ATOMIC_ARRAY_H__
#define __x_ATOMIC_ARRAY_H__

#include "../xlib.h"

X_BEGIN_DECLS

typedef union _XAtomicArrayMetadata {
    xsize    size;
    xpointer _alignment_padding;
} XAtomicArrayMetadata;
#define X_ATOMIC_ARRAY_DATA_SIZE(mem)       (((XAtomicArrayMetadata *)(mem) - 1)->size)

typedef struct _XAtomicArray XAtomicArray;
struct _XAtomicArray {
    xpointer data;
};

void _x_atomic_array_init(XAtomicArray *array);
void _x_atomic_array_update(XAtomicArray *array, xpointer new_data);
xpointer _x_atomic_array_copy(XAtomicArray *array, xsize header_size, xsize additional_element_size);

#define X_ATOMIC_ARRAY_GET_LOCKED(_array, _type)            ((_type *)((_array)->data))

#define X_ATOMIC_ARRAY_DO_TRANSACTION(_array, _type, _C_)   \
    X_STMT_START {                                          \
        xpointer *_datap  = &(_array)->data;                \
        _type *transaction_data, *__check;                  \
                                                            \
        __check = (_type *)x_atomic_pointer_get(_datap);    \
        do {                                                \
            transaction_data = __check;                     \
            {_C_;}                                          \
            __check = (_type *)x_atomic_pointer_get(_datap);\
        } while (transaction_data != __check);              \
    } X_STMT_END

X_END_DECLS

#endif
