/*
 * MIT License
 *
 * Copyright(c) 2011-2019 The Maintainers of Nanvix
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

#ifndef NANVIX_SYS_SEMAPHORE_H_
#define NANVIX_SYS_SEMAPHORE_H_

	#include <nanvix/kernel/kernel.h>

#if (CORES_NUM > 1)

	#include <nanvix/sys/thread.h>

	/**
	 * @brief Semaphore.
	 */
	struct nanvix_semaphore
	{
		int val;                    /**< Semaphore value.  */
		spinlock_t lock;            /**< Lock.             */

		#if (__NANVIX_SEMAPHORE_SLEEP)

			kthread_t tids[THREAD_MAX]; /**< Sleeping threads. */

		#endif /* __NANVIX_SEMAPHORE_SLEEP */
	};

	/**
	 * @brief Initializes a semaphore.
	 *
	 * @param m Target semaphore.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_semaphore_init(struct nanvix_semaphore *sem, int val);

	/**
	 * @brief Performs a down operation on a semaphore.
	 *
	 * @param m Target semaphore.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_semaphore_down(struct nanvix_semaphore *sem);

	/**
	 * @brief Performs an up operation on a semaphore.
	 *
	 * @param m Target semaphore.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_semaphore_up(struct nanvix_semaphore *sem);

#endif

#endif /* NANVIX_SYS_SEMAPHORE_H_ */

/**@}*/
