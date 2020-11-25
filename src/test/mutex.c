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

#include <nanvix/sys/mutex.h>
#include <nanvix/sys/thread.h>
#include "test.h"

#if (CORES_NUM > 1)

#define TEST_LOCK    0
#define TEST_UNLOCK  1
#define TEST_TRYLOCK 2

/**
 * @brief Context to mutex test functions
 */
struct task_context
{
	struct nanvix_mutex *mutex;  /**< mutex */
	int function;                /**< mutex function */
	int success;                 /**< should the function work? */
};

/**
 * @brief Test for recursive mutex
 *
 * @param mutex Target mutex
 * @param i Number of iterations
 */
static void test_mutex_recursive(struct nanvix_mutex *mutex, int i)
{
	nanvix_mutex_lock(mutex);
	test_assert(mutex->owner == kthread_self());
	test_assert((int) mutex->locked == 1);
	if (i != 0)
		test_mutex_recursive(mutex, i - 1);
	nanvix_mutex_unlock(mutex);
}

/**
 * @brief Recursive task
 *
 * @param arg Target mutex
 */
void *task_recursive(void *arg)
{
	struct nanvix_mutex *mutex = (struct nanvix_mutex *) arg;

	test_mutex_recursive(mutex, 10);

	return (NULL);
}

/**
 * @brief Lock and unlock task
 *
 * @param arg Target mutex
 */
void *task_lock_unlock(void *arg)
{
	struct nanvix_mutex *mutex = (struct nanvix_mutex *) arg;

	nanvix_mutex_lock(mutex);
	test_assert(mutex->owner == kthread_self());
	test_assert((int) mutex->locked == 1);
	nanvix_mutex_unlock(mutex);

	return (NULL);
}

/**
 * @brief General mutex function task
 *
 * Performs a single mutex function: lock, unlock or trylock
 *
 * @param arg Context for the task
 */
void *task_mutex_function(void *arg)
{
	struct task_context *ctx;
	ctx = (struct task_context *) arg;
	int r;

	switch (ctx->function)
	{
		case TEST_LOCK:
			r = nanvix_mutex_lock(ctx->mutex);
			break;
		case TEST_UNLOCK:
			r = nanvix_mutex_unlock(ctx->mutex);
			break;
		case TEST_TRYLOCK:
			r = nanvix_mutex_trylock(ctx->mutex);
			break;
		default:
			kprintf("invalid function specified");
			r = 1;
	}

	if (ctx->success)
	{
		test_assert(r == 0);
	}
	else
	{
		test_assert(r < 0);
	}

	return (NULL);
}

/**
 * @brief Tests a single mutex function
 *
 * @param mutex Target mutex
 * @param function Target mutex function
 * @param success Should the test work?
 */
void test_mutex_function(struct nanvix_mutex *mutex, int function, int success)
{
	kthread_t tid;
	struct task_context ctx = {mutex, function, success};

	test_assert(kthread_create(&tid, task_mutex_function, (void *) &ctx) == 0);
	test_assert(kthread_join(tid, NULL) == 0);
}

/**
 * @brief API test for normal mutex.
 */
static void test_api_mutex_normal(void)
{
	struct nanvix_mutex mutex;

	test_assert(nanvix_mutex_init(&mutex, NULL) == 0);

	/* unlock unlocked mutex */
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* basic lock unlock */
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* trylock in locked mutex */
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_trylock(&mutex) < 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* trylock in unlocked mutex */
	test_assert(nanvix_mutex_trylock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);
	#if (THREAD_MAX > 2)
		/* lock unlock across threads */
		test_assert(nanvix_mutex_lock(&mutex) == 0);
		test_mutex_function(&mutex, TEST_UNLOCK, 1);

		/* trylock in locked mutex across threads */
		test_assert(nanvix_mutex_lock(&mutex) == 0);
		test_mutex_function(&mutex, TEST_TRYLOCK, 0);
		test_mutex_function(&mutex, TEST_UNLOCK, 1);

		/* trylock in unlocked mutex across threads */
		test_mutex_function(&mutex, TEST_TRYLOCK, 1);
		test_assert(nanvix_mutex_unlock(&mutex) == 0);
	#endif
	test_assert(nanvix_mutex_destroy(&mutex) == 0);
}

/**
 * @brief API test for errorcheck mutex.
 */
static void test_api_mutex_errorcheck(void)
{
	struct nanvix_mutex mutex;
	struct nanvix_mutexattr mattr;

	test_assert(nanvix_mutexattr_init(&mattr) == 0);
	test_assert(nanvix_mutexattr_settype(&mattr, NANVIX_MUTEX_ERRORCHECK) == 0);
	test_assert(nanvix_mutexattr_gettype(&mattr) == NANVIX_MUTEX_ERRORCHECK);
	test_assert(nanvix_mutex_init(&mutex, &mattr) == 0);

	/* unlock unlocked mutex */
	test_assert(nanvix_mutex_unlock(&mutex) < 0);

	/* basic lock unlock */
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* double lock */
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_lock(&mutex) < 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* trylock in locked mutex */
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_trylock(&mutex) < 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* trylock in unlocked mutex */
	test_assert(nanvix_mutex_trylock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	#if (THREAD_MAX > 2)
		/* lock unlock across threads */
		test_assert(nanvix_mutex_lock(&mutex) == 0);
		test_mutex_function(&mutex, TEST_UNLOCK, 0);
		test_assert(nanvix_mutex_unlock(&mutex) == 0);

		/* trylock in locked mutex across threads */
		test_assert(nanvix_mutex_lock(&mutex) == 0);
		test_mutex_function(&mutex, TEST_TRYLOCK, 0);
		test_assert(nanvix_mutex_unlock(&mutex) == 0);
	#endif

	test_assert(nanvix_mutexattr_destroy(&mattr) == 0);
	test_assert(nanvix_mutex_destroy(&mutex) == 0);
}

/**
 * @brief API test for recursive mutex.
 */
static void test_api_mutex_recursive(void)
{
	struct nanvix_mutex mutex;
	struct nanvix_mutexattr mattr;

	test_assert(nanvix_mutexattr_init(&mattr) == 0);
	test_assert(nanvix_mutexattr_settype(&mattr, NANVIX_MUTEX_RECURSIVE) == 0);
	test_assert(nanvix_mutexattr_gettype(&mattr) == NANVIX_MUTEX_RECURSIVE);
	test_assert(nanvix_mutex_init(&mutex, &mattr) == 0);

	/* unlock unlocked mutex */
	test_assert(nanvix_mutex_unlock(&mutex) < 0);

	/* basic lock unlock */
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* double lock */
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* trylock in locked mutex */
	test_assert(nanvix_mutex_lock(&mutex) == 0);
	test_assert(nanvix_mutex_trylock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	/* trylock in unlocked mutex */
	test_assert(nanvix_mutex_trylock(&mutex) == 0);
	test_assert(nanvix_mutex_unlock(&mutex) == 0);

	#if (THREAD_MAX > 2)
		/* lock unlock across threads */
		test_assert(nanvix_mutex_lock(&mutex) == 0);
		test_mutex_function(&mutex, TEST_UNLOCK, 0);
		test_assert(nanvix_mutex_unlock(&mutex) == 0);

		/* trylock in locked mutex across threads */
		test_assert(nanvix_mutex_lock(&mutex) == 0);
		test_mutex_function(&mutex, TEST_TRYLOCK, 0);
		test_mutex_function(&mutex, TEST_UNLOCK, 0);
		test_assert(nanvix_mutex_unlock(&mutex) == 0);
	#endif

	test_assert(nanvix_mutexattr_destroy(&mattr) == 0);
	test_assert(nanvix_mutex_destroy(&mutex) == 0);
}

/**
 * @brief Stress test for normal mutex
 */
static void test_stress_mutex_normal(void)
{
	#if (THREAD_MAX > 2)
		struct nanvix_mutex mutex;
		kthread_t tids[NTHREADS];

		test_assert(nanvix_mutex_init(&mutex, NULL) == 0);

		for (int i = 0; i < NTHREADS; i++)
			kthread_create(&tids[i], task_lock_unlock, (void *) &mutex);
		for (int i = 0; i < NTHREADS; i++)
			kthread_join(tids[i], NULL);

		test_assert(nanvix_mutex_destroy(&mutex) == 0);
	#endif
}

/**
 * @brief Stress test for errorcheck mutex
 */
static void test_stress_mutex_errorcheck(void)
{
	#if (THREAD_MAX > 2)
		struct nanvix_mutex mutex;
		struct nanvix_mutexattr mattr;
		kthread_t tids[NTHREADS];

		test_assert(nanvix_mutexattr_init(&mattr) == 0);
		test_assert(nanvix_mutexattr_settype(&mattr, NANVIX_MUTEX_ERRORCHECK) == 0);
		test_assert(nanvix_mutex_init(&mutex, &mattr) == 0);

		for (int i = 0; i < NTHREADS; i++)
			kthread_create(&tids[i], task_lock_unlock, (void *) &mutex);
		for (int i = 0; i < NTHREADS; i++)
			kthread_join(tids[i], NULL);

		test_assert(nanvix_mutexattr_destroy(&mattr) == 0);
		test_assert(nanvix_mutex_destroy(&mutex) == 0);
	#endif
}

/**
 * @brief Stress test for recursive mutex
 */
static void test_stress_mutex_recursive(void)
{
	#if (THREAD_MAX > 2)
		struct nanvix_mutex mutex;
		struct nanvix_mutexattr mattr;
		kthread_t tids[NTHREADS];

		test_assert(nanvix_mutexattr_init(&mattr) == 0);
		test_assert(nanvix_mutexattr_settype(&mattr, NANVIX_MUTEX_RECURSIVE) == 0);
		test_assert(nanvix_mutex_init(&mutex, &mattr) == 0);

		for (int i = 0; i < NTHREADS; i++)
			kthread_create(&tids[i], task_recursive, (void *) &mutex);
		for (int i = 0; i < NTHREADS; i++)
			kthread_join(tids[i], NULL);

		test_assert(nanvix_mutexattr_destroy(&mattr) == 0);
		test_assert(nanvix_mutex_destroy(&mutex) == 0);
	#endif
}

/**
 * @brief API tests.
 */
static struct test mutex_tests_api[] = {
	{ test_api_mutex_normal,     "[test][mutex][api] mutex normal     [passed]" },
	{ test_api_mutex_errorcheck, "[test][mutex][api] mutex errorcheck [passed]" },
	{ test_api_mutex_recursive,  "[test][mutex][api] mutex recursive  [passed]" },
	{ NULL,                      NULL                                           },
};

/**
 * @brief Stress tests.
 */
static struct test mutex_tests_stress[] = {
	{ test_stress_mutex_normal,     "[test][mutex][stress] mutex normal     [passed]" },
	{ test_stress_mutex_errorcheck, "[test][mutex][stress] mutex errorcheck [passed]" },
	{ test_stress_mutex_recursive,  "[test][mutex][stress] mutex recursive  [passed]" },
	{ NULL,                      NULL                                           },
};

/**
 * @brief Mutex test laucher.
 */
void test_mutex(void)
{
	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; mutex_tests_api[i].test_fn != NULL; i++)
	{
		mutex_tests_api[i].test_fn();
		nanvix_puts(mutex_tests_api[i].name);
	}
	/* stress tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; mutex_tests_stress[i].test_fn != NULL; i++)
	{
		mutex_tests_stress[i].test_fn();
		nanvix_puts(mutex_tests_stress[i].name);
	}
}

#endif  /* CORES_NUM */
