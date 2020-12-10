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

/**
 * @addtogroup nanvix Nanvix System
 */
/**@{*/

#ifndef NANVIX_SYS_MUTEX_H_
#define NANVIX_SYS_MUTEX_H_

	#include <nanvix/kernel/kernel.h>

#if (CORES_NUM > 1)

	#include <nanvix/sys/thread.h>
	#include <posix/stdbool.h>

	/**
	 * @brief Mutex.
	 */
	struct nanvix_mutex
	{
		bool locked;     /**< Locked? */
		spinlock_t lock; /**< Lock.   */

		#if (__NANVIX_MUTEX_SLEEP)

			spinlock_t lock2;           /**< Lock helper.      */
			kthread_t tids[THREAD_MAX]; /**< Sleeping threads. */

		#endif /* __NANVIX_MUTEX_SLEEP */
	};

	/**
	 * @brief Initializes a mutex.
	 *
	 * @param m Target mutex.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutex_init(struct nanvix_mutex *m);

	/**
	 * @brief Locks a mutex.
	 *
	 * @param m Target mutex.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutex_lock(struct nanvix_mutex *m);

	/**
	 * @brief unlocks a mutex.
	 *
	 * @param m Target mutex.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutex_unlock(struct nanvix_mutex *m);

#endif /* CORES_NUM > 1 */

#endif /* NANVIX_SYS_MUTEX_H_ */

/**@}*/
