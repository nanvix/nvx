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

#ifndef NANVIX_SYS_FMUTEX_H_
#define NANVIX_SYS_FMUTEX_H_

	#include <nanvix/kernel/kernel.h>

#if (CORES_NUM > 1)

	#include <nanvix/sys/thread.h>

	/**
	 * @brief Fast mutex.
	 */
	struct nanvix_fmutex
	{
		spinlock_t lock;
	};

	/**
	 * @brief Initializes a fast mutex.
	 *
	 * @param m Target fmutex.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	static inline int nanvix_fmutex_init(struct nanvix_fmutex *m)
	{
		/* Invalid mutex. */
		if (m == NULL)
			return (-EINVAL);

		spinlock_init(&m->lock);

		return (0);
	}

	/**
	 * @brief Locks a fast mutex.
	 *
	 * @param m Target fmutex.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	static inline int nanvix_fmutex_lock(struct nanvix_fmutex *m)
	{
		/* Invalid mutex. */
		if (UNLIKELY(m == NULL))
			return (-EINVAL);

		spinlock_lock(&m->lock);

		return (0);
	}

	/**
	 * @brief Unlocks a fast mutex.
	 *
	 * @param m Target fmutex.
	 *
	 * @param Upon sucessful completion, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	static inline int nanvix_fmutex_unlock(struct nanvix_fmutex *m)
	{
		/* Invalid mutex. */
		if (UNLIKELY(m == NULL))
			return (-EINVAL);

		spinlock_unlock(&m->lock);

		return (0);
	}

#endif /* CORES_NUM > 1 */

#endif /* NANVIX_SYS_FMUTEX_H_ */

/**@}*/
