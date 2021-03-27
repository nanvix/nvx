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
#include <nanvix/sys/semaphore.h>
#include <posix/errno.h>

#if (CORES_NUM > 1)

/*============================================================================*
 * nanvix_semaphore_init()                                                    *
 *============================================================================*/

/**
 * @brief Initializes a semaphore.
 *
 * @param sem Target semaphore.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_semaphore_init(struct nanvix_semaphore * sem, int val)
{
	/* Invalid semaphore. */
	if (sem == NULL)
		return (-EINVAL);

	/* Invalid semaphore value. */
	if (val < 0)
		return (-EINVAL);

	sem->val = val;
	spinlock_init(&sem->lock);

	#if (__NANVIX_SEMAPHORE_SLEEP)

		sem->begin = 0;
		sem->end   = 0;
		sem->size  = 0;

		for (int i = 0; i < THREAD_MAX; ++i)
			sem->tids[i] = -1;

		spinlock_init(&sem->lock2);

	#endif /* __NANVIX_SEMAPHORE_SLEEP */

	dcache_invalidate();

	return (0);
}

/*============================================================================*
 * nanvix_semaphore_down()                                                    *
 *============================================================================*/


/**
 * @brief Performs a down operation on a semaphore.
 *
 * @param sem Target semaphore.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_semaphore_down(struct nanvix_semaphore * sem)
{
	#if (__NANVIX_SEMAPHORE_SLEEP)

		kthread_t tid;

	#endif /* __NANVIX_SEMAPHORE_SLEEP */

	/* Invalid semaphore. */
	if (sem == NULL)
		return (-EINVAL);

	#if (__NANVIX_SEMAPHORE_SLEEP)

		tid = kthread_self();

	#endif /* __NANVIX_SEMAPHORE_SLEEP */

	do
	{
		spinlock_lock(&sem->lock);

			/* Lock. */
			if (sem->val > 0)
			{
				sem->val--;
				spinlock_unlock(&sem->lock);
				break;
			}

			#if (__NANVIX_SEMAPHORE_SLEEP)

				/* Sanity checks. */
				KASSERT(WITHIN(sem->size, 0, THREAD_MAX));
				#ifndef NDEBUG
					/* Double down? */
					for (int i = 0; i < THREAD_MAX; ++i)
						KASSERT(sem->tids[i] != tid);

					/* Overflow? */
					if (sem->size > 0)
						KASSERT(sem->tids[sem->end] == -1);
				#endif

				/* Enqueue. */
				sem->tids[sem->end] = tid;
				sem->end            = (sem->end + 1) % THREAD_MAX;
				sem->size++;

			#endif /* __NANVIX_SEMAPHORE_SLEEP */

		spinlock_unlock(&sem->lock);


		#if (__NANVIX_SEMAPHORE_SLEEP)

			ksleep();

		#else

			int busy = 10;
			while (--busy);

		#endif /* __NANVIX_SEMAPHORE_SLEEP */

	} while (true);

	return (0);
}

/*============================================================================*
 * nanvix_semaphore_up()                                                      *
 *============================================================================*/

/**
 * @brief Performs an up operation on a semaphore.
 *
 * @param sem Target semaphore.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_semaphore_up(struct nanvix_semaphore * sem)
{
	/* Invalid semaphore. */
	if (sem == NULL)
		return (-EINVAL);

#if (__NANVIX_SEMAPHORE_SLEEP)
	kthread_t tid = -1;
	spinlock_lock(&sem->lock2);
#endif /* __NANVIX_SEMAPHORE_SLEEP */

		spinlock_lock(&sem->lock);

			sem->val++;

			#if (__NANVIX_SEMAPHORE_SLEEP)
				/**
				 * Remove the head of the queue.
				 */
				if (sem->size > 0)
				{
					/* Gets the sleep thread id. */
					tid = sem->tids[sem->begin];
					sem->tids[sem->begin] = -1;

					/* Dequeue. */
					sem->begin = (sem->begin + 1) % THREAD_MAX;
					sem->size--;

					KASSERT(tid >= 0);
				}
			#endif /* __NANVIX_SEMAPHORE_SLEEP */

		spinlock_unlock(&sem->lock);

#if (__NANVIX_SEMAPHORE_SLEEP)
		/**
		 * May be we need try to wakeup a thread more than one time because it
		 * is not an atomic sleep/wakeup.
		 *
		 * Obs.: We release the primary lock because we don't need garantee
		 * that the head thread gets the mutex.
		 */
		if (tid != -1)
			while (LIKELY(kwakeup(tid) != 0));

	spinlock_unlock(&sem->lock2);
#endif

	return (0);
}

/**
 * @brief Try to perform a down operation on a semaphore.
 *
 * @param sem Target semaphore.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_semaphore_trywait(struct nanvix_semaphore * sem)
{
	int ret;

	if (sem == NULL)
		return (-EINVAL);

	ret =  (-EINVAL);

	spinlock_lock(&sem->lock);

		if (sem->val > 0)
		{
			sem->val--;
			ret = (0);
		}

	spinlock_unlock(&sem->lock);

	return (ret);
}

/**
 * @brief Destroy a semaphore
 *
 * @param sem Target semaphore.
 *
 * @return Upon sucessful completion, zero is returned. Upon failure, a
 * negative error code is returned instead.
 */
PUBLIC int nanvix_semaphore_destroy(struct nanvix_semaphore * sem)
{
	if (!sem)
		return (-EINVAL);

	#if (__NANVIX_SEMAPHORE_SLEEP)
		/* The thread queue must be empty. */
		KASSERT(sem->size == 0);
	#endif

	/**
	 * Sets -INT_MAX because we force any thread to block.
	 *
	 * @TODO We must implement a way to detect an uninitialized semaphore.
	 */
	sem->val = -2147483648;

	return (0);
}

#endif /* CORES_NUM > 1 */

