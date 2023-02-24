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

#if (CORES_NUM > 1)

/**
 * @brief Number of iterations.
 */
#ifdef __k1bdp__
	#define CITERATIONS (250)
#else
	#define CITERATIONS (2500)
#endif

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define OBJSIZE                        (128) /**< Maximum Object Size                    */
#define BUFLEN                           16  /**< Buffer Length                          */
#define NOBJECTS                         32  /**< Number of Objects                      */
#define WORD_OBJSIZE   (OBJSIZE / WORD_SIZE) /**< Size of a object in words              */
#define BUFFER_SIZE  (BUFLEN * WORD_OBJSIZE) /**< Buffer data size from objects          */
/**@}*/

/**
 * @brief Semaphores used in tests
 */
PRIVATE struct nanvix_semaphore sem;

#if (THREAD_MAX > 2)

/**
 * @brief Global variable used in semaphore tests
 */
PRIVATE int counter;

/**
 * @brief Buffer.
 */
struct buffer
{
	size_t first;
	size_t last;
	struct nanvix_semaphore lock;
	struct nanvix_semaphore full;
	struct nanvix_semaphore empty;
	word_t data[BUFFER_SIZE];
};

/**
 * @brief Task info.
 */
struct tdata
{
	int n;
	int tnum;
	bool verify;
	struct buffer * buf;
	word_t data[WORD_OBJSIZE];
} tdata[TESTS_NTHREADS] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Buffers.
 */
PRIVATE struct buffer buffers[TESTS_NTHREADS / 2];

/*============================================================================*
 * Threads                                                                    *
 *============================================================================*/

/**
 * @brief Down test.
 */
PRIVATE void * counter_down_task(void * arg)
{
	UNUSED(arg);

	for (int i = 0; i < CITERATIONS; i++)
	{
		test_assert(nanvix_semaphore_down(&sem) == 0);
			counter++;
		test_assert(nanvix_semaphore_up(&sem) == 0);
	}

	return NULL;
}

/**
 * @brief Trydown test.
 */
PRIVATE void *counter_trydown_task(void * arg)
{
	UNUSED(arg);

	for (int i = 0; i < CITERATIONS; i++)
	{
		while (nanvix_semaphore_trydown(&sem) != 0);
		counter++;
		nanvix_semaphore_up(&sem);
	}

	return NULL;
}

/**
 * @brief Initializes a buffer.
 */
PRIVATE void buffer_init(struct buffer * buf, int nslots)
{
	buf->first = 0;
	buf->last  = 0;
	nanvix_semaphore_init(&buf->lock, 1);
	nanvix_semaphore_init(&buf->full, 0);
	nanvix_semaphore_init(&buf->empty, nslots - 1);
}

/**
 * @brief Initializes a buffer.
 */
PRIVATE void buffer_destroy(struct buffer * buf)
{
	nanvix_semaphore_destroy(&buf->lock);
	nanvix_semaphore_destroy(&buf->full);
	nanvix_semaphore_destroy(&buf->empty);
}

/**
 * @brief Puts an object in a buffer.
 *
 * @param buf  Target buffer.
 * @param data Data to place in the buffer.
 */
PRIVATE inline void buffer_put(struct buffer * buf, const word_t * data)
{
	kmemcpy(&buf->data[buf->first], data, OBJSIZE);
	buf->first = (buf->first + WORD_OBJSIZE) % BUFFER_SIZE;
}

/**
 * @brief Gets an object from a buffer.
 *
 * @param buf  Target buffer.
 * @param data Location to store the data.
 */
PRIVATE inline void buffer_get(struct buffer *buf, word_t *data)
{
	kmemcpy(data, &buf->data[buf->last], OBJSIZE);
	buf->last = (buf->last + WORD_OBJSIZE) % BUFFER_SIZE;
}

/**
 * @brief Producer.
 */
PRIVATE void * producer(void * arg)
{
	struct tdata * t    = arg;
	struct buffer * buf = t->buf;

	do
	{
		for (size_t i = 0; i < WORD_OBJSIZE; ++i)
			t->data[i] = (word_t) (t->verify ? t->n : -1);

		nanvix_semaphore_down(&buf->empty);
			nanvix_semaphore_down(&buf->lock);

				buffer_put(buf, t->data);

			nanvix_semaphore_up(&buf->lock);
		nanvix_semaphore_up(&buf->full);

	} while (--t->n > 0);

	return NULL;
}

/**
 * @brief Consumer.
 */
PRIVATE void * consumer(void * arg)
{
	struct tdata * t    = arg;
	struct buffer * buf = t->buf;

	do
	{
		for (size_t i = 0; i < WORD_OBJSIZE; ++i)
			t->data[i] = (word_t) -1;

		nanvix_semaphore_down(&buf->full);
			nanvix_semaphore_down(&buf->lock);

				buffer_get(buf, t->data);

			nanvix_semaphore_up(&buf->lock);
		nanvix_semaphore_up(&buf->empty);

		for (size_t i = 0; i < WORD_OBJSIZE; ++i)
			test_assert(t->data[i] == (word_t) (t->verify ? t->n : -1));

	} while (--t->n > 0);

	return NULL;
}

#endif /* THREADS_MAX > 2 */

/*============================================================================*
 * Semaphore Unit Tests                                                       *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_api_semaphore_init()                                                  *
 *----------------------------------------------------------------------------*/

/*
 * @brief API Init and destroy  a semaphore.
 */
PRIVATE void test_api_semaphore_init(void)
{
	test_assert(nanvix_semaphore_init(&sem, 1) == 0);
	test_assert(nanvix_semaphore_destroy(&sem) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_semaphore_up_down()                                               *
 *----------------------------------------------------------------------------*/

/*
 * @brief API Init and destroy  a semaphore.
 */
PRIVATE void test_api_semaphore_up_down(void)
{
	test_assert(nanvix_semaphore_init(&sem, 1) == 0);

		/* Normal. */
		test_assert(nanvix_semaphore_down(&sem) == 0);
		test_assert(nanvix_semaphore_up(&sem) == 0);

		/* Try wait. */
		test_assert(nanvix_semaphore_trydown(&sem) == 0);
		test_assert(nanvix_semaphore_trydown(&sem) < 0);

		/* Conter. */
		test_assert(nanvix_semaphore_up(&sem) == 0);
		test_assert(nanvix_semaphore_up(&sem) == 0);
			test_assert(nanvix_semaphore_down(&sem) == 0);
			test_assert(nanvix_semaphore_up(&sem) == 0);
		test_assert(nanvix_semaphore_down(&sem) == 0);
		test_assert(nanvix_semaphore_down(&sem) == 0);
		test_assert(nanvix_semaphore_trydown(&sem) < 0);

	test_assert(nanvix_semaphore_destroy(&sem) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_semaphore_producer_consumer()                                     *
 *----------------------------------------------------------------------------*/

/*
 * @brief API Init and destroy  a semaphore.
 */
PRIVATE void test_api_semaphore_producer_consumer(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[2];

	struct buffer * buf;

	buffer_init(buf = &buffers[0], BUFLEN);

	tdata[0].tnum   = 0;
	tdata[1].tnum   = 1;
	tdata[0].n      = tdata[1].n      = NOBJECTS;
	tdata[0].buf    = tdata[1].buf    = buf;
	tdata[0].verify = tdata[1].verify = true;

	test_assert(kthread_create(&tids[0], producer, &tdata[0]) == 0);
	test_assert(kthread_create(&tids[1], consumer, &tdata[1]) == 0);

	test_assert(kthread_join(tids[0], NULL) == 0);
	test_assert(kthread_join(tids[1], NULL) == 0);

	buffer_destroy(buf);
#endif
}

/*============================================================================*
 * Semaphore Fault Tests                                                      *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_fault_semaphore_init()                                                *
 *----------------------------------------------------------------------------*/

/*
 * @brief Fault test for init and destroy.
 */
PRIVATE void test_fault_semaphore_init(void)
{
	test_assert(nanvix_semaphore_init(NULL, 1) < 0);
	test_assert(nanvix_semaphore_init(&sem, -1) < 0);
	test_assert(nanvix_semaphore_destroy(NULL) < 0);
}

/*----------------------------------------------------------------------------*
 * test_fault_semaphore_up_down()                                             *
 *----------------------------------------------------------------------------*/

/*
 * @brief Fault test for up/down/trydown.
 */
PRIVATE void test_fault_semaphore_up_down(void)
{
	test_assert(nanvix_semaphore_up(NULL) < 0);
	test_assert(nanvix_semaphore_down(NULL) < 0);
	test_assert(nanvix_semaphore_trydown(NULL) < 0);
}

/*============================================================================*
 * Semaphore Stress Tests                                                     *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_stress_semaphore_up_down()                                            *
 *----------------------------------------------------------------------------*/

/*
 * @brief Stress test for up/down.
 */
PRIVATE void test_stress_semaphore_up_down(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[TESTS_NTHREADS];

	counter = 0;
	test_assert(nanvix_semaphore_init(&sem, 1) == 0);

		for (int i = 0; i < TESTS_NTHREADS; i++)
			test_assert(kthread_create(&tids[i], counter_down_task, NULL) == 0);

		for (int i = 0; i < TESTS_NTHREADS; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

		test_assert(counter == TESTS_NTHREADS * CITERATIONS);

	test_assert(nanvix_semaphore_destroy(&sem) == 0);
#endif
}

/*----------------------------------------------------------------------------*
 * test_stress_semaphore_trydown()                                            *
 *----------------------------------------------------------------------------*/

/*
 * @brief Stress test for trydown.
 */
PRIVATE void test_stress_semaphore_trydown(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[TESTS_NTHREADS];

	counter = 0;
	test_assert(nanvix_semaphore_init(&sem, 1) == 0);

		for (int i = 0; i < TESTS_NTHREADS; i++)
			test_assert(kthread_create(&tids[i], counter_trydown_task, NULL) == 0);

		for (int i = 0; i < TESTS_NTHREADS; i++)
			test_assert(kthread_join(tids[i], NULL) == 0);

		test_assert(counter == TESTS_NTHREADS * CITERATIONS);

	test_assert(nanvix_semaphore_destroy(&sem) == 0);
#endif
}

/*----------------------------------------------------------------------------*
 * test_stress_semaphore_producer_consumer()                                  *
 *----------------------------------------------------------------------------*/

/*
 * @brief Stress test for producer/consumer.
 */
PRIVATE void test_stress_semaphore_producer_consumer(void)
{
#if (THREAD_MAX > 2)
	kthread_t tids[TESTS_NTHREADS];

	for (int i = 0; (i + 1) < TESTS_NTHREADS; i += 2)
	{
		struct buffer * buf;

		buffer_init(buf = &buffers[i / 2], BUFLEN);

		tdata[i].tnum     = i;
		tdata[i + 1].tnum = i + 1;
		tdata[i].n      = tdata[i + 1].n      = NOBJECTS;
		tdata[i].buf    = tdata[i + 1].buf    = buf;
		tdata[i].verify = tdata[i + 1].verify = false;

		test_assert(kthread_create(&tids[i], producer, &tdata[i]) == 0);
		test_assert(kthread_create(&tids[i + 1], consumer, &tdata[i + 1]) == 0);
	}

	for (int i = 0; (i + 1) < TESTS_NTHREADS; i += 2)
	{
		test_assert(kthread_join(tids[i], NULL) == 0);
		test_assert(kthread_join(tids[i + 1], NULL) == 0);

		buffer_destroy(&buffers[i / 2]);
	}
#endif
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief Stress tests.
 */
PRIVATE struct test test_api_semaphore[] = {
	{ test_api_semaphore_init,              "[test][semaphore][api] Init/Destroy      [passed]" },
	{ test_api_semaphore_up_down,           "[test][semaphore][api] Up/Down           [passed]" },
	{ test_api_semaphore_producer_consumer, "[test][semaphore][api] Producer-Consumer [passed]" },
	{ NULL,                                  NULL                                               },
};

/**
 * @brief Fault tests.
 */
PRIVATE struct test test_fault_semaphore[] = {
	{ test_fault_semaphore_init,              "[test][semaphore][fault] Init/Destroy    [passed]" },
	{ test_fault_semaphore_up_down,           "[test][semaphore][fault] Up/Down/trydown [passed]" },
	{ NULL,                                    NULL                                               },
};

/**
 * @brief Stress tests.
 */
PRIVATE struct test test_stress_semaphore[] = {
	{ test_stress_semaphore_up_down,           "[test][semaphore][stress] Up/Down [passed]"           },
	{ test_stress_semaphore_trydown,           "[test][semaphore][stress] trydown [passed]"           },
	{ test_stress_semaphore_producer_consumer, "[test][semaphore][stress] Producer-Consumer [passed]" },
	{ NULL,                                     NULL                                                  },
};

#endif /* CORES_NUM > 1 */

/**
 * @brief Semaphore test launcher
 */
PUBLIC void test_semaphore(void)
{
#if (CORES_NUM > 1)

	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; test_api_semaphore[i].test_fn != NULL; i++)
	{
		test_api_semaphore[i].test_fn();
		nanvix_puts(test_api_semaphore[i].name);
	}

	/* Fault Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; test_fault_semaphore[i].test_fn != NULL; i++)
	{
		test_fault_semaphore[i].test_fn();
		nanvix_puts(test_fault_semaphore[i].name);
	}

	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; test_stress_semaphore[i].test_fn != NULL; i++)
	{
		test_stress_semaphore[i].test_fn();
		nanvix_puts(test_stress_semaphore[i].name);
	}

#endif /* CORES_NUM > 1 */
}

