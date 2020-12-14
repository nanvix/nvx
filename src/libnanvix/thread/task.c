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

#include <nanvix/sys/task.h>
#include <posix/errno.h>

/*============================================================================*
 * __ktask_call1()                                                            *
 *============================================================================*/

/*
 * @see __ktask_call1()
 */
PRIVATE int __ktask_call1(word_t nr_syscall, word_t t)
{
	int ret;

	/* Invalid task. */
	if (!t)
	{
		errno = (-EINVAL);
		return (-1);
	}

	ret = kcall1(nr_syscall, t);

	/* System call failed. */
	if (ret < 0)
	{
		errno = -ret;
		return (-1);
	}

	return (ret);
}

/*============================================================================*
 * __ktask_call2()                                                            *
 *============================================================================*/

/*
 * @see __ktask_call2()
 */
PRIVATE int __ktask_call2(word_t nr_syscall, word_t t0, word_t t1)
{
	int ret;

	/* Invalid task. */
	if (!t0 || !t1)
	{
		errno = (-EINVAL);
		return (-1);
	}

	ret = kcall2(nr_syscall, t0, t1);

	/* System call failed. */
	if (ret < 0)
	{
		errno = -ret;
		return (-1);
	}

	return (ret);
}

/*============================================================================*
 * ktask_unlink()                                                             *
 *============================================================================*/

/*
 * @see ktask_unlink()
 */
PUBLIC int ktask_unlink(ktask_t * task)
{
	return (__ktask_call1(NR_task_unlink, (word_t) task));
}

/*============================================================================*
 * ktask_dispatch()                                                           *
 *============================================================================*/

/*
 * @see ktask_dispatch()
 */
PUBLIC int ktask_dispatch(ktask_t * task)
{
	return (__ktask_call1(NR_task_dispatch, (word_t) task));
}

/*============================================================================*
 * ktask_wait()                                                               *
 *============================================================================*/

/*
 * @see ktask_wait()
 */
PUBLIC int ktask_wait(ktask_t * task)
{
	return (__ktask_call1(NR_task_wait, (word_t) task));
}

/*============================================================================*
 * ktask_continue()                                                           *
 *============================================================================*/

PUBLIC int ktask_continue(ktask_t * task)
{
	return (__ktask_call1(NR_task_continue, (word_t) task));
}

/*============================================================================*
 * ktask_complete()                                                           *
 *============================================================================*/

PUBLIC int ktask_complete(ktask_t * task)
{
	return (__ktask_call1(NR_task_complete, (word_t) task));
}

/*============================================================================*
 * ktask_current()                                                            *
 *============================================================================*/

PUBLIC ktask_t * ktask_current(void)
{
	ktask_t * curr = NULL;

	if ( __ktask_call1(NR_task_current, (word_t) &curr) < 0)
		return (NULL);

	return (curr);
}

/*============================================================================*
 * ktask_connect()                                                            *
 *============================================================================*/

/*
 * @see ktask_connect()
 */
PUBLIC int ktask_connect(ktask_t * parent, ktask_t * child)
{
	return (__ktask_call2(NR_task_connect, (word_t) parent, (word_t) child));
}

/*============================================================================*
 * ktask_disconnect()                                                         *
 *============================================================================*/

/*
 * @see ktask_disconnect()
 */
PUBLIC int ktask_disconnect(ktask_t * parent, ktask_t * child)
{
	return (__ktask_call2(NR_task_disconnect, (word_t) parent, (word_t) child));
}

