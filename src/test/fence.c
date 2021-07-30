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
 #include <nanvix/runtime/fence.h>
 #include "test.h"

#if (CORES_NUM > 1)

/*
 * @brief Number of synchronizations.
 */
#define NSYNCS 50

/*
 * @brief Fence variable used in tests.
 */
PRIVATE struct nanvix_fence fence;

/*
 * @brief Auxiliar spinlock.
 */
PRIVATE spinlock_t flock;

/*
 * @brief Auxiliar of synchronizations.
 */
PRIVATE int nsyncs;

/*============================================================================*
 * Threads                                                                    *
 *============================================================================*/

/*
 * @brief Fence auxiliar. Verify if the nsyncs is inside the range
 * [ time * nthreads, (time + 1) * nthreads ).
 */
PRIVATE bool verify_fence_syncs(int time, int nthreads)
{
	return (WITHIN(nsyncs, time * nthreads, (time + 1) * nthreads));
}

/*
 * @brief Task called in fence variable tests.
 */
PRIVATE void * fence_sanity(void * arg)
{
	UNUSED(arg);

	spinlock_lock(&flock);
		test_assert(verify_fence_syncs(0, fence.nthreads));
		nsyncs++;
	spinlock_unlock(&flock);

	test_assert(nanvix_fence(&fence) == 0);

	spinlock_lock(&flock);
		test_assert(verify_fence_syncs(1, fence.nthreads));
		nsyncs++;
	spinlock_unlock(&flock);

	test_assert(nanvix_fence(&fence) == 0);

	spinlock_lock(&flock);
		test_assert(verify_fence_syncs(2, fence.nthreads));
		nsyncs++;
	spinlock_unlock(&flock);

	test_assert(nanvix_fence(&fence) == 0);

	spinlock_lock(&flock);
		test_assert(nsyncs == (3 * fence.nthreads));
	spinlock_unlock(&flock);

	return (NULL);
}

/*
 * @brief Task called in fence variable tests.
 */
PRIVATE void * fence_do(void * arg)
{
	UNUSED(arg);

	/* Do N synchronizations. */
	for (int i = 0; i < nsyncs; ++i)
		test_assert(nanvix_fence(&fence) == 0);

	return (NULL);
}

/*============================================================================*
 * fence Unit Tests                                                         *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_api_fence_init()                                                    *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test for condition variable
 *
 * Tests condition variable initialization
 */
PRIVATE void test_api_fence_init(void)
{
	test_assert(nanvix_fence_init(&fence, 1) == 0);

		test_assert(nanvix_fence(&fence) == 0);

	test_assert(nanvix_fence_destroy(&fence) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_fence_sanity()                                                    *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test sanity for fence variable.
 *
 * Tests fence variable behavior using two threads
 */
PRIVATE void test_api_fence_sanity(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[2];

	nsyncs = 0;

	test_assert(nanvix_fence_init(&fence, 2) == 0);

		for (int i = 0; i < 2; i++)
			test_assert(kthread_create(&tids[i], fence_sanity, NULL) == 0);

		for (int i = 0; i < 2; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

	test_assert(nanvix_fence_destroy(&fence) == 0);
#endif
}

/*----------------------------------------------------------------------------*
 * test_api_fence_do()                                                        *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test broadcast for condition variable.
 *
 * Tests condition variable behavior using two threads
 */
PRIVATE void test_api_fence_do(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[2];

	nsyncs = NSYNCS;

	test_assert(nanvix_fence_init(&fence, 2) == 0);

		for (int i = 0; i < 2; i++)
			test_assert(kthread_create(&tids[i], fence_do, NULL) == 0);

		for (int i = 0; i < 2; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

	test_assert(nanvix_fence_destroy(&fence) == 0);
#endif
}

/*============================================================================*
 * fence Fault Tests                                                          *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_fault_fence_init()                                                    *
 *----------------------------------------------------------------------------*/

PRIVATE void test_fault_fence_init(void)
{
	test_assert(nanvix_fence_init(NULL, 1) < 0);
	test_assert(nanvix_fence_init(NULL, 0) < 0);
	test_assert(nanvix_fence_init(NULL, (THREAD_MAX + 1)) < 0);

	test_assert(nanvix_fence_init(&fence, -1) < 0);
	test_assert(nanvix_fence_init(&fence, 0) < 0);
	test_assert(nanvix_fence_init(&fence, (THREAD_MAX + 1)) < 0);

	test_assert(nanvix_fence_destroy(NULL) < 0);
}

/*----------------------------------------------------------------------------*
 * test_fault_fence_operations()                                              *
 *----------------------------------------------------------------------------*/

PRIVATE void test_fault_fence_operations(void)
{
	test_assert(nanvix_fence(NULL) < 0);
}

/*============================================================================*
 * fence Stress Tests                                                         *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_stress_fence_sanity()                                                 *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test sanity for fence variable.
 *
 * Tests fence variable behavior using two threads
 */
PRIVATE void test_stress_fence_sanity(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[NTHREADS];

	nsyncs = 0;

	test_assert(nanvix_fence_init(&fence, NTHREADS) == 0);

		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_create(&tids[i], fence_sanity, NULL) == 0);

		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

	test_assert(nanvix_fence_destroy(&fence) == 0);
#endif
}

/*----------------------------------------------------------------------------*
 * test_stress_fence_do()                                                     *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test broadcast for condition variable.
 *
 * Tests condition variable behavior using two threads
 */
PRIVATE void test_stress_fence_do(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[NTHREADS];

	nsyncs = NSYNCS;

	test_assert(nanvix_fence_init(&fence, NTHREADS) == 0);

		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_create(&tids[i], fence_do, NULL) == 0);

		for (int i = 0; i < NTHREADS; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

	test_assert(nanvix_fence_destroy(&fence) == 0);
#endif
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
PRIVATE struct test fences_tests_api[] = {
	{ test_api_fence_init,   "[test][fence][api] Fence init                       [passed]" },
	{ test_api_fence_sanity, "[test][fence][api] Sanity fence between two threads [passed]" },
	{ test_api_fence_do,     "[test][fence][api] N syncs between two threads      [passed]" },
	{ NULL,                  NULL                                                           },
};

/**
 * @brief Fault tests.
 */
PRIVATE struct test fences_tests_fault[] = {
	{ test_fault_fence_init,       "[test][fence][fault] Fence init [passed]" },
	{ test_fault_fence_operations, "[test][fence][fault] Fence      [passed]" },
	{ NULL,                        NULL                                       },
};

/**
 * @brief Stress tests.
 */
PRIVATE struct test fences_tests_stress[] = {
	{ test_stress_fence_sanity, "[test][fence][stress] Sanity fence between two threads [passed]" },
	{ test_stress_fence_do,     "[test][fence][stress] N syncs between two threads      [passed]" },
	{ NULL,                     NULL                                                              },
};

#endif  /* CORES_NUM */

/**
 * @brief Condition variable tests laucher
 */
PUBLIC void test_fence(void)
{
#if (CORES_NUM > 1)

	spinlock_init(&flock);

	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; fences_tests_api[i].test_fn != NULL; i++)
	{
		fences_tests_api[i].test_fn();
		nanvix_puts(fences_tests_api[i].name);
	}

	/* Fault Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; fences_tests_fault[i].test_fn != NULL; i++)
	{
		fences_tests_fault[i].test_fn();
		nanvix_puts(fences_tests_fault[i].name);
	}

	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; fences_tests_stress[i].test_fn != NULL; i++)
	{
		fences_tests_stress[i].test_fn();
		nanvix_puts(fences_tests_stress[i].name);
	}

#endif  /* CORES_NUM */
}

