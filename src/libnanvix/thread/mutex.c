/*
 * MIT License
 *
 * Copyright(c) 2011-2020 The Maintainers of Nanvix
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Must come first. */
#define __NEED_SECTION_GUARD

#include <nanvix/hal/section_guard.h>
#include <nanvix/kernel/kernel.h>
#include <nanvix/sys/mutex.h>
#include <posix/errno.h>

#if (CORES_NUM > 1)

/*============================================================================*
 * nanvix_mutex_init()                                                        *
 *============================================================================*/

/**
 * @brief Initializes a mutex.
 *
 * @param m Target mutex.
 *
 * @param Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
int nanvix_mutex_init(struct nanvix_mutex *m)
{
	/* Invalid mutex. */
	if (m == NULL)
		return (-EINVAL);

	m->locked = false;
	spinlock_init(&m->lock);

	#if (__NANVIX_MUTEX_SLEEP)

		spinlock_init(&m->lock2);

		for (int i = 0; i < THREAD_MAX; i++)
			m->tids[i] = -1;

	#endif /* __NANVIX_MUTEX_SLEEP */

	dcache_invalidate();

	return (0);
}

/*============================================================================*
 * nanvix_mutex_lock()                                                        *
 *============================================================================*/

/**
 * @brief Locks a mutex.
 *
 * @param m Target mutex.
 *
 * @param Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
int nanvix_mutex_lock(struct nanvix_mutex *m)
{
	struct section_guard guard; /* Section guard. */

	#if (__NANVIX_MUTEX_SLEEP)

		kthread_t tid;

	#endif /* __NANVIX_MUTEX_SLEEP */

	/* Invalid mutex. */
	if (UNLIKELY(m == NULL))
		return (-EINVAL);

	#if (__NANVIX_MUTEX_SLEEP)

		tid = kthread_self();

	#endif /* __NANVIX_MUTEX_SLEEP */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &m->lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);

	#if (__NANVIX_MUTEX_SLEEP)

		/* Enqueue kernel thread. */
		for (int i = 0; i < THREAD_MAX; i++)
		{
			if (m->tids[i] == -1)
			{
				m->tids[i] = tid;
				break;
			}
		}

	#endif /* __NANVIX_MUTEX_SLEEP */

	do
	{
			/* Lock free and its my time. */
			if (LIKELY(m->tids[0] == tid && !m->locked))
			{
				m->locked = true;
				break;
			}

		section_guard_exit(&guard);

			#if (__NANVIX_MUTEX_SLEEP)

					ksleep();

			#else

					/**
					 * Decreases the competition and allows another thread
					 * to release the lock.
					 */
					int busy = 10;
					while (busy--);

			#endif /* __NANVIX_MUTEX_SLEEP */

		section_guard_entry(&guard);

	} while (LIKELY(true));

	#if (__NANVIX_MUTEX_SLEEP)

		KASSERT(m->tids[0] == tid);

		/* Dequeue kernel thread. */
		for (int i = 0; i < (THREAD_MAX - 1); i++)
			m->tids[i] = m->tids[i + 1];
		m->tids[THREAD_MAX - 1] = -1;

	#endif /* __NANVIX_MUTEX_SLEEP */

	section_guard_exit(&guard);

	return (0);
}

/*============================================================================*
 * nanvix_mutex_unlock()                                                      *
 *============================================================================*/

/**
 * @brief unlocks a mutex.
 *
 * @param m Target mutex.
 *
 * @param Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
int nanvix_mutex_unlock(struct nanvix_mutex *m)
{
	struct section_guard guard; /* Section guard. */

	/* Invalid mutex. */
	if (UNLIKELY(m == NULL))
		return (-EINVAL);

#if (__NANVIX_MUTEX_SLEEP)
	spinlock_lock(&m->lock2);
#endif /* __NANVIX_MUTEX_SLEEP */

	/* Prevent this call be preempted by any maskable interrupt. */
	section_guard_init(&guard, &m->lock, INTERRUPT_LEVEL_NONE);
	section_guard_entry(&guard);
		m->locked = false;

#if (__NANVIX_MUTEX_SLEEP)
		int ret = -1;
		int head;

		if ((head = m->tids[0]) >= 0)
		{
			/* Dequeue thread. */
			while (ret < 0 && !m->locked && m->tids[0] == head)
			{
				section_guard_exit(&guard);
					ret = kwakeup(head);
				section_guard_entry(&guard);
			}
		}
#endif /* __NANVIX_MUTEX_SLEEP */

	section_guard_exit(&guard);

#if (__NANVIX_MUTEX_SLEEP)
	spinlock_unlock(&m->lock2);
#endif /* __NANVIX_MUTEX_SLEEP */

	return (0);
}

#endif /* CORES_NUM > 1 */
