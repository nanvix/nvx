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

#include <nanvix/sys/portal.h>

#if __TARGET_HAS_PORTAL

#include <nanvix/sys/thread.h>
#include <nanvix/sys/noc.h>
#include <nanvix/runtime/stdikc.h>

/**
 * @brief Kernel standard input portal.
 */
static int __stdinportal[THREAD_MAX + 1] = {
	[0 ... THREAD_MAX] = -1
};

/**
 * @todo TODO: provide a detailed description for this function.
 */
int stdinportal_get_port(void)
{
	int port = (kthread_self() - (SYS_THREAD_MAX - 1));

	/* Kernel thread ? 0 else port. */
	return ((port <= 0) ? 0 : port);
}

/**
 * @todo TODO: provide a detailed description for this function.
 */
int __stdportal_setup(void)
{
	int tid;

	if ((tid = stdinportal_get_port()) > (THREAD_MAX + 1))
		return (-1);

	if (__stdinportal[tid] >= 0)
		return (__stdinportal[tid]);

	__stdinportal[tid] = kportal_create(knode_get_num(), tid);

	return ((__stdinportal[tid] < 0) ? -1 : 0);
}

/**
 * @todo TODO: provide a detailed description for this function.
 */
int __stdportal_cleanup(void)
{
	int tid;

	if ((tid = stdinportal_get_port()) > (THREAD_MAX + 1))
		return (-1);

	if (__stdinportal[tid] < 0)
		return (-1);

	return (kportal_unlink(__stdinportal[tid]));
}

/**
 * @todo TODO: provide a detailed description for this function.
 */
int stdinportal_get(void)
{
	int tid;

	if ((tid = stdinportal_get_port()) > (THREAD_MAX + 1))
		return (-1);

	return (__stdinportal[tid]);
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_PORTAL */
