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
bool wait_signal_condition = false;

/*
 * @brief Condition variable used in tests
 */
struct nanvix_cond_var cond_var;

/*
 * @brief Mutex used in condition variable tests
 */
struct nanvix_mutex mutex;


/*
 * @brief Task called in condition variable tests
 */
static void *task(void *arg)
{
	UNUSED(arg);
	nanvix_mutex_lock(&mutex);
	bool local_condition = wait_signal_condition;
	wait_signal_condition = !wait_signal_condition;
	if (!local_condition)
	{
		nanvix_cond_wait(&cond_var, &mutex);
	}
	else
	{
		nanvix_cond_signal(&cond_var);
	}
	nanvix_mutex_unlock(&mutex);
	return (NULL);
}

/*
 * @brief Test for condition variable
 *
 * Tests condition variable initialization
 */
static void test_api_condvar_init(void)
{
	test_assert(nanvix_cond_init(&cond_var) == 0);
	test_assert(nanvix_mutex_init(&mutex) == 0);
}

/*
 * @brief Test for condition variable
 *
 * Tests condition variable behavior using two threads
 */
static void test_api_condvar(void)
{
	#if (THREAD_MAX > 2)
		kthread_t tids[2];
		wait_signal_condition = false;

		for (int i = 0; i < 2; i++)
			kthread_create(&tids[i], task, NULL);

		for (int i = 0; i < 2; i++)
			kthread_join(tids[i], NULL);
	#endif
}

/*
 * @brief Test for condition variable
 *
 * Tests condition variable behavior using more than two threads
 */
static void test_stress_condvar(void)
{
	#if (THREAD_MAX > 2)
		kthread_t tids[NTHREADS];
		int n_threads = NTHREADS;

		if (!((NTHREADS % 2) == 0))
		{
			n_threads = NTHREADS - 1;
		}

		wait_signal_condition = false;

		for (int i = 0; i < n_threads; i++) {
			kthread_create(&tids[i], task, NULL);
		}

		for (int i = 0; i < n_threads; i++)
			kthread_join(tids[i], NULL);
	#endif
}

/**
 * @brief API tests.
 */
static struct test condition_variables_tests_api[] = {
	{ test_api_condvar_init, "[test][condvar][api] condition variable init               [passed]" },
	{ test_api_condvar,      "[test][condvar][api] condition variable single thread pair [passed]" },
	{ NULL,                  NULL                                                        },
};

/**
 * @brief Stress tests.
 */
static struct test condition_variables_tests_stress[] = {
	{ test_stress_condvar, "[test][condvar][stress] condition variable multiple thread pairs [passed]" },
	{ NULL,                NULL                                                        },
};

/**
 * @brief Condition variable tests laucher
 */
void test_condition_variables(void)
{
	/* API Tests */
	for (int i = 0; condition_variables_tests_api[i].test_fn != NULL; i++)
	{
		condition_variables_tests_api[i].test_fn();
		nanvix_puts(condition_variables_tests_api[i].name);
	}

	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; condition_variables_tests_stress[i].test_fn != NULL; i++)
	{
		condition_variables_tests_stress[i].test_fn();
		nanvix_puts(condition_variables_tests_stress[i].name);
	}
}

#endif  /* CORES_NUM */
