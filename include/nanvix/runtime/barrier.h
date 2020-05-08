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

#ifndef NANVIX_RUNTIME_BARRIER_H_
#define NANVIX_RUNTIME_BARRIER_H_

	/**
	 * @brief Barrier
	 */
	typedef struct
	{
		int leader;   /**< Leader Node      */
		int syncs[2]; /**< Underlying Syncs */
	} barrier_t;

	/**
	 * @brief NULL Barrier
	 */
	#define BARRIER_NULL ((barrier_t) { .leader = -1, .syncs[0] = -1, .syncs[1] = -1 })

	/**
	 * @brief Asserts if a barrier is invalid.
	 *
	 * @param x Target barrier.
	 */
	#define BARRIER_IS_VALID(x) \
		(!((barrier.leader < 0) || (barrier.syncs[0] < 0 || (barrier.syncs[1] < 0))))

	/**
	 * @brief Creates a barrier.
	 *
	 * @returns Upons sucessful completion a newly created barrier is
	 * returned. Upon failure BARRIER_NULL is returned instead.
	 *
	 * @author Pedro Henrique Penna
	 */
	extern barrier_t barrier_create(const int *nodes, int nnodes);

	/**
	 * @brief Destroys a barrier.
	 *
	 * @param barrier Target barrier.
	 *
	 * @returns Upon sucessful completion, zero is returned. Upon failure a
	 * negative errorcode is returned instead.
	 *
	 * @author Pedro Henrique Penna
	 */
	extern int barrier_destroy(barrier_t barrier);

	/**
	 * @brief Waits on a barrier.
	 *
	 * @param barrier Target barrier.
	 *
	 * @returns Upon successful completion, zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 *
	 * @author Pedro Henrique Penna
	 */
	extern int barrier_wait(barrier_t barrier);

#endif /* NANVIX_RUNTIME_BARRIER_H_ */
