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

/**
 * @addtogroup nanvix Nanvix System
 */
/**@{*/

#ifndef NANVIX_SYS_TASK_H_
#define NANVIX_SYS_TASK_H_

	#include <nanvix/kernel/kernel.h>

	/**
	 * @brief Task Types
	 */
	/**@{*/
	typedef struct task ktask_t;
	typedef struct task_args ktask_args_t;
	typedef int (*ktask_fn)(ktask_args_t *);
	/**@}*/

	/**
	 * @name Task Kernel Calls
	 */
	/**@{*/
	#define ktask_create(task, fn, args, period) task_create(task, fn, args, period)
	#define ktask_get_id(task)                   task_get_id(task)
	#define ktask_get_return(task)               task_get_return(task)
	extern int ktask_unlink(ktask_t *);
	extern int ktask_connect(ktask_t *, ktask_t *);
	extern int ktask_disconnect(ktask_t *, ktask_t *);
	extern int ktask_dispatch(ktask_t *);
	extern int ktask_wait(ktask_t *);
	extern int ktask_continue(ktask_t * task);
	extern int ktask_complete(ktask_t * task);
	extern ktask_t * ktask_current(void);
	/**@}*/

#endif /* NANVIX_SYS_TASK_H_ */

/**@}*/
