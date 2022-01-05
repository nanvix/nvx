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
	 * @brief Number of arguments in the task function.
	 */
	#define KTASK_ARGS_NUM TASK_ARGS_NUM

	/**
	 * @brief Maximum number of children.
	 */
	#define KTASK_CHILDREN_MAX TASK_CHILDREN_MAX

	/**
	 * @brief Maximum number of parents.
	 */
	#define KTASK_PARENTS_MAX TASK_PARENTS_MAX

	/**
	 * @name Task priority.
	 *
	 * @details Dummy priority:
	 * High: Insert the task in the front of the queue.
	 * Low: Insert the task in the back of the queue.
	 *
	 * TODO: Define a smarter priority mechanism.
	 */
	/**@{*/
	#define KTASK_PRIORITY_LOW     TASK_PRIORITY_LOW  /**< Low priority.     */
	#define KTASK_PRIORITY_HIGH    TASK_PRIORITY_HIGH /**< High priority.    */

	#define KTASK_PRIORITY_DEFAULT TASK_PRIORITY_LOW  /**< Default priority. */
	/**@}*/

	/**
	 * @name Connection's type.
	 *
	 * @details Types' description:
	 * 0: Hard connections are lifetime connections and must be explicitly
	 *    disconnected.
	 * 1: Soft connections are (on-demand) temporary connections that cease
	 *    to exist when the parent completes.
	 * 2: Invalid connection is used internally to specify an invalid node.
	 */
	/**@{*/
	#define KTASK_CONN_IS_PERSISTENT TASK_CONN_IS_PERSISTENT /**< Lifetime connection.  */
	#define KTASK_CONN_IS_TEMPORARY  TASK_CONN_IS_TEMPORARY  /**< Temporary connection. */
	#define KTASK_CONN_IS_DEPENDENCY TASK_CONN_IS_DEPENDENCY /**< Dependency. */
	#define KTASK_CONN_IS_FLOW       TASK_CONN_IS_FLOW       /**< Flow.       */
	/**@}*/

	/**
	 * @name Dependency's trigger condition.
	 *
	 * @details When a dependency is notified.
	 * 0: On a success notification.
	 * 0: On a finish notification.
	 * 0: On a error notification.
	 */
	/**@{*/
	#define KTASK_TRIGGER_USER0       TASK_TRIGGER_USER0       /**< At success.                                  */
	#define KTASK_TRIGGER_USER1       TASK_TRIGGER_USER1       /**< At success.                                  */
	#define KTASK_TRIGGER_USER2       TASK_TRIGGER_USER2       /**< At success.                                  */
	#define KTASK_TRIGGER_AGAIN       TASK_TRIGGER_AGAIN       /**< At reschedule.                               */
	#define KTASK_TRIGGER_STOP        TASK_TRIGGER_STOP        /**< At task stoppage.                            */
	#define KTASK_TRIGGER_PERIODIC    TASK_TRIGGER_PERIODIC    /**< At periodic reschedule.                      */
	#define KTASK_TRIGGER_ERROR_THROW TASK_TRIGGER_ERROR_THROW /**< At error propagation (cannot dispatch child) */
	#define KTASK_TRIGGER_ERROR_CATCH TASK_TRIGGER_ERROR_CATCH /**< At error notification (can dispatch child).  */

	#define KTASK_TRIGGER_ALL         TASK_TRIGGER_ALL         /**< All triggers.                                */
	#define KTASK_TRIGGER_NONE        TASK_TRIGGER_NONE        /**< None trigger.                                */
	#define KTASK_TRIGGER_DEFAULT     TASK_TRIGGER_DEFAULT     /**< Default trigger.                             */

	/**@}*/

	/**
	 * @name Management behaviors on a task.
	 *
	 * @details The management value indicates which action the Dispatcher must
	 * perform over the current task.
	 */
	/**@{*/
	#define KTASK_MANAGEMENT_USER0    TASK_MANAGEMENT_USER0    /**< Release the task on success.                   */
	#define KTASK_MANAGEMENT_USER1    TASK_MANAGEMENT_USER1    /**< Release the task on finalization.              */
	#define KTASK_MANAGEMENT_USER2    TASK_MANAGEMENT_USER2    /**< Complete the task without releasing semaphore. */
	#define KTASK_MANAGEMENT_AGAIN    TASK_MANAGEMENT_AGAIN    /**< Reschedule the task.                           */
	#define KTASK_MANAGEMENT_STOP     TASK_MANAGEMENT_STOP     /**< Move the task to stopped state.                */
	#define KTASK_MANAGEMENT_PERIODIC TASK_MANAGEMENT_PERIODIC /**< Periodic reschedule the task.                  */
	#define KTASK_MANAGEMENT_ERROR    TASK_MANAGEMENT_ERROR    /**< Release the task with error.                   */
	#define KTASK_MANAGEMENT_INVALID  TASK_MANAGEMENT_INVALID  /**< Invalid management.                            */

	#define KTASK_MANAGEMENT_DEFAULT  TASK_MANAGEMENT_DEFAULT  /**< Default management.                            */
	/**@}*/

	/**
	 * @name Default merge arguments's functions.
	 */
	/**@{*/
	#define KTASK_MERGE_ARGS_FN_REPLACE TASK_MERGE_ARGS_FN_REPLACE /**< Replace arguments. */
	#define KTASK_MERGE_ARGS_FN_DEFAULT TASK_MERGE_ARGS_FN_DEFAULT /**< Replace arguments. */
	/**@}*/

	/**
	 * @name Task Types.
	 */
	/**@{*/
	typedef struct task ktask_t;                                      /**< Task structure.                     */
	typedef int (* ktask_fn)(word_t, word_t, word_t, word_t, word_t); /**< Task function.                      */
	typedef void (* ktask_merge_args_fn)(                             /**< How pass arguments to a child task. */
		const word_t exit_args[KTASK_ARGS_NUM],
		word_t child_args[KTASK_ARGS_NUM]
	);
	/**@}*/

	/**
	 * @name Task Kernel Calls.
	 *
	 * @details Work in Progress.
	 */
	/**@{*/
	#define ktask_get_id(task)                            task_get_id(task)
	#define ktask_get_return(task)                        task_get_return(task)
	#define ktask_get_priority(task)                      task_get_priority(task)
	#define ktask_set_priority(task, priority)            task_set_priority(task, priority)
	#define ktask_get_number_parents(task)                task_get_number_parents(task)
	#define ktask_get_number_children(task)               task_get_number_children(task)
	#define ktask_get_child(task, id)                     task_get_child(task, id)
	#define ktask_get_period(task)                        task_get_return(task)
	#define ktask_set_period(task, period)                task_set_period(task, period)
	#define ktask_set_arguments(task, a0, a1, a2, a3, a4) task_set_arguments(task, a0, a1, a2, a3, a4)
	#define ktask_get_arguments(task)                     task_get_arguments(task)
	#define ktask_args_replace(exit, child)               task_args_replace(exit, child)

	extern ktask_t * ktask_current(void);

	extern int ktask_create(ktask_t *, ktask_fn, int, int, char);
	extern int ktask_unlink(ktask_t *);
	extern int ktask_connect(ktask_t *, ktask_t *, bool, bool, char);
	extern int ktask_disconnect(ktask_t *, ktask_t *);

	#define ktask_dispatch0(task)                     ktask_dispatch(task, 0,  0,   0,  0,  0)
	#define ktask_dispatch1(task, a0)                 ktask_dispatch(task, a0, 0,   0,  0,  0)
	#define ktask_dispatch2(task, a0, a1)             ktask_dispatch(task, a0, a1,  0,  0,  0)
	#define ktask_dispatch3(task, a0, a1, a2)         ktask_dispatch(task, a0, a1, a2,  0,  0)
	#define ktask_dispatch4(task, a0, a1, a2, a3)     ktask_dispatch(task, a0, a1, a2, a3,  0)
	#define ktask_dispatch5(task, a0, a1, a2, a3, a4) ktask_dispatch(task, a0, a1, a2, a3, a4)
	extern int ktask_dispatch(ktask_t *, word_t, word_t, word_t, word_t, word_t);

	#define ktask_emit0(task, coreid)                     ktask_emit(task, coreid,  0,  0,  0,  0,  0)
	#define ktask_emit1(task, coreid, a0)                 ktask_emit(task, coreid, a0,  0,  0,  0,  0)
	#define ktask_emit2(task, coreid, a0, a1)             ktask_emit(task, coreid, a0, a1,  0,  0,  0)
	#define ktask_emit3(task, coreid, a0, a1, a2)         ktask_emit(task, coreid, a0, a1, a2,  0,  0)
	#define ktask_emit4(task, coreid, a0, a1, a2, a4)     ktask_emit(task, coreid, a0, a1, a2, a3,  0)
	#define ktask_emit5(task, coreid, a0, a1, a2, a3, a4) ktask_emit(task, coreid, a0, a1, a2, a3, a4)
	extern int ktask_emit(ktask_t *, int, word_t, word_t, word_t, word_t, word_t);

	#define ktask_exit0(ret, mgt)                         ktask_exit(ret, mgt, NULL,  0, 0,   0,  0,  0)
	#define ktask_exit1(ret, mgt, fn, a0)                 ktask_exit(ret, mgt,   fn, a0, 0,   0,  0,  0)
	#define ktask_exit2(ret, mgt, fn, a0, a1)             ktask_exit(ret, mgt,   fn, a0, a1,  0,  0,  0)
	#define ktask_exit3(ret, mgt, fn, a0, a1, a2)         ktask_exit(ret, mgt,   fn, a0, a1, a2,  0,  0)
	#define ktask_exit4(ret, mgt, fn, a0, a1, a2)         ktask_exit(ret, mgt,   fn, a0, a1, a2, a3,  0)
	#define ktask_exit5(ret, mgt, fn, a0, a1, a2, a3, a4) ktask_exit(ret, mgt,   fn, a0, a1, a2, a3, a4)
	extern int ktask_exit(int, int, ktask_merge_args_fn, word_t, word_t, word_t, word_t, word_t);

	extern int ktask_wait(ktask_t *);
	extern int ktask_trywait(ktask_t *);
	extern int ktask_stop(ktask_t *);
	extern int ktask_continue(ktask_t *);
	extern int ktask_complete(ktask_t *, char);

	extern int ktask_mailbox_write(ktask_t *, ktask_t *);
	extern int ktask_mailbox_read(ktask_t *, ktask_t *);

	extern int ktask_portal_write(ktask_t *, ktask_t *);
	extern int ktask_portal_read(ktask_t *, ktask_t *, ktask_t *);
	/**@}*/

#endif /* NANVIX_SYS_TASK_H_ */

/**@}*/
