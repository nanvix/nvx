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

#include <nanvix/runtime/fence.h>
#include <posix/errno.h>

#if (CORES_NUM > 1)

/**
 * @see nanvix_fence_init() in nanvix/runtime/fence.h
 */
PUBLIC int nanvix_fence_init(struct nanvix_fence * b, int nthreads)
{
	/* Invalide fence. */
	if (b == NULL)
		return (-EINVAL);

	/* Invalide number of threads. */
	if (!WITHIN(nthreads, 1, (THREAD_MAX + 1)))
		return (-EINVAL);

	b->release  = 0;
	b->nreached = 0;
	b->nthreads = nthreads;
	nanvix_mutex_init(&b->mutex, NULL);
	nanvix_cond_init(&b->cond);

	/* Success. */
	return (0);
}

/**
 * @see nanvix_fence_destroy() in nanvix/runtime/fence.h
 */
PUBLIC int nanvix_fence_destroy(struct nanvix_fence * b)
{
	/* Invalide fence. */
	if (b == NULL)
		return (-EINVAL);

	b->release  = 0;
	b->nreached = 0;
	b->nthreads = (THREAD_MAX + 1);
	nanvix_mutex_destroy(&b->mutex);
	nanvix_cond_destroy(&b->cond);

	/* Success. */
	return (0);
}

/**
 * @see nanvix_fence() in nanvix/runtime/fence.h
 */
PUBLIC int nanvix_fence(struct nanvix_fence * b)
{
	int local_release;

	/* Invalide fence. */
	if (b == NULL)
		return (-EINVAL);

	/* Invalide number of threads. */
	if (!WITHIN(b->nthreads, 1, (THREAD_MAX + 1)))
		return (-EINVAL);

	nanvix_mutex_lock(&b->mutex);

		/* Gets release condition. */
		local_release = !b->release;

		/* Notifies arrival. */
		b->nreached++;

		/* Last thread? */
		if (b->nreached == b->nthreads)
		{
			/* Resets number of reached threads. */
			b->nreached = 0;

			/* Update wait condition. */
			b->release = local_release;

			/* Wake up other threads. */
			nanvix_cond_broadcast(&b->cond);
		}

		/* Until all the threads arrive, sleep. */
		while (local_release != b->release)
			nanvix_cond_wait(&b->cond, &b->mutex);

	nanvix_mutex_unlock(&b->mutex);

	/* Success. */
	return (0);
}

#endif /* CORES_NUM > 1 */

