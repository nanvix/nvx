/*
 * MIT License
 *
 * Copyright(c) 2011-2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
 *              2015-2016 Davidson Francis     <davidsondfgl@gmail.com>
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

#include <nanvix/sys/semaphore.h>
#include "test.h"


/**
 * @brief Global variable used in semaphore tests
 */
int counter;

/**
 * @brief Semaphore used in tests
 */
struct nanvix_semaphore sem;

/**
 * @brief Trywait test task
 */
void *task(void *arg)
{
	int iterations = *(int *)arg;
	for (int i = 0; i < iterations; i++)
	{
		while (nanvix_semaphore_trywait(&sem) != 0);
		counter++;
		nanvix_semaphore_up(&sem);
	}
	return NULL;
}

/*
 * @brief Stress test for trywait
 */
static void test_stress_trywait(void)
{
	#if (THREAD_MAX > 2)
		int iterations = 5000000/NTHREADS;
		kthread_t tids[NTHREADS];
		nanvix_semaphore_init(&sem, 1);

		for (int i = 0; i < NTHREADS; i++)
			kthread_create(&tids[i], task, (void *) &iterations);

		for (int i = 0; i < NTHREADS; i++)
			kthread_join(tids[i], NULL);

		test_assert(counter == NTHREADS * iterations);
	#endif
}

/**
 * @brief Stress tests.
 */
static struct test test_stress_semaphore[] = {
	{ test_stress_trywait, "[test][semaphore][stress] trywait [passed]" },
	{ NULL,                NULL                                         },
};

/**
 * @brief Semaphore test launcher
 */
void test_semaphore(void)
{
	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; test_stress_semaphore[i].test_fn != NULL; i++)
	{
		test_stress_semaphore[i].test_fn();
		nanvix_puts(test_stress_semaphore[i].name);
	}
}
