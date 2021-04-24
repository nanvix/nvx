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

#ifndef NANVIX_SYS_CONDVAR_H_
#define NANVIX_SYS_CONDVAR_H_

#include <nanvix/kernel/kernel.h>

#if (CORES_NUM > 1)

	#include <nanvix/sys/mutex.h>
	#include <nanvix/sys/thread.h>

	/**
	 * @brief Condition Variable
	 */
	struct nanvix_cond_var
	{
		spinlock_t lock;
		spinlock_t lock2;

		int begin;                  /**< Fist valid element.    */
		int end;                    /**< Last valid element.    */
		int size;                   /**< Current queue size.    */
		kthread_t tids[THREAD_MAX]; /**< Buffer.                */

		#if (!__NANVIX_CONDVAR_SLEEP)
			bool locked;
		#endif
	};

	/**
	 * @brief Initializes a condition variable.
	 *
	 * @param cond Condition variable to be initialized.
	 *
	 * @returns 0 if the condition variable was sucessfully initialized
	 * or a negative code error if an error occurred.
	 */
	extern int nanvix_cond_init(struct nanvix_cond_var *cond);

	/**
	 * @brief Block thread on a condition variable.
	 *
	 * @param cond  Condition variable to wait for.
	 * @param mutex Target mutex unlocked when waiting for a signal and locked when
	 * after a cond_signal call.
	 *
	 * @returns 0 upon successfull completion or a negative error code
	 * upon failure.
	 */
	extern int nanvix_cond_wait(struct nanvix_cond_var *cond, struct nanvix_mutex *mutex);

	/**
	 * @brief Unlock the first thread blocked on a condition variable.
	 *
	 * @param cond Condition variable to be signaled.
	 *
	 * @returns 0 upon successfull completion or a negative error code
	 * upon failure.
	 */
	extern int nanvix_cond_signal(struct nanvix_cond_var *cond);

	/**
	 * @brief Unlocks all threads blocked on a condition variable.
	 *
	 * @param cond Condition variable to be signaled.
	 *
	 * @returns Upon successful completion, zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 */
	extern int nanvix_cond_broadcast(struct nanvix_cond_var *cond);

	/**
	 * @brief Destroy a condition variable.
	 *
	 * @param cond Condition variable to destroy.
	 *
	 * @returns 0 upon successfull completion or a negative error code
	 * upon failure.
	 */
	extern int nanvix_cond_destroy(struct nanvix_cond_var *cond);

#endif  /* CORES_NUM */

#endif  /* NANVIX_SYS_CONDVAR_H_ */

/**@}*/
