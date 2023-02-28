#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>

#if defined(HAVE_POSIX_MEMALIGN) && !defined(_XOPEN_SOURCE)
# define _XOPEN_SOURCE                              600
#endif

#if defined(HAVE_MEMALIGN) || defined(HAVE__ALIGNED_MALLOC)
#include <malloc.h>
#endif

#ifdef HAVE__ALIGNED_MALLOC
#define aligned_alloc(alignment, size)              _aligned_malloc(size, alignment)
#define aligned_free(x)                             _aligned_free(x)
#else
#define aligned_free(x)                             free(x)
#endif

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xbacktrace.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xlib_trace.h>

static XMemVTable xlib_mem_vtable = {
    malloc,
    realloc,
    free,
    calloc,
    malloc,
    realloc,
};

xpointer x_malloc(xsize n_bytes)
{
    if (X_LIKELY(n_bytes)) {
        xpointer mem;

        mem = malloc(n_bytes);
        TRACE(XLIB_MEM_ALLOC((void *)mem, (unsigned int)n_bytes, 0, 0));
        if (mem) {
            return mem;
        }

        x_error("%s: failed to allocate %" X_XSIZE_FORMAT " bytes", X_STRLOC, n_bytes);
    }

    TRACE(XLIB_MEM_ALLOC((void *)NULL, (int) n_bytes, 0, 0));
    return NULL;
}

xpointer x_malloc0(xsize n_bytes)
{
    if (X_LIKELY(n_bytes)) {
        xpointer mem;

        mem = calloc(1, n_bytes);
        TRACE(XLIB_MEM_ALLOC((void *)mem, (unsigned int)n_bytes, 1, 0));
        if (mem) {
            return mem;
        }

        x_error("%s: failed to allocate %" X_XSIZE_FORMAT " bytes", X_STRLOC, n_bytes);
    }

    TRACE(XLIB_MEM_ALLOC((void*) NULL, (int) n_bytes, 1, 0));
    return NULL;
}

xpointer x_realloc(xpointer mem, xsize n_bytes)
{
    xpointer newmem;

    if (X_LIKELY(n_bytes)) {
        newmem = realloc(mem, n_bytes);
        TRACE(XLIB_MEM_REALLOC((void *)newmem, (void *)mem, (unsigned int)n_bytes, 0));
        if (newmem) {
            return newmem;
        }

        x_error("%s: failed to allocate %" X_XSIZE_FORMAT " bytes", X_STRLOC, n_bytes);
    }

    free(mem);
    TRACE(XLIB_MEM_REALLOC((void *)NULL, (void *)mem, 0, 0));

    return NULL;
}

void x_free(xpointer mem)
{
    free(mem);
    TRACE(XLIB_MEM_FREE((void *)mem));
}

void x_free_sized(void *mem, size_t size)
{
#ifdef HAVE_FREE_SIZED
    free_sized(mem, size);
#else
    free(mem);
#endif
    TRACE(XLIB_MEM_FREE((void *)mem));
}

#undef x_clear_pointer
void x_clear_pointer(xpointer *pp, XDestroyNotify destroy)
{
    xpointer _p;

    _p = *pp;
    if (_p) {
        *pp = NULL;
        destroy(_p);
    }
}

xpointer x_try_malloc(xsize n_bytes)
{
    xpointer mem;

    if (X_LIKELY(n_bytes)) {
        mem = malloc(n_bytes);
    } else {
        mem = NULL;
    }

    TRACE(XLIB_MEM_ALLOC((void *)mem, (unsigned int)n_bytes, 0, 1));
    return mem;
}

xpointer x_try_malloc0(xsize n_bytes)
{
    xpointer mem;

    if (X_LIKELY(n_bytes)) {
        mem = calloc(1, n_bytes);
    } else {
        mem = NULL;
    }

    return mem;
}

xpointer x_try_realloc(xpointer mem, xsize n_bytes)
{
    xpointer newmem;

    if (X_LIKELY(n_bytes)) {
        newmem = realloc(mem, n_bytes);
    } else {
        newmem = NULL;
        free(mem);
    }

    TRACE(XLIB_MEM_REALLOC((void *)newmem, (void *)mem, (unsigned int)n_bytes, 1));
    return newmem;
}

#define SIZE_OVERFLOWS(a, b)            (X_UNLIKELY((b) > 0 && (a) > X_MAXSIZE / (b)))

xpointer x_malloc_n(xsize n_blocks, xsize n_block_bytes)
{
    if (SIZE_OVERFLOWS(n_blocks, n_block_bytes)) {
        x_error("%s: overflow allocating %" X_XSIZE_FORMAT "*%" X_XSIZE_FORMAT " bytes", X_STRLOC, n_blocks, n_block_bytes);
    }

    return x_malloc(n_blocks * n_block_bytes);
}

xpointer x_malloc0_n(xsize n_blocks, xsize n_block_bytes)
{
    if (SIZE_OVERFLOWS(n_blocks, n_block_bytes)) {
        x_error("%s: overflow allocating %" X_XSIZE_FORMAT "*%" X_XSIZE_FORMAT " bytes", X_STRLOC, n_blocks, n_block_bytes);
    }

    return x_malloc0(n_blocks * n_block_bytes);
}

xpointer x_realloc_n(xpointer mem, xsize n_blocks, xsize n_block_bytes)
{
    if (SIZE_OVERFLOWS(n_blocks, n_block_bytes)) {
        x_error("%s: overflow allocating %" X_XSIZE_FORMAT "*%" X_XSIZE_FORMAT " bytes", X_STRLOC, n_blocks, n_block_bytes);
    }

    return x_realloc(mem, n_blocks * n_block_bytes);
}

xpointer x_try_malloc_n(xsize n_blocks, xsize n_block_bytes)
{
    if (SIZE_OVERFLOWS(n_blocks, n_block_bytes)) {
        return NULL;
    }

    return x_try_malloc(n_blocks * n_block_bytes);
}

xpointer x_try_malloc0_n(xsize n_blocks, xsize n_block_bytes)
{
    if (SIZE_OVERFLOWS(n_blocks, n_block_bytes)) {
        return NULL;
    }

    return x_try_malloc0(n_blocks * n_block_bytes);
}

xpointer x_try_realloc_n(xpointer mem, xsize n_blocks, xsize n_block_bytes)
{
    if (SIZE_OVERFLOWS(n_blocks, n_block_bytes)) {
        return NULL;
    }

    return x_try_realloc(mem, n_blocks * n_block_bytes);
}

xboolean x_mem_is_system_malloc(void)
{
    return TRUE;
}

void _mem_set_vtable(XMemVTable *vtable)
{
    x_warning(X_STRLOC ": custom memory allocation vtable not supported");
}

XMemVTable *xlib_mem_profiler_table = &xlib_mem_vtable;

void x_mem_profile (void)
{
    x_warning(X_STRLOC ": memory profiling not supported");
}

xpointer x_aligned_alloc(xsize n_blocks, xsize n_block_bytes, xsize alignment)
{
    xsize real_size;
    xpointer res = NULL;

    if (X_UNLIKELY((alignment == 0) || (alignment & (alignment - 1)) != 0)) {
        x_error("%s: alignment %" X_XSIZE_FORMAT " must be a positive power of two", X_STRLOC, alignment);
    }

    if (X_UNLIKELY((alignment % sizeof(void *)) != 0)) {
        x_error("%s: alignment %" X_XSIZE_FORMAT " must be a multiple of %" X_XSIZE_FORMAT, X_STRLOC, alignment, sizeof (void *));
    }

    if (SIZE_OVERFLOWS(n_blocks, n_block_bytes)) {
        x_error("%s: overflow allocating %" X_XSIZE_FORMAT "*%" X_XSIZE_FORMAT " bytes", X_STRLOC, n_blocks, n_block_bytes);
    }

    real_size = n_blocks * n_block_bytes;
    if (X_UNLIKELY(real_size == 0)) {
        TRACE(XLIB_MEM_ALLOC((void *)NULL, (int)real_size, 0, 0));
        return NULL;
    }

    errno = 0;

#if defined(HAVE_POSIX_MEMALIGN)
    errno = posix_memalign (&res, alignment, real_size);
#elif defined(HAVE_ALIGNED_ALLOC) || defined(HAVE__ALIGNED_MALLOC)
    if (real_size % alignment != 0) {
        xsize offset = real_size % alignment;

        if ((X_MAXSIZE - real_size) < (alignment - offset)) {
            x_error("%s: overflow allocating %" X_XSIZE_FORMAT "+%" X_XSIZE_FORMAT " bytes", X_STRLOC, real_size, (alignment - offset));
        }

        real_size += (alignment - offset);
    }

    res = aligned_alloc(alignment, real_size);
#elif defined(HAVE_MEMALIGN)
    res = memalign(alignment, real_size);
#else
#error "This platform does not have an aligned memory allocator."
#endif

    TRACE(XLIB_MEM_ALLOC((void *)res, (unsigned int)real_size, 0, 0));
    if (res) {
        return res;
    }

    x_error("%s: failed to allocate %" X_XSIZE_FORMAT " bytes", X_STRLOC, real_size);
    return NULL;
}

xpointer x_aligned_alloc0(xsize n_blocks, xsize n_block_bytes, xsize alignment)
{
    xpointer res = x_aligned_alloc(n_blocks, n_block_bytes, alignment);

    if (X_LIKELY(res != NULL)) {
        memset(res, 0, n_blocks * n_block_bytes);
    }

    return res;
}

void x_aligned_free(xpointer mem)
{
    aligned_free(mem);
}

void x_aligned_free_sized(void *mem, size_t alignment, size_t size)
{
#ifdef HAVE_FREE_ALIGNED_SIZED
    free_aligned_sized(mem, alignment, size);
#else
    aligned_free(mem);
#endif
}
