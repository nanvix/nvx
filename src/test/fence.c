/*
 * MIT License
 *
 * Copyright(c) 2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
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

#include <nanvix/sys/sync.h>
#include <nanvix/sys/mutex.h>
#include <posix/errno.h>
#include "test.h"

#if (__TARGET_HAS_SYNC)

/**
 * @brief Initializes a fence.
 *
 * @param b      Target fence.
 * @param ncores Number of cores in the fence.
 */
PUBLIC void fence_init(struct fence *b, int ncores)
{
	b->ncores   = ncores;
	b->nreached = 0;
	b->release  = 0;
	nanvix_mutex_init(&b->lock);
}

/**
 * @brief Waits in a fence.
 */
PUBLIC void fence(struct fence *b)
{
	int exit;
	int local_release;

	/* Notifies thread reach. */
	nanvix_mutex_lock(&b->lock);

		local_release = !b->release;

		b->nreached++;

		if (b->nreached == b->ncores)
		{
			b->nreached = 0;
			b->release  = local_release;
		}

	nanvix_mutex_unlock(&b->lock);

	do
	{
		nanvix_mutex_lock(&b->lock);
			exit = (local_release == b->release);
		nanvix_mutex_unlock(&b->lock);
	} while (!exit);
}

#endif /* __TARGET_HAS_SYNC */
