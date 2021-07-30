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

#include <nanvix/sys/thread.h>
#include "test.h"

/**
 * @name Auxiliar Constants
 */
/**@{*/
#define TEST_EXPECTED_VALUE (0xc0ffee) /**< Default value. */
/**@}*/

/**
 * @name Extra Tests
 */
/**@{*/
#define UTEST_KTHREAD_BAD_START 0 /**< Test bad thread start?    */
#define UTEST_KTHREAD_BAD_ARG   0 /**< Test bad thread argument? */
#define UTEST_KTHREAD_BAD_JOIN  0 /**< Test bad thread join?     */
#define UTEST_KTHREAD_BAD_EXIT  0 /**< Test bad thread exit?     */
/**@}*/

/**
 * @brief Dummy task.
 *
 * @param arg Unused argument.
 */
static void *task(void *arg)
{
	UNUSED(arg);

	return (NULL);
}

/**
 * @brief Global lock to thread tests.
 */
static spinlock_t lock_tt = SPINLOCK_UNLOCKED;

/**
 * @brief Release fence task.
 */
static bool release_tt = false;

/**
 * @brief Fence task.
 *
 * @param arg Unused argument.
 */
static void *fence_task(void *arg)
{
	int release;

	UNUSED(arg);
	release = false;

	while (!release)
	{
		spinlock_lock(&lock_tt);

			release = release_tt;

		spinlock_unlock(&lock_tt);
	}

	return (NULL);
}

/**
 * @brief Yield task.
 *
 * @param arg Unused argument.
 */
static void *yield_task(void *arg)
{
	uint64_t a;

	UNUSED(arg);

	a = TEST_EXPECTED_VALUE;

	kthread_yield();

	KASSERT(a == TEST_EXPECTED_VALUE);

	return (NULL);
}

/**
 * @brief Global variable used in the return task.
 */
static int global_val = 0;

/**
 * @brief Return pointer task.
 *
 * @param arg Unused argument.
 */
static void * return_pointer_task(void *arg)
{
	UNUSED(arg);

	/* Store a new value to shared variable. */
	global_val = TEST_EXPECTED_VALUE;

	/* Return the pointer to be access on master thread. */
	return (&global_val);
}

static void * return_value_task(void *arg)
{
	UNUSED(arg);

	return ((void *) TEST_EXPECTED_VALUE);
}

/**
 * @brief One counter for each thread.
 */
PRIVATE volatile int sched_counter = 0;

/**
 * @brief Sched task.
 *
 * @param arg Unused argument.
 */
static void *sched_task(void *arg)
{
	int busy;
	bool exit;

	UNUSED(arg);

	spinlock_lock(&lock_tt);

		busy = 1000;
		while (busy--);

		++sched_counter;

		busy = 1000;
		while (busy--);

	spinlock_unlock(&lock_tt);

	exit = false;

	while (!exit)
	{
		/* Shared region. */
		spinlock_lock(&lock_tt);

			busy = 1000;
			while (busy--);

			exit = (sched_counter == NTHREADS);

			busy = 1000;
			while (busy--);

		spinlock_unlock(&lock_tt);
	}

	return (NULL);
 }

/**
 * @brief Yield task.
 *
 * @param arg Unused argument.
 */
static void *affinity_task(void *arg)
{
	int curr_coreid;
	int next_coreid;

	UNUSED(arg);

	curr_coreid = core_get_id();
	next_coreid = curr_coreid == 1 ? 2 : 1;

	KASSERT(kthread_set_affinity(1 << curr_coreid) == KTHREAD_AFFINITY_DEFAULT);

	KASSERT(core_get_id() == curr_coreid);

	KASSERT(kthread_set_affinity(1 << next_coreid) == (1 << curr_coreid));

	KASSERT(core_get_id() == next_coreid);

	KASSERT(kthread_set_affinity(KTHREAD_AFFINITY_DEFAULT) == (1 << next_coreid));

	KASSERT(core_get_id() == next_coreid);

	return (NULL);
}

/*============================================================================*
 * API Testing Units                                                          *
 *============================================================================*/

/**
 * @brief API test for thread identification.
 */
static void test_api_kthread_self(void)
{
	/**
	 * The range [0, (KTHREAD_MAX - 1)] are reserved to kernel threads.
	 * So, the first valid ID is the KTHREAD_MAX.
	 */
	test_assert(kthread_self() == KTHREAD_LEADER_TID);
}

/**
 * @brief API test for thread creation/termination.
 */
static void test_api_kthread_create(void)
{
#if (THREAD_MAX > 1)

	kthread_t tid;

	/* Spawn thread. */
	test_assert(kthread_create(&tid, task, NULL) == 0);

	/* Wait for thread. */
	test_assert(kthread_join(tid, NULL) == 0);

#endif
}

/**
 * @brief API test for thread yield.
 */
static void test_api_kthread_yield(void)
{
#if (THREAD_MAX > 1)

	kthread_t tid;

	/* Spawn thread. */
	test_assert(kthread_create(&tid, yield_task, NULL) == 0);

	/* Wait for thread. */
	test_assert(kthread_join(tid, NULL) == 0);

#endif
}

/**
 * @brief API test for thread affinity.
 */
static void test_api_kthread_affinity(void)
{
#if ((THREAD_MAX > 1) && CORE_SUPPORTS_MULTITHREADING && !__NANVIX_MICROKERNEL_STATIC_SCHED)

	kthread_t tid;

	/* Spawn thread. */
	test_assert(kthread_create(&tid, affinity_task, NULL) == 0);

	/* Wait for thread. */
	test_assert(kthread_join(tid, NULL) == 0);

#endif
}

/**
 * @brief API Test: Return NULL from the slave thread.
 */
static void test_api_kthread_return_null(void)
{
#if (THREAD_MAX > 1)

	int * retval;
	kthread_t tid;

	/* Spawn thread. */
	test_assert(kthread_create(&tid, task, NULL) == 0);

	/* Set retval to 1 to assert that the join will set it to NULL. */
	retval = (int *) 0x1;

	/* Wait for thread. */
	test_assert(kthread_join(tid, (void **) &retval) == 0);

	/* In this test, we return a expected int value. */
	test_assert(retval == NULL);

#endif
}

/**
 * @brief API Test: Return a int from the slave thread.
 */
static void test_api_kthread_return_value(void)
{
#if (THREAD_MAX > 1)

	int * retval;
	kthread_t tid;

	/* Spawn thread. */
	test_assert(kthread_create(&tid, return_value_task, NULL) == 0);

	retval = NULL;

	/* Wait for thread. */
	test_assert(kthread_join(tid, (void **) &retval) == 0);

	/* In this test, we return a expected int value. */
	test_assert(retval == (void *) TEST_EXPECTED_VALUE);

#endif
}

/**
 * @brief API Test: Return a pointer seted by the slave thread.
 */
static void test_api_kthread_return_pointer(void)
{
#if (THREAD_MAX > 1)

	int * retval;
	kthread_t tid;

	/* Spawn thread. */
	test_assert(kthread_create(&tid, return_pointer_task, NULL) == 0);

	retval = NULL;

	/* Wait for thread. */
	test_assert(kthread_join(tid, (void **) &retval) == 0);

	/* Returned pointer must be the same of the global variable. */
	test_assert(retval == &global_val);

	/* Both access must contain the same value. */
	test_assert(*retval == global_val);

	/* The value must be the expected value. */
	test_assert(*retval == TEST_EXPECTED_VALUE);

#endif
}

/**
 * @brief API test for thread stats.
 */
static void test_api_kthread_stats(void)
{
#if (CORE_SUPPORTS_MULTITHREADING)

	kthread_t tid;
	uint64_t t0 = 0ULL;
	uint64_t t1 = 0ULL;

	tid = kthread_self();

#if __NANVIX_MICROKERNEL_THREAD_STATS

	/* Gets old time. */
	test_assert(kthread_stats(tid, &t0, KTHREAD_STATS_EXEC_TIME) == 0);

	/* Reset time. */
	test_assert(kthread_stats(tid, NULL, KTHREAD_STATS_EXEC_TIME) == 0);

	/* Gets new time. */
	test_assert(kthread_stats(tid, &t1, KTHREAD_STATS_EXEC_TIME) == 0);

	test_assert(t0 != 0ULL);
	test_assert(t1 != 0ULL);
	test_assert(t1 < t0);

#else

	test_assert(kthread_stats(tid, &t0, KTHREAD_STATS_EXEC_TIME)  < 0);
	test_assert(kthread_stats(tid, NULL, KTHREAD_STATS_EXEC_TIME) < 0);
	UNUSED(t1);

#endif

#endif
}

/*============================================================================*
 * Fault Testing Units                                                        *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Thread Create
 */
static void test_fault_kthread_create_invalid(void)
{
#if (THREAD_MAX > 1)

	kthread_t tid;

	/* Invalid start routine. */
	test_assert(kthread_create(&tid, NULL, NULL) < 0);

#endif
}

/**
 * @brief Fault Test: Bad Thread Create
 */
static void test_fault_kthread_create_bad(void)
{
#if (THREAD_MAX > 1)

	kthread_t tid;

	UNUSED(tid);

	/* Bad starting routine. */
#if (defined(UTEST_KTHREAD_BAD_START) && (UTEST_KTHREAD_BAD_START == 1))
	test_assert(kthread_create(&tid, (void *(*)(void *)) KBASE_VIRT, NULL) < 0);
	test_assert(kthread_create(&tid, (void *(*)(void *)) (UBASE_VIRT - PAGE_SIZE), NULL) < 0);
#endif

	/* Bad argument. */
#if (defined(UTEST_KTHREAD_BAD_ARG) && (UTEST_KTHREAD_BAD_ARG == 1))
	test_assert(kthread_create(&tid[0], task, (void *(*)(void *)) KBASE_VIRT) < 0);
	test_assert(kthread_create(&tid[0], task, (void *(*)(void *)) (UBASE_VIRT - PAGE_SIZE)) < 0);
#endif

#endif
}

/**
 * @brief Fault Test: Invalid Thread Join
 */
static void test_fault_kthread_join_invalid(void)
{
#if (THREAD_MAX > 1)

	test_assert(kthread_join(-1, NULL) < 0);
	test_assert(kthread_join(0, NULL) < 0);
	test_assert(kthread_join(1, NULL) < 0);

#endif
}

/**
 * @brief Fault Test: Bad Thread Join
 */
static void test_fault_kthread_join_bad(void)
{
#if (THREAD_MAX > 1)

	kthread_t tid;

	/* Join bad thread. */
	test_assert(kthread_create(&tid, task, NULL) == 0);
	test_assert(kthread_join(tid + 1, NULL) < 0);
	test_assert(kthread_join(tid, NULL) == 0);

	/* Join with bad return value. */
#if (defined(UTEST_KTHREAD_BAD_JOIN) && (UTEST_KTHREAD_BAD_JOIN == 1))
	test_assert(kthread_create(&tid, task, NULL) == 0);
	test_assert(kthread_join(tid, (void *)(KBASE_VIRT)) < 0);
	test_assert(kthread_join(tid, (void *)(UBASE_VIRT - PAGE_SIZE)) < 0);
	test_assert(kthread_join(tid, NULL) == 0);
#endif

#endif
}

/**
 * @brief Fault Test: Bad Affinity
 */
static void test_fault_kthread_affinity(void)
{
#if (CORE_SUPPORTS_MULTITHREADING && !__NANVIX_MICROKERNEL_STATIC_SCHED)

	test_assert(kthread_set_affinity(0) < 0);
	test_assert(kthread_set_affinity(1 << CORES_NUM) < 0);

#endif
}

/**
 * @brief Fault test for thread stats.
 */
static void test_fault_kthread_stats(void)
{
#if (CORE_SUPPORTS_MULTITHREADING)

	uint64_t t0 = 0ULL;

	/* Invalid stat. */
	test_assert(kthread_stats(kthread_self(), &t0, -1) < 0);

	/* Invalid thread. */
	test_assert(kthread_stats(-1, &t0, KTHREAD_STATS_EXEC_TIME) < 0);

	/* All Invalid. */
	test_assert(kthread_stats(-1, NULL, -1) < 0);

#endif
}
/*============================================================================*
 * Stress Testing Units                                                       *
 *============================================================================*/

/**
 * @brief Fault Test: Create Too Many Threads
 */
static void test_stress_kthread_create_overflow(void)
{
#if (THREAD_MAX > 1)

	kthread_t tid[NTHREADS + 1];

	for (int i = 0; i < NTHREADS; i++)
		test_assert(kthread_create(&tid[i], fence_task, NULL) == 0);

	test_assert(kthread_create(&tid[NTHREADS], fence_task, NULL) < 0);

	spinlock_lock(&lock_tt);
		release_tt = true;
	spinlock_unlock(&lock_tt);

	for (int i = 0; i < NTHREADS; i++)
		test_assert(kthread_join(tid[i], NULL) == 0);

	spinlock_lock(&lock_tt);
		release_tt = false;
	spinlock_unlock(&lock_tt);

#endif
}

/**
 * @brief Stress test for thread creation/termination.
 */
static void test_stress_kthread_create(void)
{
#if (THREAD_MAX > 2)

	for (int j = 0; j < NITERATIONS; j++)
	{
		kthread_t tid[NTHREADS];

		/* Spawn threads. */
		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_create(&tid[i], task, NULL) == 0);

		/* Wait for threads. */
		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_join(tid[i], NULL) == 0);
	}

#endif
}

/**
 * @brief Stress test for thread creation/termination yield.
 */
static void test_stress_kthread_yield(void)
{
#if (THREAD_MAX > 2)

	for (int j = 0; j < NITERATIONS; j++)
	{
		kthread_t tid[NTHREADS];

		/* Spawn threads. */
		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_create(&tid[i], yield_task, NULL) == 0);

		/* Wait for threads. */
		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_join(tid[i], NULL) == 0);
	}

#endif
}

/**
 * @brief Stress test for thread scheduler.
 */
static void test_stress_kthread_scheduler(void)
{
#if (THREAD_MAX > 2 && CORE_SUPPORTS_MULTITHREADING)

	for (int j = 0; j < NITERATIONS; j++)
	{
		kthread_t tid[NTHREADS];

		sched_counter = 0;

		/* Spawn threads. */
		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_create(&tid[i], sched_task, NULL) == 0);

		/* Wait for threads. */
		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_join(tid[i], NULL) == 0);
	}

#endif
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
static struct test thread_mgmt_tests_api[] = {
	{ test_api_kthread_self,           "[test][thread][api] thread identification       [passed]" },
	{ test_api_kthread_create,         "[test][thread][api] thread creation/termination [passed]" },
	{ test_api_kthread_yield,          "[test][thread][api] thread yield                [passed]" },
	{ test_api_kthread_affinity,       "[test][thread][api] thread affinity             [passed]" },
	{ test_api_kthread_stats,          "[test][thread][api] thread stats                [passed]" },
	{ test_api_kthread_return_null,    "[test][thread][api] thread return null          [passed]" },
	{ test_api_kthread_return_value,   "[test][thread][api] thread return value         [passed]" },
	{ test_api_kthread_return_pointer, "[test][thread][api] thread return pointer       [passed]" },
	{NULL,                              NULL                                                      },
};

/**
 * @brief Fault tests.
 */
static struct test thread_mgmt_tests_fault[] = {
	{ test_fault_kthread_create_invalid,  "[test][thread][fault] invalid thread create [passed]" },
	{ test_fault_kthread_create_bad,      "[test][thread][fault] bad thread create     [passed]" },
	{ test_fault_kthread_join_invalid,    "[test][thread][fault] invalid thread join   [passed]" },
	{ test_fault_kthread_join_bad,        "[test][thread][fault] bad thread join       [passed]" },
	{ test_fault_kthread_affinity,        "[test][thread][fault] bad affinity          [passed]" },
	{ test_fault_kthread_stats,           "[test][thread][fault] bad stats             [passed]" },
	{ NULL,                                NULL                                                  },
};

/**
 * @brief Stress tests.
 */
static struct test thread_mgmt_tests_stress[] = {
	{ test_stress_kthread_create_overflow, "[test][thread][stress] thread creation overflow              [passed]" },
	{ test_stress_kthread_create,          "[test][thread][stress] thread creation/termination           [passed]" },
	{ test_stress_kthread_yield,           "[test][thread][stress] thread creation/termination yield     [passed]" },
	{ test_stress_kthread_scheduler,       "[test][thread][stress] thread creation/termination scheduler [passed]" },
	{ NULL,                                 NULL                                                                   },
};

/**
 * The test_thread_mgmt() function launches testing units on thread manager.
 *
 * @author Pedro Henrique Penna
 */
void test_thread_mgmt(void)
{
	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; thread_mgmt_tests_api[i].test_fn != NULL; i++)
	{
		thread_mgmt_tests_api[i].test_fn();
		nanvix_puts(thread_mgmt_tests_api[i].name);
	}

	/* Fault Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; thread_mgmt_tests_fault[i].test_fn != NULL; i++)
	{
		thread_mgmt_tests_fault[i].test_fn();
		nanvix_puts(thread_mgmt_tests_fault[i].name);
	}

	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; thread_mgmt_tests_stress[i].test_fn != NULL; i++)
	{
		thread_mgmt_tests_stress[i].test_fn();
		nanvix_puts(thread_mgmt_tests_stress[i].name);
	}
}

