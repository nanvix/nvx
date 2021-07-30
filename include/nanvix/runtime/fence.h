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

#ifndef NANVIX_RUNTIME_FENCE_H_
#define NANVIX_RUNTIME_FENCE_H_

	#include <nanvix/sys/condvar.h>
	#include <nanvix/sys/mutex.h>

#if (CORES_NUM > 1)

	/**
	 * @brief Fence.
	 */
	struct nanvix_fence
	{
		int release;                 /**< Wait condition.                           */
		int nthreads;                /**< Number of threads in the fence.           */
		int nreached;                /**< Number of threads that reached the fence. */
		struct nanvix_mutex mutex;   /**< Mutex.                                    */
		struct nanvix_cond_var cond; /**< Condition.                                */
	};

	/**
	 * @brief Initializes a fence.
	 *
	 * @param b        Target fence.
	 * @param nthreads Number of threads in the fence.
	 *
	 * @returns Upons sucessful completion zero is returned. Upon failure,
	 * a negative number is returned instead.
	 */
	extern int nanvix_fence_init(struct nanvix_fence * b, int nthreads);

	/**
	 * @brief Destroy a fence.
	 *
	 * @param b Target fence.
	 *
	 * @returns Upons sucessful completion zero is returned. Upon failure,
	 * a negative number is returned instead.
	 */
	extern int nanvix_fence_destroy(struct nanvix_fence * b);

	/**
	 * @brief Waits in a fence until the defined number of threads reach it.
	 *
	 * @param b Target fence.
	 *
	 * @returns Upon sucessful completion, zero is returned. Upon failure a
	 * negative errorcode is returned instead.
	 */
	extern int nanvix_fence(struct nanvix_fence * b);

#endif /* CORES_NUM > 1 */

#endif /* NANVIX_RUNTIME_FENCE_H_ */

