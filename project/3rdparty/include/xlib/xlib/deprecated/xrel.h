#ifndef __X_REL_H__
#define __X_REL_H__

#include "../xtypes.h"

X_BEGIN_DECLS

typedef struct _XTuples XTuples;
typedef struct _XRelation XRelation;

struct _XTuples {
    xuint len;
};

XLIB_DEPRECATED_IN_2_26
XRelation *x_relation_new(xint fields);

XLIB_DEPRECATED_IN_2_26
void x_relation_destroy(XRelation *relation);

XLIB_DEPRECATED_IN_2_26
void x_relation_index(XRelation *relation, xint field, XHashFunc hash_func, XEqualFunc key_equal_func);

XLIB_DEPRECATED_IN_2_26
void x_relation_insert(XRelation *relation, ...);

XLIB_DEPRECATED_IN_2_26
xint x_relation_delete(XRelation *relation, xconstpointer key, xint field);

XLIB_DEPRECATED_IN_2_26
XTuples *x_relation_select(XRelation *relation, xconstpointer key, xint field);

XLIB_DEPRECATED_IN_2_26
xint x_relation_count(XRelation *relation, xconstpointer key, xint field);

XLIB_DEPRECATED_IN_2_26
xboolean x_relation_exists(XRelation *relation, ...);

XLIB_DEPRECATED_IN_2_26
void x_relation_print(XRelation *relation);

XLIB_DEPRECATED_IN_2_26
void x_tuples_destroy(XTuples *tuples);

XLIB_DEPRECATED_IN_2_26
xpointer x_tuples_index(XTuples *tuples, xint index_, xint field);

X_END_DECLS

#endif
