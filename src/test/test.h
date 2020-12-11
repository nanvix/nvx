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

#ifndef _TEST_H_
#define _TEST_H_

	#include <nanvix/sys/thread.h>

	/**
     * @brief Test's parameters
     */
	/**@{*/
    #define NR_NODES       2
    #define MASTER_NODENUM 0
#ifdef __mppa256__
	#define SLAVE_NODENUM  8
	#define TEST_THREAD_NPORTS (K1BDP_CORES_NUM - 1)
#else
	#define SLAVE_NODENUM  1
	#define TEST_THREAD_NPORTS (THREAD_MAX)
#endif
    #define NSETUPS 10
    #define NCOMMUNICATIONS 100
	/**@}*/

    /**
     * @name Portal sizes.
     */
	/**@{*/
    #define PORTAL_SIZE       (1 * KB)                             /**< Small size. */
	#define PORTAL_SIZE_LARGE (HAL_PORTAL_DATA_SIZE + PORTAL_SIZE) /**< Large size. */
	/**@}*/

	/**
     * @brief Portal message size
     */
    #define MAILBOX_SIZE (KMAILBOX_MESSAGE_SIZE)

    /**
     * @brief Max number of nodes involved.
     */
    #define MAX_NUM_NODES PROCESSOR_NOC_NODES_NUM

	/**
	 * @brief Number of iterations in stress tests.
	 */
	#define NITERATIONS 10

	/**
	 * @brief Number of threads to spawn in stress tests.
	 */
	#define NTHREADS (THREAD_MAX - 1)

	#define ___STRINGIFY(x) #x
	#define ___TOSTRING(x) ___STRINGIFY(x)

	/**
	 * @brief Unit test.
	 */
	struct test
	{
		void (*test_fn)(void); /**< Test function. */
		const char *name;      /**< Test name.     */
	};

	/**
	 * @brief Writes a string to the standard output device.
	 *
	 * @param str Target string.
	 */
	extern void nanvix_puts(const char *str);

#if __TARGET_HAS_SYNC
    /**
     * @name Barrier functions
     */
    /**@{*/
	extern void test_delay(int times, uint64_t cycles);
    extern void test_barrier_nodes_setup(const int * nodes, int nnodes, int is_master);
    extern void test_barrier_nodes(void);
    extern void test_barrier_nodes_cleanup(void);
    /**@}*/
#endif /* __TARGET_HAS_SYNC */

	/**
	 * @name User-Level Testing Units
	 */
	/**@{*/
	extern void test_kframe_mgmt(void);
	extern void test_page_mgmt(void);
	extern void test_thread_mgmt(void);
	extern void test_excp_mgmt(void);
	extern void test_thread_sleep(void);
	extern void test_condition_variables(void);
	extern void test_perf(void);
	extern void test_signal(void);
	extern void test_network(void);
	extern void test_noc(void);
	extern void test_sync(void);
	extern void test_mailbox(void);
	extern void test_portal(void);
	extern void test_ikc(void);

	/**@}*/

	/**
	 * @brief Asserts a condition.
	 *
	 * @param x Condition to assert.
	 */
	#define test_assert(x)                            \
	{                                                 \
		if (!(x))                                     \
		{                                             \
			nanvix_puts("assertation failed at "      \
				__FILE__":" ___TOSTRING(__LINE__)"\n" \
			);                                        \
			kshutdown() /* noop */;                   \
		}                                             \
	}

	/**
     * @brief Horizontal line.
     */
    extern const char *HLINE;

#endif /* _TEST_H_  */
