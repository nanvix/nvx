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
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutex_init(struct nanvix_mutex * m, struct nanvix_mutexattr * mattr)
{
	/* Invalid mutex. */
	if (m == NULL)
		return (-EINVAL);

	m->locked = false;
	m->owner = -1;
	m->rlevel = 0;
	spinlock_init(&m->lock);

	if (mattr)
		m->type = mattr->type;
	else
		m->type = NANVIX_MUTEX_NORMAL;

	#if (__NANVIX_MUTEX_SLEEP)

		m->begin = 0;
		m->end   = 0;
		m->size  = 0;

		for (int i = 0; i < THREAD_MAX; i++)
			m->tids[i] = -1;

		spinlock_init(&m->lock2);

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
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutex_lock(struct nanvix_mutex * m)
{
	kthread_t tid;

	/* Invalid mutex. */
	if (UNLIKELY(m == NULL))
		return (-EINVAL);

	tid = kthread_self();

	if (m->type == NANVIX_MUTEX_ERRORCHECK && m->owner == tid)
		return (-EDEADLK);

	if (m->type == NANVIX_MUTEX_RECURSIVE && m->owner == tid)
	{
		m->rlevel++;
		return (0);
	}

	do
	{
		spinlock_lock(&m->lock);

			/* Lock. */
			if (LIKELY(!m->locked))
			{
				m->locked = true;
				m->owner = tid;
				if (m->type == NANVIX_MUTEX_RECURSIVE)
					m->rlevel++;
				spinlock_unlock(&m->lock);
				break;
			}

			#if (__NANVIX_MUTEX_SLEEP)

				KASSERT(m->size < THREAD_MAX);

				m->tids[m->end] = tid;
				m->end          = (m->end + 1) % THREAD_MAX;
				m->size++;

			#endif /* __NANVIX_MUTEX_SLEEP */

		spinlock_unlock(&m->lock);

		#if (__NANVIX_MUTEX_SLEEP)

			ksleep();

		#endif /* __NANVIX_MUTEX_SLEEP */

	} while (LIKELY(true));

	return (0);
}

/*============================================================================*
 * nanvix_mutex_trylock()                                                     *
 *============================================================================*/

/**
 * @brief Tries to lock a mutex.
 *
 * @param m Target mutex.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutex_trylock(struct nanvix_mutex * m)
{
	int ret;
	kthread_t tid;

	/* Invalid mutex. */
	if (!m)
		return (-EINVAL);

	ret = (-EBUSY);
	tid = kthread_self();

	spinlock_lock(&m->lock);

		/* Locked? */
		if (m->locked)
		{
			if (m->type == NANVIX_MUTEX_RECURSIVE && m->owner == tid)
			{
				m->rlevel++;
				ret = (0);
			}
		}

		/* Not reserved? */
		else
		{
			if (m->type == NANVIX_MUTEX_RECURSIVE)
				m->rlevel++;
			m->owner  = tid;
			m->locked = true;
			ret = (0);
		}

	spinlock_unlock(&m->lock);

	return (ret);
 }


/*============================================================================*
 * nanvix_mutex_unlock()                                                      *
 *============================================================================*/

/**
 * @brief Unlocks a mutex.
 *
 * @param m Target mutex.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutex_unlock(struct nanvix_mutex * m)
{
	int ret = -EINVAL;
	kthread_t tid;

	/* Invalid mutex. */
	if (UNLIKELY(m == NULL))
		return (-EINVAL);

	tid = kthread_self();

#if (__NANVIX_MUTEX_SLEEP)
	int head = -1;
	spinlock_lock(&m->lock2);
#endif

		spinlock_lock(&m->lock);

			if (m->type == NANVIX_MUTEX_ERRORCHECK)
			{
				if (!m->locked || m->owner != tid)
					ret = (-EPERM);
			}
			else if (m->type == NANVIX_MUTEX_RECURSIVE)
			{
				if (m->rlevel > 0 && m->owner == tid)
				{
					m->rlevel--;
					if (m->rlevel != 0)
						ret = (0);
				}
				else
					ret = (-EPERM);
			}

			/* Some error or recursive option triggered. */
			if (ret != -EINVAL)
				goto exit;

			m->locked = false;
			m->owner  = -1;

#if (__NANVIX_MUTEX_SLEEP)
			/**
			 * Remove the head of the queue.
			 */
			if (m->size > 0)
			{

				head     = m->tids[m->begin];
				m->begin = (m->begin + 1) % THREAD_MAX;
				m->size--;
			}
#endif

			/* Success.*/
			ret = (0);
exit:
		spinlock_unlock(&m->lock);

#if (__NANVIX_MUTEX_SLEEP)
		/**
		 * May be we need try to wakeup a thread more than one time because it
		 * is not an atomic sleep/wakeup.
		 *
		 * Obs.: We release the primary lock because we don't need garantee
		 * that the head thread gets the mutex.
		 */
		if (head != -1)
			while (LIKELY(kwakeup(head) != 0));

	spinlock_unlock(&m->lock2);
#endif

	return (ret);
}

/*============================================================================*
 * nanvix_mutex_destroy()                                                     *
 *============================================================================*/

/**
 * @brief Destroy a mutex.
 *
 * @param m Target mutex.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutex_destroy(struct nanvix_mutex * m)
{
	/* Invalid mutex. */
	if (!m)
		return (-EINVAL);

	KASSERT(m->owner == -1);
	KASSERT(m->locked == false);
	#if (__NANVIX_MUTEX_SLEEP)
		KASSERT(m->size == 0);
	#endif

	m = NULL;
	return (0);
}

/*============================================================================*
 * nanvix_mutexattr_init()                                                    *
 *============================================================================*/

/**
 * @brief Initializes a mutex attribute.
 *
 * @param mattr Target mutex attribute.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutexattr_init(struct nanvix_mutexattr * mattr)
{
	/* Invalid attr. */
	if (!mattr)
		return (-EINVAL);

	mattr->type = NANVIX_MUTEX_DEFAULT;

	return(0);
}

/*============================================================================*
 * nanvix_mutexattr_destroy()                                                 *
 *============================================================================*/

/**
 * @brief Destroy a mutex attribute.
 *
 * @param mattr Target mutex attribute.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutexattr_destroy(struct nanvix_mutexattr * mattr)
{
	/* Invalid attr. */
	if (!mattr)
		return (-EINVAL);

	mattr = NULL;

	return (0);
}

/*============================================================================*
 * nanvix_mutexattr_settype()                                                 *
 *============================================================================*/

/**
 * @brief Set mutex attribute type.
 *
 * @param mattr Target mutex attribute.
 * @param type Mutex attribute type
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutexattr_settype(struct nanvix_mutexattr * mattr, int type)
{
	/* Invalid attr. */
	if (!mattr)
		return (-EINVAL);

	/* Invalid type. */
	if (!WITHIN(type, 0, NANVIX_MUTEX_LIMIT))
		return (-EINVAL);

	mattr->type = type;

	return (0);
}

/*============================================================================*
 * nanvix_mutexattr_gettype()                                                 *
 *============================================================================*/

/**
 * @brief Get mutex attribute type.
 *
 * @param mattr Target mutex attribute.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_mutexattr_gettype(struct nanvix_mutexattr * mattr)
{
	/* Invalid attr. */
	if (!mattr)
		return (-EINVAL);

	return (mattr->type);
}

#endif /* CORES_NUM > 1 */

