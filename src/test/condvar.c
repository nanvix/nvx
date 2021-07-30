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
 #include <nanvix/sys/condvar.h>
 #include <nanvix/sys/mutex.h>
 #include "test.h"

#if (CORES_NUM > 1)

/*
 * @brief Boolean variable used in condition variable tests
 */
PRIVATE bool thread_condition;

/*
 * @name Broadcast variables
 */
/**{*/
PRIVATE int thread_amount;
PRIVATE int thread_counter;
/**}*/

/*
 * @brief Condition variable used in tests
 */
PRIVATE struct nanvix_cond_var cond_var;

/*
 * @brief Mutex used in condition variable tests
 */
PRIVATE struct nanvix_mutex mutex;

/*============================================================================*
 * Threads                                                                    *
 *============================================================================*/

/*
 * @brief Task called in condition variable tests
 */
PRIVATE void * condvar_signal(void * arg)
{
	bool must_wait;

	UNUSED(arg);

	nanvix_mutex_lock(&mutex);

		/* Gets rule. */
		must_wait = thread_condition;

		/* Change rule for the next thread. */
		thread_condition = !thread_condition;

		if (must_wait)
			nanvix_cond_wait(&cond_var, &mutex);
		/* must signal. */
		else
			nanvix_cond_signal(&cond_var);

	nanvix_mutex_unlock(&mutex);

	return (NULL);
}

/*
 * @brief Test broadcast.
 */
PRIVATE void * condvar_broadcast(void * arg)
{
	UNUSED(arg);

	nanvix_mutex_lock(&mutex);

		/* Count me. */
		thread_counter++;

		/* Am I the last? Sleep. */
		if (thread_counter < thread_amount)
			nanvix_cond_wait(&cond_var, &mutex);

		/* Wake up everyone. */
		else
			nanvix_cond_broadcast(&cond_var);

	nanvix_mutex_unlock(&mutex);

	return (NULL);
}

/*============================================================================*
 * Condvar Unit Tests                                                         *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_api_condvar_init()                                                    *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test for condition variable
 *
 * Tests condition variable initialization
 */
PRIVATE void test_api_condvar_init(void)
{
	test_assert(nanvix_cond_init(&cond_var) == 0);
	test_assert(nanvix_cond_destroy(&cond_var) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_condvar_signal()                                                  *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test signal for condition variable.
 *
 * Tests condition variable behavior using two threads
 */
PRIVATE void test_api_condvar_signal(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[2];

	/* First thread must wait. */
	thread_condition = true;

	test_assert(nanvix_cond_init(&cond_var) == 0);
	test_assert(nanvix_mutex_init(&mutex, NULL) == 0);

		for (int i = 0; i < 2; i++)
			test_assert(kthread_create(&tids[i], condvar_signal, NULL) == 0);

		for (int i = 0; i < 2; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

	test_assert(nanvix_mutex_destroy(&mutex) == 0);
	test_assert(nanvix_cond_destroy(&cond_var) == 0);
#endif
}

/*----------------------------------------------------------------------------*
 * test_api_condvar_broadcast()                                               *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test broadcast for condition variable.
 *
 * Tests condition variable behavior using two threads
 */
PRIVATE void test_api_condvar_broadcast(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[2];

	/* First thread must wait. */
	thread_amount  = 2;
	thread_counter = 0;

	test_assert(nanvix_cond_init(&cond_var) == 0);
	test_assert(nanvix_mutex_init(&mutex, NULL) == 0);

		for (int i = 0; i < 2; i++)
			test_assert(kthread_create(&tids[i], condvar_broadcast, NULL) == 0);

		for (int i = 0; i < 2; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

	test_assert(nanvix_mutex_destroy(&mutex) == 0);
	test_assert(nanvix_cond_destroy(&cond_var) == 0);
#endif
}

/*============================================================================*
 * Condvar Fault Tests                                                        *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_fault_condvar_init()                                                  *
 *----------------------------------------------------------------------------*/

PRIVATE void test_fault_condvar_init(void)
{
	test_assert(nanvix_cond_init(NULL) < 0);
	test_assert(nanvix_cond_destroy(NULL) < 0);
}

/*----------------------------------------------------------------------------*
 * test_fault_condvar_operations()                                            *
 *----------------------------------------------------------------------------*/

PRIVATE void test_fault_condvar_operations(void)
{
	test_assert(nanvix_cond_signal(NULL) < 0);
	test_assert(nanvix_cond_broadcast(NULL) < 0);

	test_assert(nanvix_cond_init(&cond_var) == 0);
	test_assert(nanvix_mutex_init(&mutex, NULL) == 0);
		test_assert(nanvix_cond_wait(&cond_var, NULL) < 0);
		test_assert(nanvix_cond_wait(NULL, &mutex) < 0);
	test_assert(nanvix_mutex_destroy(&mutex) == 0);
	test_assert(nanvix_cond_destroy(&cond_var) == 0);
}

/*============================================================================*
 * Condvar Stress Tests                                                       *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_stress_condvar_signal()                                               *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test signal for condition variable
 *
 * Tests condition variable behavior using more than two threads
 */
PRIVATE void test_stress_condvar_signal(void)
{
#if (THREAD_MAX > 2)
	int nthreads = NTHREADS;
	kthread_t tids[NTHREADS];

	/* Pair of threads. */
	nthreads = NTHREADS - (NTHREADS % 2);

	/* First thread must wait. */
	thread_condition = true;

	test_assert(nanvix_cond_init(&cond_var) == 0);
	test_assert(nanvix_mutex_init(&mutex, NULL) == 0);

		for (int i = 0; i < nthreads; i++)
			test_assert(kthread_create(&tids[i], condvar_signal, NULL) == 0);

		for (int i = 0; i < nthreads; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

	test_assert(nanvix_mutex_destroy(&mutex) == 0);
	test_assert(nanvix_cond_destroy(&cond_var) == 0);
#endif
}

/*----------------------------------------------------------------------------*
 * test_stress_condvar_broadcast()                                            *
 *----------------------------------------------------------------------------*/

/*
 * @brief Test broadcast for condition variable.
 *
 * Tests condition variable behavior using more than two threads
 */
PRIVATE void test_stress_condvar_broadcast(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[NTHREADS];

	/* Config. */
	thread_amount    = (CORES_NUM - 2);
	thread_condition = true;

	test_assert(nanvix_cond_init(&cond_var) == 0);
	test_assert(nanvix_mutex_init(&mutex, NULL) == 0);

		for (int i = 0; i < thread_amount; i++)
			test_assert(kthread_create(&tids[i], condvar_broadcast, NULL) == 0);

		for (int i = 0; i < thread_amount; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

	test_assert(nanvix_mutex_destroy(&mutex) == 0);
	test_assert(nanvix_cond_destroy(&cond_var) == 0);
#endif
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
PRIVATE struct test condition_variables_tests_api[] = {
	{ test_api_condvar_init,      "[test][condvar][api] condition variable init            [passed]" },
	{ test_api_condvar_signal,    "[test][condvar][api] Wait/Signal between two threads    [passed]" },
	{ test_api_condvar_broadcast, "[test][condvar][api] Wait/Broadcast between two threads [passed]" },
	{ NULL,                        NULL                                                               },
};

/**
 * @brief Fault tests.
 */
PRIVATE struct test condition_variables_tests_fault[] = {
	{ test_fault_condvar_init,       "[test][condvar][fault] condition variable init [passed]" },
	{ test_fault_condvar_operations, "[test][condvar][fault] Wait/Signal/Broadcast   [passed]" },
	{ NULL,                           NULL                                                     },
};

/**
 * @brief Stress tests.
 */
PRIVATE struct test condition_variables_tests_stress[] = {
	{ test_stress_condvar_signal,    "[test][condvar][stress] Wait/Signal between pairs of threads [passed]" },
	{ test_stress_condvar_broadcast, "[test][condvar][stress] Wait/Broadcast among several threads [passed]" },
	{ NULL,                           NULL                                                                   },
};

#endif  /* CORES_NUM */

/**
 * @brief Condition variable tests laucher
 */
PUBLIC void test_condition_variables(void)
{
#if (CORES_NUM > 1)

	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; condition_variables_tests_api[i].test_fn != NULL; i++)
	{
		condition_variables_tests_api[i].test_fn();
		nanvix_puts(condition_variables_tests_api[i].name);
	}

	/* Fault Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; condition_variables_tests_fault[i].test_fn != NULL; i++)
	{
		condition_variables_tests_fault[i].test_fn();
		nanvix_puts(condition_variables_tests_fault[i].name);
	}

	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; condition_variables_tests_stress[i].test_fn != NULL; i++)
	{
		condition_variables_tests_stress[i].test_fn();
		nanvix_puts(condition_variables_tests_stress[i].name);
	}

#endif  /* CORES_NUM */
}

