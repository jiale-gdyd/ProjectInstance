#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xqsort.h>
#include <xlib/xlib/xalloca.h>
#include <xlib/xlib/xtestutils.h>

struct msort_param {
    size_t           s;
    size_t           var;
    XCompareDataFunc cmp;
    void             *arg;
    char             *t;
};

static void msort_with_tmp(const struct msort_param *p, void *b, size_t n)
{
    char *b1, *b2;
    size_t n1, n2;
    char *tmp = p->t;
    void *arg = p->arg;
    const size_t s = p->s;
    XCompareDataFunc cmp = p->cmp;

    if (n <= 1) {
        return;
    }

    n1 = n / 2;
    n2 = n - n1;
    b1 = (char *)b;
    b2 = (char *)b + (n1 * p->s);

    msort_with_tmp(p, b1, n1);
    msort_with_tmp(p, b2, n2);

    switch (p->var) {
        case 0:
            while ((n1 > 0) && (n2 > 0)) {
                if ((*cmp)(b1, b2, arg) <= 0) {
                    *(xuint32 *)tmp = *(xuint32 *)b1;
                    b1 += sizeof(xuint32);
                    --n1;
                } else {
                    *(xuint32 *)tmp = *(xuint32 *)b2;
                    b2 += sizeof(xuint32);
                    --n2;
                }
                tmp += sizeof(xuint32);
            }
            break;

        case 1:
            while ((n1 > 0) && (n2 > 0)) {
                if ((*cmp)(b1, b2, arg) <= 0) {
                    *(xuint64 *)tmp = *(xuint64 *)b1;
                    b1 += sizeof(xuint64);
                    --n1;
                } else {
                    *(xuint64 *)tmp = *(xuint64 *)b2;
                    b2 += sizeof(xuint64);
                    --n2;
                }

                tmp += sizeof(xuint64);
            }
            break;

        case 2:
            while ((n1 > 0) && (n2 > 0)) {
                xuintptr *bl;
                xuintptr *tmpl = (xuintptr *)tmp;

                tmp += s;
                if ((*cmp)(b1, b2, arg) <= 0) {
                    bl = (xuintptr *)b1;
                    b1 += s;
                    --n1;
                } else {
                    bl = (xuintptr *)b2;
                    b2 += s;
                    --n2;
                }

                while (tmpl < (xuintptr *)tmp) {
                    *tmpl++ = *bl++;
                }
            }
            break;

        case 3:
            while ((n1 > 0) && (n2 > 0)) {
                if ((*cmp)(*(const void **)b1, *(const void **)b2, arg) <= 0) {
                    *(void **)tmp = *(void **)b1;
                    b1 += sizeof(void *);
                    --n1;
                } else {
                    *(void **)tmp = *(void **)b2;
                    b2 += sizeof(void *);
                    --n2;
                }
                tmp += sizeof(void *);
            }
            break;

        default:
            while ((n1 > 0) && (n2 > 0)) {
                if ((*cmp)(b1, b2, arg) <= 0) {
                    memcpy(tmp, b1, s);
                    tmp += s;
                    b1 += s;
                    --n1;
                } else {
                    memcpy(tmp, b2, s);
                    tmp += s;
                    b2 += s;
                    --n2;
                }
            }
            break;
    }

    if (n1 > 0) {
        memcpy(tmp, b1, n1 * s);
    }

    memcpy(b, p->t, (n - n2) * s);
}

static void msort_r(void *b, size_t n, size_t s, XCompareDataFunc cmp, void *arg)
{
    char *tmp = NULL;
    size_t size = n * s;
    struct msort_param p;

    if (s > 32) {
        size = 2 * n * sizeof(void *) + s;
    }

    if (size < 1024) {
        p.t = (char *)x_alloca(size);
    } else {
        tmp = (char *)x_malloc(size);
        p.t = tmp;
    }

    p.s = s;
    p.var = 4;
    p.cmp = cmp;
    p.arg = arg;

    if (s > 32) {
        char *kp;
        size_t i;
        char *ip = (char *)b;
        void **tp = (void **)(p.t + n * sizeof(void *));
        void **t = tp;
        void *tmp_storage = (void *)(tp + n);

        while ((void *)t < tmp_storage) {
            *t++ = ip;
            ip += s;
        }

        p.s = sizeof(void *);
        p.var = 3;
        msort_with_tmp(&p, p.t + n * sizeof(void *), n);

        for (i = 0, ip = (char *)b; i < n; i++, ip += s) {
            if ((kp = (char *)tp[i]) != ip) {
                size_t j = i;
                char *jp = ip;
                memcpy (tmp_storage, ip, s);

                do {
                    size_t k = (kp - (char *) b) / s;
                    tp[j] = jp;
                    memcpy(jp, kp, s);
                    j = k;
                    jp = kp;
                    kp = (char *)tp[k];
                } while (kp != ip);

                tp[j] = jp;
                memcpy(jp, tmp_storage, s);
            }
        }
    } else {
        if ((s & (sizeof(xuint32) - 1)) == 0 && (xsize)(xuintptr)b % X_ALIGNOF(xuint32) == 0) {
            if (s == sizeof (xuint32)) {
                p.var = 0;
            } else if (s == sizeof(xuint64) && (xsize)(xuintptr)b % X_ALIGNOF(xuint64) == 0) {
                p.var = 1;
            } else if ((s & (sizeof(void *) - 1)) == 0 && (xsize)(xuintptr)b % X_ALIGNOF(void *) == 0) {
                p.var = 2;
            }
        }

        msort_with_tmp(&p, b, n);
    }

    x_free(tmp);
}

void x_qsort_with_data(xconstpointer pbase, xint total_elems, xsize size, XCompareDataFunc compare_func, xpointer user_data)
{
    msort_r((xpointer)pbase, total_elems, size, compare_func, user_data);
}
