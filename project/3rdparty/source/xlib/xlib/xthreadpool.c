#include <xlib/xlib/config.h>
#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xtimer.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xthreadpool.h>
#include <xlib/xlib/xasyncqueue.h>
#include <xlib/xlib/xlib-private.h>
#include <xlib/xlib/xthreadprivate.h>
#include <xlib/xlib/xasyncqueueprivate.h>

#define DEBUG_MSG(x)

typedef struct _XRealThreadPool XRealThreadPool;

struct _XRealThreadPool {
    XThreadPool      pool;
    XAsyncQueue      *queue;
    XCond            cond;
    xint             max_threads;
    xuint            num_threads;
    xboolean         running;
    xboolean         immediate;
    xboolean         waiting;
    XCompareDataFunc sort_func;
    xpointer         sort_user_data;
};

static xint wakeup_thread_serial = 0;
static const xpointer wakeup_thread_marker = (xpointer)&x_thread_pool_new;

static xint unused_threads = 0;
static xint max_unused_threads = 2;
static xint kill_unused_threads = 0;
static xuint max_idle_time = 15 * 1000;
static XAsyncQueue *unused_thread_queue = NULL;

typedef struct {
    XThreadPool *pool;
    XThread     *thread;
    XError      *error;
} SpawnThreadData;

static XCond spawn_thread_cond;
static XAsyncQueue *spawn_thread_queue;

static void x_thread_pool_queue_push_unlocked(XRealThreadPool *pool, xpointer data);
static void x_thread_pool_free_internal(XRealThreadPool *pool);
static xpointer x_thread_pool_thread_proxy(xpointer data);
static xboolean x_thread_pool_start_thread(XRealThreadPool *pool, XError **error);
static void x_thread_pool_wakeup_and_stop_all(XRealThreadPool *pool);
static XRealThreadPool *x_thread_pool_wait_for_new_pool(void);
static xpointer x_thread_pool_wait_for_new_task(XRealThreadPool *pool);

static void x_thread_pool_queue_push_unlocked(XRealThreadPool *pool, xpointer data)
{
    if (pool->sort_func) {
        x_async_queue_push_sorted_unlocked(pool->queue, data, pool->sort_func, pool->sort_user_data);
    } else {
        x_async_queue_push_unlocked(pool->queue, data);
    }
}

static XRealThreadPool *x_thread_pool_wait_for_new_pool(void)
{
    XRealThreadPool *pool;
    xint local_max_idle_time;
    xuint local_max_unused_threads;
    xint last_wakeup_thread_serial;
    xint local_wakeup_thread_serial;
    xboolean have_relayed_thread_marker = FALSE;

    local_max_unused_threads = (xuint)x_atomic_int_get(&max_unused_threads);
    local_max_idle_time = x_atomic_int_get(&max_idle_time);
    last_wakeup_thread_serial = x_atomic_int_get(&wakeup_thread_serial);

    do {
        if ((xuint)x_atomic_int_get(&unused_threads) >= local_max_unused_threads) {
            pool = NULL;
        } else if (local_max_idle_time > 0) {
            DEBUG_MSG(("thread %p waiting in global pool for %f seconds.", x_thread_self(), local_max_idle_time / 1000.0));
            pool = (XRealThreadPool *)x_async_queue_timeout_pop(unused_thread_queue, local_max_idle_time * 1000);
        } else {
            DEBUG_MSG(("thread %p waiting in global pool.", x_thread_self()));
            pool = (XRealThreadPool *)x_async_queue_pop(unused_thread_queue);
        }

        if (pool == wakeup_thread_marker) {
            local_wakeup_thread_serial = x_atomic_int_get(&wakeup_thread_serial);
            if (last_wakeup_thread_serial == local_wakeup_thread_serial) {
                if (!have_relayed_thread_marker) {
                    DEBUG_MSG(("thread %p relaying wakeup message to waiting thread with lower serial.", x_thread_self()));

                    x_async_queue_push(unused_thread_queue, wakeup_thread_marker);
                    have_relayed_thread_marker = TRUE;
                    x_usleep(100);
                }
            } else {
                if (x_atomic_int_add(&kill_unused_threads, -1) > 0) {
                    pool = NULL;
                    break;
                }

                DEBUG_MSG(("thread %p updating to new limits.", x_thread_self()));

                local_max_unused_threads = (xuint)x_atomic_int_get(&max_unused_threads);
                local_max_idle_time = x_atomic_int_get(&max_idle_time);
                last_wakeup_thread_serial = local_wakeup_thread_serial;

                have_relayed_thread_marker = FALSE;
            }
        }
    } while (pool == wakeup_thread_marker);

    return pool;
}

static xpointer x_thread_pool_wait_for_new_task(XRealThreadPool *pool)
{
    xpointer task = NULL;

    if (pool->running || (!pool->immediate && x_async_queue_length_unlocked(pool->queue) > 0)) {
        if (pool->max_threads != -1 && pool->num_threads > (xuint)pool->max_threads) {
            DEBUG_MSG(("superfluous thread %p in pool %p.",
                        x_thread_self(), pool));
        } else if (pool->pool.exclusive) {
            task = x_async_queue_pop_unlocked(pool->queue);

            DEBUG_MSG(("thread %p in exclusive pool %p waits for task (%d running, %d unprocessed).",
                x_thread_self(), pool, pool->num_threads, x_async_queue_length_unlocked(pool->queue)));
        } else {
            DEBUG_MSG(("thread %p in pool %p waits for up to a 1/2 second for task (%d running, %d unprocessed).",
                x_thread_self(), pool, pool->num_threads, x_async_queue_length_unlocked(pool->queue)));

            task = x_async_queue_timeout_pop_unlocked(pool->queue, X_USEC_PER_SEC / 2);
        }
    } else {
        DEBUG_MSG(("pool %p not active, thread %p will go to global pool (running: %s, immediate: %s, len: %d).",
            pool, x_thread_self(), pool->running ? "true" : "false", pool->immediate ? "true" : "false", x_async_queue_length_unlocked (pool->queue)));
    }

    return task;
}

static xpointer x_thread_pool_spawn_thread(xpointer data)
{
    while (TRUE) {
        XError *error = NULL;
        XThread *thread = NULL;
        xchar name[16] = "pool";
        SpawnThreadData *spawn_thread_data;
        const xchar *prgname = x_get_prgname();

        if (prgname) {
            x_snprintf(name, sizeof (name), "pool-%s", prgname);
        }

        x_async_queue_lock(spawn_thread_queue);
        spawn_thread_data = (SpawnThreadData *)x_async_queue_pop_unlocked(spawn_thread_queue);
        thread = x_thread_try_new(name, x_thread_pool_thread_proxy, spawn_thread_data->pool, &error);

        spawn_thread_data->thread = x_steal_pointer(&thread);
        spawn_thread_data->error = x_steal_pointer(&error);

        x_cond_broadcast(&spawn_thread_cond);
        x_async_queue_unlock(spawn_thread_queue);
    }

    return NULL;
}

static xpointer x_thread_pool_thread_proxy(xpointer data)
{
    XRealThreadPool *pool;

    pool = (XRealThreadPool *)data;
    DEBUG_MSG(("thread %p started for pool %p.", x_thread_self(), pool));

    x_async_queue_lock(pool->queue);

    while (TRUE) {
        xpointer task;

        task = x_thread_pool_wait_for_new_task(pool);
        if (task) {
            if (pool->running || !pool->immediate) {
                x_async_queue_unlock(pool->queue);
                DEBUG_MSG(("thread %p in pool %p calling func.", x_thread_self(), pool));
                pool->pool.func (task, pool->pool.user_data);
                x_async_queue_lock(pool->queue);
            }
        } else {
            xboolean free_pool = FALSE;

            DEBUG_MSG(("thread %p leaving pool %p for global pool.", x_thread_self(), pool));
            pool->num_threads--;

            if (!pool->running) {
                if (!pool->waiting) {
                    if (pool->num_threads == 0) {
                        free_pool = TRUE;
                    } else {
                        if (x_async_queue_length_unlocked(pool->queue) == (xint) -pool->num_threads) {
                            x_thread_pool_wakeup_and_stop_all(pool);
                        }
                    }
                } else if (pool->immediate || x_async_queue_length_unlocked(pool->queue) <= 0) {
                    x_cond_broadcast(&pool->cond);
                }
            }

            x_atomic_int_inc(&unused_threads);
            x_async_queue_unlock(pool->queue);

            if (free_pool) {
                x_thread_pool_free_internal(pool);
            }

            pool = x_thread_pool_wait_for_new_pool();
            x_atomic_int_add(&unused_threads, -1);

            if (pool == NULL) {
                break;
            }

            x_async_queue_lock(pool->queue);

            DEBUG_MSG(("thread %p entering pool %p from global pool.", x_thread_self(), pool));
        }
    }

    return NULL;
}

static xboolean x_thread_pool_start_thread(XRealThreadPool *pool, XError **error)
{
    xboolean success = FALSE;

    if (pool->max_threads != -1 && pool->num_threads >= (xuint)pool->max_threads) {
        return TRUE;
    }

    x_async_queue_lock(unused_thread_queue);

    if (x_async_queue_length_unlocked(unused_thread_queue) < 0) {
        x_async_queue_push_unlocked(unused_thread_queue, pool);
        success = TRUE;
    }

    x_async_queue_unlock(unused_thread_queue);

    if (!success) {
        XThread *thread;
        xchar name[16] = "pool";
        const xchar *prgname = x_get_prgname();

        if (prgname) {
            x_snprintf(name, sizeof (name), "pool-%s", prgname);
        }

        if (pool->pool.exclusive) {
            thread = x_thread_try_new(name, x_thread_pool_thread_proxy, pool, error);
        } else {
            SpawnThreadData spawn_thread_data = { (XThreadPool *)pool, NULL, NULL };

            x_async_queue_lock(spawn_thread_queue);
            x_async_queue_push_unlocked(spawn_thread_queue, &spawn_thread_data);

            while (!spawn_thread_data.thread && !spawn_thread_data.error) {
                x_cond_wait(&spawn_thread_cond, _x_async_queue_get_mutex(spawn_thread_queue));
            }

            thread = spawn_thread_data.thread;
            if (!thread) {
                x_propagate_error(error, x_steal_pointer(&spawn_thread_data.error));
            }
            x_async_queue_unlock(spawn_thread_queue);
        }

        if (thread == NULL) {
            return FALSE;
        }

        x_thread_unref(thread);
    }

    pool->num_threads++;
    return TRUE;
}

XThreadPool *x_thread_pool_new(XFunc func, xpointer user_data, xint max_threads, xboolean exclusive, XError **error)
{
    return x_thread_pool_new_full(func, user_data, NULL, max_threads, exclusive, error);
}

XThreadPool *x_thread_pool_new_full(XFunc func, xpointer user_data, XDestroyNotify item_free_func, xint max_threads, xboolean exclusive, XError **error)
{
    XRealThreadPool *retval;
    X_LOCK_DEFINE_STATIC(init);

    x_return_val_if_fail(func, NULL);
    x_return_val_if_fail(!exclusive || max_threads != -1, NULL);
    x_return_val_if_fail(max_threads >= -1, NULL);

    retval = x_new(XRealThreadPool, 1);

    retval->pool.func = func;
    retval->pool.user_data = user_data;
    retval->pool.exclusive = exclusive;
    retval->queue = x_async_queue_new_full(item_free_func);
    x_cond_init(&retval->cond);
    retval->max_threads = max_threads;
    retval->num_threads = 0;
    retval->running = TRUE;
    retval->immediate = FALSE;
    retval->waiting = FALSE;
    retval->sort_func = NULL;
    retval->sort_user_data = NULL;

    X_LOCK(init);
    if (!unused_thread_queue) {
        unused_thread_queue = x_async_queue_new();
    }

    if (!exclusive && !spawn_thread_queue) {
        XThread *pool_spawner = NULL;

        spawn_thread_queue = x_async_queue_new();
        x_cond_init(&spawn_thread_cond);
        pool_spawner = x_thread_new("pool-spawner", x_thread_pool_spawn_thread, NULL);
        x_ignore_leak(pool_spawner);
    }
    X_UNLOCK(init);

    if (retval->pool.exclusive) {
        x_async_queue_lock(retval->queue);
        while (retval->num_threads < (xuint) retval->max_threads) {
            XError *local_error = NULL;

            if (!x_thread_pool_start_thread(retval, &local_error)) {
                x_propagate_error(error, local_error);
                break;
            }
        }
        x_async_queue_unlock(retval->queue);
    }

    return (XThreadPool*) retval;
}

xboolean x_thread_pool_push(XThreadPool *pool, xpointer data, XError **error)
{
    xboolean result;
    XRealThreadPool *real;

    real = (XRealThreadPool*)pool;

    x_return_val_if_fail(real, FALSE);
    x_return_val_if_fail(real->running, FALSE);

    result = TRUE;

    x_async_queue_lock(real->queue);

    if (x_async_queue_length_unlocked(real->queue) >= 0) {
        XError *local_error = NULL;

        if (!x_thread_pool_start_thread(real, &local_error)) {
            x_propagate_error(error, local_error);
            result = FALSE;
        }
    }

    x_thread_pool_queue_push_unlocked(real, data);
    x_async_queue_unlock(real->queue);

    return result;
}

xboolean x_thread_pool_set_max_threads(XThreadPool *pool, xint max_threads, XError **error)
{
    xint to_start;
    xboolean result;
    XRealThreadPool *real;

    real = (XRealThreadPool*) pool;

    x_return_val_if_fail(real, FALSE);
    x_return_val_if_fail(real->running, FALSE);
    x_return_val_if_fail(!real->pool.exclusive || max_threads != -1, FALSE);
    x_return_val_if_fail(max_threads >= -1, FALSE);

    result = TRUE;

    x_async_queue_lock(real->queue);

    real->max_threads = max_threads;
    if (pool->exclusive) {
        to_start = real->max_threads - real->num_threads;
    } else {
        to_start = x_async_queue_length_unlocked(real->queue);
    }

    for ( ; to_start > 0; to_start--) {
        XError *local_error = NULL;

        if (!x_thread_pool_start_thread(real, &local_error)) {
            x_propagate_error(error, local_error);
            result = FALSE;
            break;
        }
    }

    x_async_queue_unlock(real->queue);

    return result;
}

xint x_thread_pool_get_max_threads(XThreadPool *pool)
{
    xint retval;
    XRealThreadPool *real;

    real = (XRealThreadPool*)pool;

    x_return_val_if_fail(real, 0);
    x_return_val_if_fail(real->running, 0);

    x_async_queue_lock(real->queue);
    retval = real->max_threads;
    x_async_queue_unlock(real->queue);

    return retval;
}

xuint x_thread_pool_get_num_threads(XThreadPool *pool)
{
    xuint retval;
    XRealThreadPool *real;

    real = (XRealThreadPool*)pool;

    x_return_val_if_fail(real, 0);
    x_return_val_if_fail(real->running, 0);

    x_async_queue_lock(real->queue);
    retval = real->num_threads;
    x_async_queue_unlock(real->queue);

    return retval;
}

xuint x_thread_pool_unprocessed(XThreadPool *pool)
{
    xint unprocessed;
    XRealThreadPool *real;

    real = (XRealThreadPool *)pool;

    x_return_val_if_fail(real, 0);
    x_return_val_if_fail(real->running, 0);

    unprocessed = x_async_queue_length(real->queue);

    return MAX(unprocessed, 0);
}

void x_thread_pool_free(XThreadPool *pool, xboolean immediate, xboolean wait_)
{
    XRealThreadPool *real;
    real = (XRealThreadPool*)pool;

    x_return_if_fail(real);
    x_return_if_fail(real->running);

    x_return_if_fail(immediate || real->max_threads != 0 || x_async_queue_length(real->queue) == 0);

    x_async_queue_lock(real->queue);

    real->running = FALSE;
    real->immediate = immediate;
    real->waiting = wait_;

    if (wait_) {
        while (x_async_queue_length_unlocked(real->queue) != (xint) -real->num_threads && !(immediate && real->num_threads == 0)) {
            x_cond_wait(&real->cond, _x_async_queue_get_mutex(real->queue));
        }
    }

    if (immediate || x_async_queue_length_unlocked(real->queue) == (xint) -real->num_threads) {
        if (real->num_threads == 0) {
            x_async_queue_unlock(real->queue);
            x_thread_pool_free_internal(real);
            return;
        }

        x_thread_pool_wakeup_and_stop_all(real);
    }

    real->waiting = FALSE;
    x_async_queue_unlock(real->queue);
}

static void x_thread_pool_free_internal(XRealThreadPool* pool)
{
    x_return_if_fail(pool);
    x_return_if_fail(pool->running == FALSE);
    x_return_if_fail(pool->num_threads == 0);

    x_async_queue_remove(pool->queue, XUINT_TO_POINTER(1));

    x_async_queue_unref(pool->queue);
    x_cond_clear(&pool->cond);

    x_free(pool);
}

static void x_thread_pool_wakeup_and_stop_all(XRealThreadPool *pool)
{
    xuint i;

    x_return_if_fail(pool);
    x_return_if_fail(pool->running == FALSE);
    x_return_if_fail(pool->num_threads != 0);

    pool->immediate = TRUE;
    for (i = 0; i < pool->num_threads; i++) {
        x_async_queue_push_unlocked(pool->queue, XUINT_TO_POINTER(1));
    }
}

void x_thread_pool_set_max_unused_threads(xint max_threads)
{
    x_return_if_fail(max_threads >= -1);

    x_atomic_int_set(&max_unused_threads, max_threads);

    if (max_threads != -1) {
        max_threads -= x_atomic_int_get(&unused_threads);
        if (max_threads < 0) {
            x_atomic_int_set(&kill_unused_threads, -max_threads);
            x_atomic_int_inc(&wakeup_thread_serial);

            x_async_queue_lock(unused_thread_queue);

            do {
                x_async_queue_push_unlocked(unused_thread_queue, wakeup_thread_marker);
            } while (++max_threads);

            x_async_queue_unlock(unused_thread_queue);
        }
    }
}

xint x_thread_pool_get_max_unused_threads(void)
{
    return x_atomic_int_get(&max_unused_threads);
}

xuint x_thread_pool_get_num_unused_threads(void)
{
    return (xuint)x_atomic_int_get(&unused_threads);
}

void x_thread_pool_stop_unused_threads(void)
{
    xuint oldval;

    oldval = x_thread_pool_get_max_unused_threads();
    x_thread_pool_set_max_unused_threads(0);
    x_thread_pool_set_max_unused_threads(oldval);
}

void x_thread_pool_set_sort_function(XThreadPool *pool, XCompareDataFunc func, xpointer user_data)
{
    XRealThreadPool *real;
    real = (XRealThreadPool*)pool;

    x_return_if_fail(real);
    x_return_if_fail(real->running);

    x_async_queue_lock(real->queue);

    real->sort_func = func;
    real->sort_user_data = user_data;

    if (func) {
        x_async_queue_sort_unlocked(real->queue, real->sort_func, real->sort_user_data);
    }

    x_async_queue_unlock(real->queue);
}

xboolean x_thread_pool_move_to_front(XThreadPool *pool, xpointer data)
{
    xboolean found;
    XRealThreadPool *real = (XRealThreadPool*)pool;

    x_async_queue_lock(real->queue);
    found = x_async_queue_remove_unlocked(real->queue, data);
    if (found) {
        x_async_queue_push_front_unlocked(real->queue, data);
    }
    x_async_queue_unlock(real->queue);

    return found;
}

void x_thread_pool_set_max_idle_time(xuint interval)
{
    xuint i;

    x_atomic_int_set(&max_idle_time, interval);

    i = (xuint)x_atomic_int_get(&unused_threads);
    if (i > 0) {
        x_atomic_int_inc(&wakeup_thread_serial);

        x_async_queue_lock(unused_thread_queue);
        do {
            x_async_queue_push_unlocked(unused_thread_queue, wakeup_thread_marker);
        } while (--i);
        x_async_queue_unlock(unused_thread_queue);
    }
}

xuint x_thread_pool_get_max_idle_time(void)
{
    return (xuint)x_atomic_int_get(&max_idle_time);
}
