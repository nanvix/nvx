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

#include <nanvix/sys/condvar.h>
#include <nanvix/kernel/kernel.h>
#include <posix/errno.h>

#if (CORES_NUM > 1)

/**
 * @brief Initializes a condition variable
 * @param cond Condition variable to be initialized
 * @return 0 if the condition variable was sucessfully initialized
 * or a negative code error if an error occurred
 */
int nanvix_cond_init(struct nanvix_cond_var *cond)
{
	if (!cond)
	{
		kprintf("Invalid condition variable");
		return (-EINVAL);
	}

	spinlock_init(&cond->lock);
	spinlock_init(&cond->lock2);

	cond->begin = 0;
	cond->end   = 0;
	cond->size  = 0;

	for (int i = 0; i < THREAD_MAX; i++)
		cond->tids[i] = -1;

	#if (!__NANVIX_CONDVAR_SLEEP)
		cond->locked = true;
	#endif  /* __NANVIX_CONDVAR_SLEEP */

	return (0);
}

/**
 * @brief Destroy a condition variable
 * @param cond Condition variable to destroy
 * @return 0 upon successfull completion or a negative error code
 * upon failure
 */
int nanvix_condvar_destroy(struct nanvix_cond_var *cond)
{
	if (!cond)
	{
		kprintf("Invalid condition variable");
		return (-EINVAL);
	}

	KASSERT(cond->size == 0);

	cond = NULL;

	return (0);
}

/**
 * @brief Block thread on a condition variable
 * @param cond Condition variable to wait for
 * @param mutex Mutex unlocked when waiting for a signal and locked when
 * the funtion return after a cond_signal call
 * @return 0 upon successfull completion or a negative error code
 * upon failure
 */
int nanvix_cond_wait(struct nanvix_cond_var *cond, struct nanvix_mutex *mutex)
{
	kthread_t tid;

	if (!cond)
	{
		kprintf("Invalid condition variable");
		return (-EINVAL);
	}

	if (!mutex)
	{
		kprintf("Invalid mutex");
		return (-EINVAL);
	}

	tid = kthread_self();

	spinlock_lock(&cond->lock);

		KASSERT(cond->size < THREAD_MAX);

		cond->tids[cond->end] = tid;
		cond->end             = (cond->end + 1) % THREAD_MAX;
		cond->size++;

	spinlock_unlock(&cond->lock);

	/* Releases @p mutex. */
	nanvix_mutex_unlock(mutex);

	#if (__NANVIX_CONDVAR_SLEEP)
		ksleep();
	#else
		while (1)
		{
			spinlock_lock(&cond->lock);
				if ((cond->tids[cond->begin] == tid) && (!cond->locked))
				{
					cond->locked = true;
					break;
				}
			spinlock_unlock(&cond->lock);
		}

		spinlock_unlock(&cond->lock);
	#endif  /* __NANVIX_CONDVAR_SLEEP */

	/* Reacquire @p mutex. */
	nanvix_mutex_lock(mutex);

	return (0);
}

/**
 * @brief Unclock thread blocked on a condition variable
 * @param cond Condition variable to signal
 * @return 0 upon successfull completion or a negative error code
 * upon failure
 */
int nanvix_cond_signal(struct nanvix_cond_var *cond)
{
#if (__NANVIX_CONDVAR_SLEEP)
	int head = -1;
#endif

	if (!cond)
	{
		kprintf("Invalid condition variable");
		return (-EINVAL);
	}

	spinlock_lock(&cond->lock2);

		spinlock_lock(&cond->lock);

			/**
			 * Remove the head of the queue.
			 */
			if (cond->size > 0)
			{

#if (__NANVIX_CONDVAR_SLEEP)
				head        = cond->tids[cond->begin];
#endif
				cond->begin = (cond->begin + 1) % THREAD_MAX;
				cond->size--;
			}

#if (!__NANVIX_CONDVAR_SLEEP)
			cond->locked = false;
#endif

		spinlock_unlock(&cond->lock);

#if (__NANVIX_CONDVAR_SLEEP)

		/**
		 * May be we need try to wakeup a thread more than one time because it
		 * is not an atomic sleep/wakeup.
		 *
		 * Obs.: We release the primary lock because we don't need garantee
		 * that the head thread gets the mutex.
		 */
		if (head != -1)
			while (LIKELY(kwakeup(head) != 0));

#endif  /* __NANVIX_CONDVAR_SLEEP */

	spinlock_unlock(&cond->lock2);

	return (0);
}

/**
 * @brief Unlocks all threads blocked on a condition variable.
 *
 * @param cond Condition variable to be signaled.
 *
 * @returns Upon successful completion, zero is returned. Upon failure,
 * a negative error code is returned instead.
 */
PUBLIC int nanvix_cond_broadcast(struct nanvix_cond_var *cond)
{
	if (!cond)
	{
		kprintf("Invalid condition variable");
		return (-EINVAL);
	}

#if (__NANVIX_CONDVAR_SLEEP)
	again:
#endif  /* __NANVIX_CONDVAR_SLEEP */

	spinlock_lock(&cond->lock);

		while (cond->tids[0] != -1)
		{
		#if (__NANVIX_CONDVAR_SLEEP)
			if (kwakeup(cond->tids[0]) != 0)
			{
				spinlock_unlock(&cond->lock);
				goto again;
			}
		#else
			cond->locked = false;
		#endif /* __NANVIX_CONDVAR_SLEEP */
		}

	spinlock_unlock(&cond->lock);

	return (0);
}

#endif  /* CORES_NUM */
