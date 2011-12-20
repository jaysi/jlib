#ifndef _J_COMPAT_H
#define _J_COMPAT_H

#include <assert.h>

#ifdef _J_USE_PTHREAD

#include <pthread.h>
#include <semaphore.h>

typedef pthread_mutex_t jmx_t;
typedef pthread_t jthread_t;
typedef pthread_cond_t jcond_t;
typedef pthread_mutexattr_t jmxattr_t;
typedef pthread_condattr_t jcondattr_t;
typedef sem_t jsem_t;

#ifndef NDEBUG
#define _init_mx(mx, attr) assert(!pthread_mutex_init(mx, attr))
#define _destroy_mx(mx) assert(!pthread_mutex_destroy(mx))
#else
#define _init_mx(mx, attr) pthread_mutex_init(mx, attr)
#define _destroy_mx(mx) pthread_mutex_destroy(mx)
#endif

#ifndef NDEBUG
#define _lock_mx(mx) assert(!pthread_mutex_lock(mx))
#define _unlock_mx(mx) assert(!pthread_mutex_unlock(mx))
#else
#define _lock_mx(mx) pthread_mutex_lock(mx)
#define _unlock_mx(mx) pthread_mutex_unlock(mx)
#endif

#define _create_thread(th, arg1, fname, farg) pthread_create(th, arg1, fname, farg)
#define _cancel_thread(th) pthread_cancel(th)
#define _join_thread(th, arg) pthread_join(th, arg)
#define _get_thread_id() pthread_self()
#define _init_cond(cond, attr) pthread_cond_init(cond, attr)
#define _destroy_cond(cond) pthread_cond_destroy(cond)
#define _wait_cond(cond, mx) pthread_cond_wait(cond, mx)
#define _timed_wait_cond(cond, mx, ts) pthread_cond_timedwait(cond, mx, ts)
#define _signal_cond(cond) pthread_cond_signal(cond)
#define _push_cleanup(fn, ar) pthread_cleanup_push(fn, ar)
#define _pop_cleanup(val) pthread_cleanup_pop(val)
#define _init_sem(sem) sem_init(sem, 0, 0)
#define _wait_sem(sem) sem_wait(sem)
#define _post_sem(sem) sem_post(sem)
#define _try_sem(sem) sem_trywait(sem)
#define _destroy_sem(sem) sem_destroy(sem)

#else

#ifdef _WIN32

#define jmx_t HANDLE
#define jthread_t int
#define jcond_t int
#define jmxattr_t int
#define jcondattr_t int

#define _init_mx(mx, attr) (mx=::CreateMutex(attr, false, 0))
#define _destroy_mx(mx) ::CloseHandle(mx)
#define _lock_mx(mx) WaitForSingleObject(mx, INFINITE)
#define _unlock_mx(mx) ReleaseMutex(mx)
#define _cancel_thread(th)
#define _init_cond(cond, attr)
#define _destroy_cond(cond)

#else

#define jmx_t int
#define jthread_t int
#define jcond_t int
#define jmxattr_t int

#define _init_mx(mx, attr)
#define _destroy_mx(mx)
#define _lock_mx(mx)
#define _unlock_mx(mx)
#define _cancel_thread(th)

#endif

#endif

#endif
