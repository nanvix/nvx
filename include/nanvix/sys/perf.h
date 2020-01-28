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

#ifndef NANVIX_SYS_PERF_H_
#define NANVIX_SYS_PERF_H_

	#include <nanvix/kernel/kernel.h>
	#include <posix/stdint.h>

	/**
	 * @brief Queries a performance event.
	 *
	 * @param event Target event.
	 *
	 * @returns Upon successful completion, either zero or non zero is
	 * returned, indicating whether or not the queried performance event
	 * may be monitored in the underlying core. Upon failure, a negative
	 * error code is returned instead.
	 */
	extern int nanvix_perf_query(int event);

	/**
	 * @brief Starts a performance monitor.
	 *
	 * @param perf  Target performance monitor.
	 * @param event Target event to watch.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int nanvix_perf_start(int perf, int event);

	/**
	 * @brief Stops a performance monitor.
	 *
	 * @param perf Target performance monitor.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int nanvix_perf_stop(int perf);

	/**
	 * @brief Restarts a performance monitor.
	 *
	 * @param perf Target performance monitor.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int nanvix_perf_restart(int perf);

	/**
	 * @brief Reads a PM register.
	 *
	 * @param perf Target performance monitor.
	 *
	 * @returns Upon successful completion, the value of the target
	 * performance monitor. Upon failure, a negative error code,
	 * converted to uint64_t is returned instead.
	 */
	extern uint64_t nanvix_perf_read(int perf);

	/**
	 * @brief Gets the kernel wall clock.
	 *
	 * @param buffer Store location for wall clock.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int kclock(uint64_t *buffer);

	/**
	 * @brief Gets performance statistics of the kernel.
	 *
	 * @param buffer Store location for the statistics.
	 * @param perf   Performance counter to watch.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int kstats(uint64_t *buffer, int perf);

#endif /* NANVIX_SYS_PERF_H_ */

/**@}*/
