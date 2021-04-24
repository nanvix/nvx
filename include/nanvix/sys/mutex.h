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

	#define NANVIX_MUTEX_NORMAL     0
	#define NANVIX_MUTEX_ERRORCHECK 1
	#define NANVIX_MUTEX_RECURSIVE  2
	#define NANVIX_MUTEX_DEFAULT    3
	#define NANVIX_MUTEX_LIMIT      4

	/**
	 * @brief Mutex Attribute
	 */
	struct nanvix_mutexattr
	{
		int type;
	};

	/**
	 * @brief Mutex.
	 */
	struct nanvix_mutex
	{
		bool locked;     /**< Locked?         */
		spinlock_t lock; /**< Lock.           */
		int type;        /**< Type            */
		int rlevel;      /**< Recursion level */
		kthread_t owner; /**< Owner thread    */

		#if (__NANVIX_MUTEX_SLEEP)

			int begin;                  /**< Fist valid element.    */
			int end;                    /**< Last valid element.    */
			int size;                   /**< Current queue size.    */
			kthread_t tids[THREAD_MAX]; /**< Buffer.                */

			spinlock_t lock2;           /**< Exclusive unlock call. */

		#endif /* __NANVIX_MUTEX_SLEEP */
	};

	/**
	 * @brief Initializes a mutex.
	 *
	 * @param m Target mutex.
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutex_init(struct nanvix_mutex *m, struct nanvix_mutexattr *mattr);

	/**
	 * @brief Locks a mutex.
	 *
	 * @param m Target mutex.
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutex_lock(struct nanvix_mutex *m);

	/**
	 * @brief Tries to lock a mutex.
	 *
	 * @param m Target mutex.
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutex_trylock(struct nanvix_mutex *m);

	/**
	 * @brief Unlocks a mutex.
	 *
	 * @param m Target mutex.
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutex_unlock(struct nanvix_mutex *m);

	/**
	 * @brief Destroy a mutex.
	 *
	 * @param m Target mutex.
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutex_destroy(struct nanvix_mutex *m);

	/**
	 * @brief Initializes a mutex attribute.
	 *
	 * @param mattr Target mutex attribute.
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutexattr_init(struct nanvix_mutexattr *mattr);

	/**
	 * @brief Destroy a mutex attribute.
	 *
	 * @param mattr Target mutex attribute.
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutexattr_destroy(struct nanvix_mutexattr *mattr);

	/**
	 * @brief Set mutex attribute type.
	 *
	 * @param mattr Target mutex attribute.
	 * @param type Mutex attribute type
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutexattr_settype(struct nanvix_mutexattr* mattr, int type);

	/**
	 * @brief Get mutex attribute type.
	 *
	 * @param mattr Target mutex attribute.
	 *
	 * @return Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int nanvix_mutexattr_gettype(struct nanvix_mutexattr* mattr);

#endif /* CORES_NUM > 1 */

#endif /* NANVIX_SYS_MUTEX_H_ */

/**@}*/
