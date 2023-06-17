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
	/* Invalid task. */
	if (UNLIKELY(!t))
		return (-EINVAL);

	return (kcall1(nr_syscall, t));
}

/*============================================================================*
 * ktask_current()                                                            *
 *============================================================================*/

PUBLIC ktask_t * ktask_current(void)
{
	ktask_t * curr = NULL;

	kcall1(NR_task_current, (word_t) &curr);

	return (curr);
}

/*============================================================================*
 * ktask_create()                                                             *
 *============================================================================*/

/*
 * @see ktask_create()
 */
PUBLIC int ktask_create(
	ktask_t * task,
	ktask_fn fn,
	int priority,
	int period,
	char releases
)
{
	/* Invalid task. */
	if (UNLIKELY(task == NULL || fn == NULL))
		return (-EINVAL);

	return (kcall5(NR_task_create,
		(word_t) task,
		(word_t) fn,
		(word_t) priority,
		(word_t) period,
		(word_t) releases
	));
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
 * ktask_connect()                                                            *
 *============================================================================*/

/*
 * @see ktask_connect()
 */
PUBLIC int ktask_connect(
	ktask_t * parent,
	ktask_t * child,
	bool is_dependency,
	bool is_temporary,
	char triggers
)
{
	/* Invalid tasks. */
	if (UNLIKELY(parent == NULL || child == NULL))
		return (-EINVAL);

	/* Invalid triggers. */
	if (UNLIKELY(!(triggers & KTASK_TRIGGER_ALL)))
		return (-EINVAL);

	return (kcall5(NR_task_connect,
		(word_t) parent,
		(word_t) child,
		(word_t) is_dependency,
		(word_t) is_temporary,
		(word_t) triggers
	));
}

/*============================================================================*
 * ktask_disconnect()                                                         *
 *============================================================================*/

/*
 * @see ktask_disconnect()
 */
PUBLIC int ktask_disconnect(ktask_t * parent, ktask_t * child)
{
	/* Invalid tasks. */
	if (UNLIKELY(parent == NULL || child == NULL))
		return (-EINVAL);

	return (kcall2(NR_task_disconnect, (word_t) parent, (word_t) child));
}

/*============================================================================*
 * ktask_dispatch()                                                           *
 *============================================================================*/

/*
 * @see ktask_dispatch()
 */
PUBLIC int ktask_dispatch(
	ktask_t * task,
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	word_t args[KTASK_ARGS_NUM];

	/* Invalid pointer. */
	if (UNLIKELY(task == NULL))
		return (-EINVAL);

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;
	args[4] = arg4;

	return (kcall2(NR_task_dispatch, (word_t) task, (word_t) args));
}

/*============================================================================*
 * ktask_emit()                                                               *
 *============================================================================*/

/*
 * @see ktask_emit()
 */
PUBLIC int ktask_emit(
	ktask_t * task,
	int coreid,
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	word_t args[KTASK_ARGS_NUM];

	/* Invalid arguments. */
	if (UNLIKELY(task == NULL || !WITHIN(coreid, 0, CORES_NUM)))
		return (-EINVAL);

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;
	args[4] = arg4;

	return (kcall3(NR_task_emit, (word_t) task, (word_t) coreid, (word_t) args));
}

/*============================================================================*
 * ktask_exit()                                                               *
 *============================================================================*/

/*
 * @see ktask_exit()
 */
PUBLIC int ktask_exit(
	int retval,
	int management,
	ktask_merge_args_fn fn,
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	word_t args[KTASK_ARGS_NUM];

	/* Invalid arguments. */
	if (UNLIKELY(!WITHIN(management, KTASK_MANAGEMENT_USER0, ((KTASK_MANAGEMENT_ERROR << 1) - 1))))
		return (-EINVAL);

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;
	args[4] = arg4;

	return (kcall4(NR_task_exit,
		(word_t) retval,
		(word_t) management,
		(word_t) fn,
		(word_t) args)
	);
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
 * ktask_trywait()                                                            *
 *============================================================================*/

/*
 * @see ktask_trywait()
 */
PUBLIC int ktask_trywait(ktask_t * task)
{
	return (__ktask_call1(NR_task_trywait, (word_t) task));
}

/*============================================================================*
 * ktask_stop()                                                               *
 *============================================================================*/

PUBLIC int ktask_stop(ktask_t * task)
{
	return (__ktask_call1(NR_task_stop, (word_t) task));
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

PUBLIC int ktask_complete(ktask_t * task, char management)
{
	/* Invalid task. */
	if (UNLIKELY(!task))
		return (-EINVAL);

	/* Invalid management. */
	if (UNLIKELY(management == 0))
		return (-EINVAL);

	return (kcall2(NR_task_complete, (word_t) task, (word_t) management));
}

