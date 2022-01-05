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


#include <nanvix/sys/task.h>
#include "test.h"

#if __NANVIX_USE_TASKS

/*============================================================================*
 * Global definition                                                          *
 *============================================================================*/

/**
 * @brief Specific value for tests.
 */
#define TEST_TASK_SPECIFIC_VALUE 0xc0ffee

/**
 * @brief Maximum lenght of a string.
 */
#define TEST_TASK_STRING_LENGTH_MAX 10

/**
 * @brief Convert to word_t.
 */
#define TOW(value) ((word_t) value)

/**
 * @name Auxiliar variables.
 */
/**@{*/
PRIVATE int len0;
PRIVATE char str0[TEST_TASK_STRING_LENGTH_MAX + 1];
/**@}*/

/**
 * @name Auxiliar synchronization.
 */
/**@{*/
PRIVATE spinlock_t master_lock;
PRIVATE spinlock_t slave_lock;
/**@}*/

/**
 * @brief Compare two strings.
 *
 * @param s0  First string.
 * @param s1  Second string.
 * @param len Length os the strings.
 */
PRIVATE bool test_strings_are_equals(const char * s0, const char * s1, int len)
{
	while (len--)
		if (s0[len] != s1[len])
			return (false);

	return (true);
}

/*============================================================================*
 * Test functions and variables.                                              *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * Dummy test.                                                                *
 *----------------------------------------------------------------------------*/

/**
 * @brief Dummy thread function.
 *
 * @param arg Unused argument.
 */
PRIVATE void * task(void *arg)
{
	UNUSED(arg);

	return (NULL);
}

/**
 * @brief Dummy task function.
 *
 * @param arg Unused argument.
 */
PRIVATE int dummy(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	UNUSED(arg0);
	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	return (0);
}

/*----------------------------------------------------------------------------*
 * Get current task test.                                                     *
 *----------------------------------------------------------------------------*/

PRIVATE int current_fn(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	UNUSED(arg0);
	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	spinlock_unlock(&master_lock);
	spinlock_lock(&slave_lock);

	return (0);
}

/*----------------------------------------------------------------------------*
 * String setter.                                                             *
 *----------------------------------------------------------------------------*/

/**
 * @brief Dummy task function.
 *
 * @param arg Unused argument.
 */
PRIVATE int str_setter(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	char * str;
	int * len;
	char chr;

	UNUSED(arg3);
	UNUSED(arg4);

	str = (char *) arg0;
	len = (int *)  arg1;
	chr = (char)   arg2;

	/* Does the current length exceeds maximum length? */
	if (*len >= TEST_TASK_STRING_LENGTH_MAX)
	{
		ktask_exit0(-1, TASK_MANAGEMENT_ERROR);
		return (-1);
	}

	/* Sets charecter. */
	str[(*len)++] = chr;

	return (TEST_TASK_SPECIFIC_VALUE);
}

/*----------------------------------------------------------------------------*
 * Periodic.                                                                  *
 *----------------------------------------------------------------------------*/

PRIVATE int periodic(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	char * str;
	int * len;
	char chr;

	UNUSED(arg3);
	UNUSED(arg4);

	str = (char *) arg0;
	len = (int *)  arg1;
	chr = (char)   arg2;

	/* Does the current length exceeds local maximum length? */
	if (*len < (int) TEST_TASK_STRING_LENGTH_MAX)
	{
		/* Sets charecter. */
		str[(*len)++] = chr;

		/**
		 * Reschedule periodic.
		 *
		 * Obs.: The current ktask_exit does not immediately return like
		 * thread_exit. It is used to set up management action after task
		 * conclusion or to pass arguments to the children tasks.
		 *
		 * TODO: Next implementation must immediately return.
		 */
		ktask_exit0(0, KTASK_MANAGEMENT_PERIODIC);
	}

	return (0);
}

/*----------------------------------------------------------------------------*
 * Loop.                                                                      *
 *----------------------------------------------------------------------------*/

PRIVATE void loop_merge_args(
	const word_t pargs[KTASK_ARGS_NUM],
	word_t nargs[KTASK_ARGS_NUM]
)
{
	nargs[0] = pargs[0];
	nargs[1] = pargs[1];

	/* Ignores args[2] because it contains individual character value. */
}

PRIVATE int loop(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	char * str;
	char chr;
	int pos;

	UNUSED(arg3);
	UNUSED(arg4);

	str = (char *) arg0;
	pos = (int)    arg1;
	chr = (char)   arg2;

	/* Is the position already written? */
	if (*str != '\0')
	{
		/* Move to the next position. */
		str++;
		pos++;
	}

	/* Does the current length exceeds local maximum length? */
	if (pos < (int) TEST_TASK_STRING_LENGTH_MAX)
	{
		/* Sets charecter. */
		*str = chr;

		/**
		 * Update the arguments of the same task and continue the loop without
		 * releasing the semaphore (args -> args + 1).
		 */
		ktask_exit2(0,
			KTASK_MANAGEMENT_USER1,
			loop_merge_args,
			(word_t) (str + 1),
			(word_t) (pos + 1)
		);
	}

	/* Finish the loop without error. */
	else
		ktask_exit0(0, KTASK_MANAGEMENT_USER0);

	return (0);
}

/*----------------------------------------------------------------------------*
 * Emission.                                                                  *
 *----------------------------------------------------------------------------*/

PRIVATE int emission(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int coreid = core_get_id();

	UNUSED(arg3);
	UNUSED(arg4);

	test_assert(((int) arg0) == coreid);
	test_assert(arg1 == 1);
	test_assert(arg2 == 2);

	return (coreid);
}

/*============================================================================*
 * API Testing Units                                                          *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * test_api_ktask_create()                                                    *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for task create.
 */
PRIVATE void test_api_ktask_create(void)
{
	ktask_t t;

	test_assert(ktask_create(&t, dummy, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

		/* Internal values. */
		test_assert(t.fn == dummy);
		test_assert(t.state == TASK_STATE_NOT_STARTED);
		test_assert(t.schedule_type == TASK_SCHEDULE_READY);

		/* External values. */
		test_assert(ktask_get_id(&t) != TASK_NULL_ID);
		test_assert(ktask_get_number_parents(&t) == 0);
		test_assert(ktask_get_number_children(&t) == 0);

	test_assert(ktask_unlink(&t) == 0);

	test_assert(ktask_get_id(&t) == TASK_NULL_ID);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_connect()                                                   *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for task connect two tasks.
 */
PRIVATE void test_api_ktask_connect(void)
{
	ktask_t t0, t1, t2, t3;

	test_assert(ktask_create(&t0, dummy, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t1, dummy, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t2, dummy, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t3, dummy, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_connect(&t1, &t2, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_DEFAULT) == 0);

		/* Parent t1. */
		test_assert(ktask_get_number_parents(&t1) == 0);
		test_assert(ktask_get_number_children(&t1) == 1);

		/* Child t2. */
		test_assert(ktask_get_number_parents(&t2) == 1);
		test_assert(ktask_get_number_children(&t2) == 0);

	test_assert(ktask_connect(&t1, &t3, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_DEFAULT) == 0);

		/* Parent t1. */
		test_assert(ktask_get_number_parents(&t1) == 0);
		test_assert(ktask_get_number_children(&t1) == 2);

		/* Child t2. */
		test_assert(ktask_get_number_parents(&t2) == 1);
		test_assert(ktask_get_number_children(&t2) == 0);

		/* Child t3. */
		test_assert(ktask_get_number_parents(&t3) == 1);
		test_assert(ktask_get_number_children(&t3) == 0);

	test_assert(ktask_connect(&t0, &t1, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_DEFAULT) == 0);
	test_assert(ktask_connect(&t0, &t3, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_DEFAULT) == 0);

		/* Parent t0. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 2);

		/* Parent and Child t1. */
		test_assert(ktask_get_number_parents(&t1) == 1);
		test_assert(ktask_get_number_children(&t1) == 2);

		/* Child t2. */
		test_assert(ktask_get_number_parents(&t2) == 1);
		test_assert(ktask_get_number_children(&t2) == 0);

		/* Child t3. */
		test_assert(ktask_get_number_parents(&t3) == 2);
		test_assert(ktask_get_number_children(&t3) == 0);

	test_assert(ktask_disconnect(&t0, &t3) == 0);
	test_assert(ktask_disconnect(&t0, &t1) == 0);
	test_assert(ktask_disconnect(&t1, &t3) == 0);
	test_assert(ktask_disconnect(&t1, &t2) == 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
	test_assert(ktask_unlink(&t2) == 0);
	test_assert(ktask_unlink(&t3) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_dispatch()                                                  *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch a task.
 */
PRIVATE void test_api_ktask_dispatch(void)
{
	ktask_t t;

	len0 = 0;
	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_dispatch3(&t, TOW(str0), TOW(&len0), TOW('s')) == 0);
	test_assert(ktask_wait(&t) == TEST_TASK_SPECIFIC_VALUE);

		/* Internal values. */
		test_assert(t.state   == TASK_STATE_COMPLETED);
		test_assert(t.args[0] == TOW(str0));
		test_assert(t.args[1] == TOW(&len0));
		test_assert(t.args[2] == TOW('s'));

		/* External values. */
		test_assert(ktask_get_id(&t)     != TASK_NULL_ID);
		test_assert(ktask_get_return(&t) == TEST_TASK_SPECIFIC_VALUE);

		/* Check the value. */
		test_assert(len0 == 1);
		test_assert(test_strings_are_equals(str0, "s\0", 2));

	test_assert(ktask_unlink(&t) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_identification()                                            *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for identificate a task.
 */
PRIVATE void test_api_ktask_identification(void)
{
	int tid;
	ktask_t t;

	test_assert(ktask_create(&t, dummy, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert((tid = ktask_get_id(&t)) != TASK_NULL_ID);

		test_assert(ktask_dispatch0(&t) == 0);
		test_assert(ktask_wait(&t) == 0);

	test_assert(ktask_get_id(&t) == tid);

	test_assert(ktask_unlink(&t) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_current()                                                   *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for get the current task.
 */
PRIVATE void test_api_ktask_current(void)
{
	ktask_t t;

	spinlock_init(&master_lock);
	spinlock_init(&slave_lock);

	spinlock_lock(&master_lock);
	spinlock_lock(&slave_lock);

	test_assert(ktask_create(&t, current_fn, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_dispatch0(&t) == 0);

		spinlock_lock(&master_lock);

			test_assert(ktask_current() == &t);

		spinlock_unlock(&slave_lock);

	test_assert(ktask_wait(&t) == 0);

	test_assert(ktask_unlink(&t) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_hard_dependendy()                                           *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch tasks with hard dependency.
 */
PRIVATE void test_api_ktask_hard_dependendy(void)
{
	ktask_t t0, t1;

	len0 = 0;
	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t0, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t1, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_connect(&t0, &t1, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_DEFAULT) == 0);

		/* Parent. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 1);

		/* Children. */
		test_assert(ktask_get_number_parents(&t1) == 1);
		test_assert(ktask_get_number_children(&t1) == 0);

	ktask_set_arguments(&t1, TOW(str0), TOW(&len0), TOW('2'), 0, 0);

	test_assert(ktask_dispatch3(&t0, TOW(str0), TOW(&len0), TOW('1')) == 0);

	test_assert(ktask_wait(&t0) == TEST_TASK_SPECIFIC_VALUE);
	test_assert(ktask_wait(&t1) == TEST_TASK_SPECIFIC_VALUE);

		/* Parent. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 1);

		/* Children. */
		test_assert(ktask_get_number_parents(&t1) == 1);
		test_assert(ktask_get_number_children(&t1) == 0);

		/* Check the value. */
		test_assert(len0 == 2);
		test_assert(test_strings_are_equals(str0, "12\0", 3));

	test_assert(ktask_disconnect(&t0, &t1) == 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_soft_dependendy()                                           *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch tasks with soft dependency.
 */
PRIVATE void test_api_ktask_soft_dependendy(void)
{
	ktask_t t0, t1;

	len0 = 0;
	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t0, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t1, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_connect(&t0, &t1, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_TEMPORARY, KTASK_TRIGGER_DEFAULT) == 0);

		/* Parent. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 1);

		/* Children. */
		test_assert(ktask_get_number_parents(&t1) == 1);
		test_assert(ktask_get_number_children(&t1) == 0);

	ktask_set_arguments(&t1, TOW(str0), TOW(&len0), TOW('2'), 0, 0);

	test_assert(ktask_dispatch3(&t0, TOW(str0), TOW(&len0), TOW('1')) == 0);
	test_assert(ktask_wait(&t0) == TEST_TASK_SPECIFIC_VALUE);
	test_assert(ktask_wait(&t1) == TEST_TASK_SPECIFIC_VALUE);

		/* Parent. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 0);

		/* Children. */
		test_assert(ktask_get_number_parents(&t1) == 0);
		test_assert(ktask_get_number_children(&t1) == 0);

		/* Check the value. */
		test_assert(len0 == 2);
		test_assert(test_strings_are_equals(str0, "12\0", 3));

	test_assert(ktask_disconnect(&t0, &t1) != 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_children()                                                  *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch a task with multiple children.
 */
PRIVATE void test_api_ktask_children(void)
{
	ktask_t t0, t1, t2;

	len0 = 0;
	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t0, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t1, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t2, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_connect(&t0, &t1, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_DEFAULT) == 0);
	test_assert(ktask_connect(&t0, &t2, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_DEFAULT) == 0);

		/* Parent. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 2);

		/* Children. */
		test_assert(ktask_get_number_parents(&t1) == 1);
		test_assert(ktask_get_number_children(&t1) == 0);
		test_assert(ktask_get_number_parents(&t2) == 1);
		test_assert(ktask_get_number_children(&t2) == 0);

	ktask_set_arguments(&t2, TOW(str0), TOW(&len0), TOW('2'), 0, 0);
	ktask_set_arguments(&t1, TOW(str0), TOW(&len0), TOW('1'), 0, 0);

	test_assert(ktask_dispatch3(&t0, TOW(str0), TOW(&len0), TOW('0')) == 0);

	test_assert(ktask_wait(&t1) == TEST_TASK_SPECIFIC_VALUE);
	test_assert(ktask_wait(&t2) == TEST_TASK_SPECIFIC_VALUE);

		/* Check the value. */
		test_assert(len0 == 3);
		test_assert(
			test_strings_are_equals(str0, "012\0", 4) ||
			test_strings_are_equals(str0, "021\0", 4)
		);

	test_assert(ktask_disconnect(&t0, &t2) == 0);
	test_assert(ktask_disconnect(&t0, &t1) == 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
	test_assert(ktask_unlink(&t2) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_parent()                                                    *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch multiple parents with the same children.
 */
PRIVATE void test_api_ktask_parent(void)
{
	ktask_t t0, t1, t2;

	len0 = 0;
	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t0, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t1, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t2, str_setter, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_connect(&t0, &t2, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_TEMPORARY, KTASK_TRIGGER_DEFAULT) == 0);
	test_assert(ktask_connect(&t1, &t2, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_DEFAULT) == 0);

		/* Parent. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 1);
		test_assert(ktask_get_number_parents(&t1) == 0);
		test_assert(ktask_get_number_children(&t1) == 1);

		/* Children. */
		test_assert(ktask_get_number_parents(&t2) == 2);
		test_assert(ktask_get_number_children(&t2) == 0);

	ktask_set_arguments(&t2, TOW(str0), TOW(&len0), TOW('2'), 0, 0);

	test_assert(ktask_dispatch3(&t0, TOW(str0), TOW(&len0), TOW('0')) == 0);
	test_assert(ktask_wait(&t0) == TEST_TASK_SPECIFIC_VALUE);

		/* Current task states. */
		test_assert(t0.state == TASK_STATE_COMPLETED);
		test_assert(t1.state == TASK_STATE_NOT_STARTED);
		test_assert(t2.state == TASK_STATE_NOT_STARTED);

		/* Parent. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 0);
		test_assert(ktask_get_number_parents(&t1) == 0);
		test_assert(ktask_get_number_children(&t1) == 1);

		/* Children. */
		test_assert(ktask_get_number_parents(&t2) == 1);
		test_assert(ktask_get_number_children(&t2) == 0);

		/* Check the value. */
		test_assert(len0 == 1);
		test_assert(test_strings_are_equals(str0, "0\0", 2));

	test_assert(ktask_dispatch3(&t1, TOW(str0), TOW(&len0), TOW('1')) == 0);
	test_assert(ktask_wait(&t1) == TEST_TASK_SPECIFIC_VALUE);
	test_assert(ktask_wait(&t2) == TEST_TASK_SPECIFIC_VALUE);

		/* Parent. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 0);
		test_assert(ktask_get_number_parents(&t1) == 0);
		test_assert(ktask_get_number_children(&t1) == 1);

		/* Children. */
		test_assert(ktask_get_number_parents(&t2) == 1);
		test_assert(ktask_get_number_children(&t2) == 0);

		/* Check values. */
		test_assert(len0 == 3);
		test_assert(test_strings_are_equals(str0, "012\0", 4));

	test_assert(ktask_disconnect(&t0, &t2) < 0);
	test_assert(ktask_disconnect(&t1, &t2) == 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
	test_assert(ktask_unlink(&t2) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_exit_periodic()                                             *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch a task.
 */
PRIVATE void test_api_ktask_exit_periodic(void)
{
	ktask_t t;

	len0 = 0;
	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t, periodic, KTASK_PRIORITY_LOW, 10, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_dispatch3(&t, TOW(str0), TOW(&len0), TOW('t')) == 0);
	test_assert(ktask_wait(&t) == 0);

		/* Check values. */
		test_assert(len0 == TEST_TASK_STRING_LENGTH_MAX);
		test_assert(test_strings_are_equals(str0, "tttttttttt\0", TEST_TASK_STRING_LENGTH_MAX + 1));

	test_assert(ktask_unlink(&t) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_loop_periodic()                                             *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch a task.
 */
PRIVATE void test_api_ktask_loop_periodic(void)
{
	ktask_t t;

	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t, loop, KTASK_PRIORITY_LOW, 10, KTASK_MANAGEMENT_USER0) == 0);
	test_assert(ktask_connect(&t, &t, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER1) == 0);

	test_assert(ktask_dispatch3(&t, TOW(str0), 0, TOW('t')) == 0);

	test_assert(ktask_wait(&t) == 0);

		/* Check values. */
		test_assert(test_strings_are_equals(str0, "tttttttttt\0", TEST_TASK_STRING_LENGTH_MAX + 1));

		/* Check state. */
		test_assert(t.state == TASK_STATE_COMPLETED);

	test_assert(ktask_disconnect(&t, &t) == 0);
	test_assert(ktask_unlink(&t) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_loop_complex()                                              *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch a task.
 */
PRIVATE void test_api_ktask_loop_complex(void)
{
	ktask_t t0, t1, t2;

	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t0, loop, KTASK_PRIORITY_LOW, 10, KTASK_MANAGEMENT_USER0) == 0);
	test_assert(ktask_create(&t1, loop, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_USER0) == 0);
	test_assert(ktask_create(&t2, loop, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_USER0) == 0);

	/**
	 * Loop: { - HARD } != { .. SOFT }
	 *
	 * +----------------+
	 * 'v               |
	 * t0 ---+---> t1 -+
	 * .^    :
	 * :     +..> t2 ..+
	 * +...............+
	 *
	 */
	test_assert(ktask_connect(&t0, &t1, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER1) == 0);
	test_assert(ktask_connect(&t1, &t0, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER1) == 0);
	test_assert(ktask_connect(&t0, &t2, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_TEMPORARY, KTASK_TRIGGER_USER1) == 0);
	test_assert(ktask_connect(&t2, &t0, KTASK_CONN_IS_DEPENDENCY, KTASK_CONN_IS_TEMPORARY, KTASK_TRIGGER_USER1) == 0);

	ktask_set_arguments(&t1, 0, 0, TOW('1'), 0, 0);
	ktask_set_arguments(&t2, 0, 0, TOW('2'), 0, 0);

	test_assert(ktask_dispatch3(&t0, TOW(str0), 0, TOW('0')) == 0);

	test_assert(ktask_wait(&t1) == 0);

		/* Check values. */
		test_assert(test_strings_are_equals(str0, "0120101010\0", TEST_TASK_STRING_LENGTH_MAX + 1));

		/* Check state. */
		test_assert(t0.state == TASK_STATE_COMPLETED);
		test_assert(t1.state == TASK_STATE_COMPLETED);
		test_assert(t2.state == TASK_STATE_COMPLETED);

	test_assert(ktask_disconnect(&t0, &t1) == 0);
	test_assert(ktask_disconnect(&t1, &t0) == 0);
	test_assert(ktask_disconnect(&t0, &t2) < 0);
	test_assert(ktask_disconnect(&t2, &t0) < 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
	test_assert(ktask_unlink(&t2) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_flow()                                                      *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch a flow type graph.
 */
PRIVATE void test_api_ktask_flow(void)
{
	ktask_t t0, t1, t2;

	kmemset(str0, '\0', TEST_TASK_STRING_LENGTH_MAX + 1);

	test_assert(ktask_create(&t0, dummy, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t1, loop, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_create(&t2, dummy, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_connect(&t0, &t1, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER0) == 0);
	test_assert(ktask_connect(&t1, &t1, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER1) == 0);
	test_assert(ktask_connect(&t1, &t2, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER0) == 0);

		/* Parent and children. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 1);
		test_assert(ktask_get_number_parents(&t1) == 2);
		test_assert(ktask_get_number_children(&t1) == 2);
		test_assert(ktask_get_number_parents(&t2) == 1);
		test_assert(ktask_get_number_children(&t2) == 0);

	ktask_set_arguments(&t1, TOW(str0), 0, TOW('t'), 0, 0);

	test_assert(ktask_dispatch0(&t0) == 0);
	test_assert(ktask_wait(&t2) == 0);

		/* Current task states. */
		test_assert(t0.state == TASK_STATE_COMPLETED);
		test_assert(t1.state == TASK_STATE_COMPLETED);
		test_assert(t2.state == TASK_STATE_COMPLETED);

		/* Parent and children. */
		test_assert(ktask_get_number_parents(&t0) == 0);
		test_assert(ktask_get_number_children(&t0) == 1);
		test_assert(ktask_get_number_parents(&t1) == 2);
		test_assert(ktask_get_number_children(&t1) == 2);
		test_assert(ktask_get_number_parents(&t2) == 1);
		test_assert(ktask_get_number_children(&t2) == 0);

		/* Check the value. */
		test_assert(test_strings_are_equals(str0, "tttttttttt\0", TEST_TASK_STRING_LENGTH_MAX + 1));

	test_assert(ktask_disconnect(&t0, &t1) == 0);
	test_assert(ktask_disconnect(&t1, &t1) == 0);
	test_assert(ktask_disconnect(&t1, &t2) == 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
	test_assert(ktask_unlink(&t2) == 0);
}

/*----------------------------------------------------------------------------*
 * test_api_ktask_emit()                                                      *
 *----------------------------------------------------------------------------*/

/**
 * @brief API test for dispatch a task.
 */
PRIVATE void test_api_ktask_emit(void)
{
	ktask_t t;

	/* Create the task. */
	test_assert(ktask_create(&t, emission, KTASK_PRIORITY_LOW, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	/* Emit the task. */
	for (int coreid = 0; coreid < CORES_NUM; ++coreid)
	{
		test_assert(ktask_emit3(&t, coreid, coreid, 1, 2) == 0);
		test_assert(ktask_wait(&t) == coreid);
	}

	/* Unlink the task. */
	test_assert(ktask_unlink(&t) == 0);
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
PRIVATE struct test task_mgmt_tests_api[] = {
	{ test_api_ktask_create,          "[test][task][api] task create          [passed]" },
	{ test_api_ktask_connect,         "[test][task][api] task connect         [passed]" },
	{ test_api_ktask_dispatch,        "[test][task][api] task dispatch        [passed]" },
	{ test_api_ktask_identification,  "[test][task][api] task identification  [passed]" },
	{ test_api_ktask_current,         "[test][task][api] task current         [passed]" },
	{ test_api_ktask_hard_dependendy, "[test][task][api] task hard dependency [passed]" },
	{ test_api_ktask_soft_dependendy, "[test][task][api] task soft dependency [passed]" },
	{ test_api_ktask_children,        "[test][task][api] task children        [passed]" },
	{ test_api_ktask_parent,          "[test][task][api] task parent          [passed]" },
	{ test_api_ktask_exit_periodic,   "[test][task][api] task exit periodic   [passed]" },
	{ test_api_ktask_loop_periodic,   "[test][task][api] task loop periodic   [passed]" },
	{ test_api_ktask_loop_complex,    "[test][task][api] task loop complex    [passed]" },
	{ test_api_ktask_flow,            "[test][task][api] task flow            [passed]" },
	{ test_api_ktask_emit,            "[test][task][api] task emit            [passed]" },
	{ NULL,                            NULL                                             },
};

/**
 * @brief Fault tests.
 */
PRIVATE struct test task_mgmt_tests_fault[] = {
	{ NULL,                            NULL                                             },
};

/**
 * @brief Stress tests.
 */
PRIVATE struct test task_mgmt_tests_stress[] = {
	{ NULL,                            NULL                                             },
};

#endif /* __NANVIX_USE_TASKS */

/**
 * The test_task_mgmt() function launches testing units on task manager.
 *
 * @author João Vicente Souto
 */
void test_task_mgmt(void)
{
#if __NANVIX_USE_TASKS

	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; task_mgmt_tests_api[i].test_fn != NULL; i++)
	{
		task_mgmt_tests_api[i].test_fn();
		nanvix_puts(task_mgmt_tests_api[i].name);
	}

	/* Fault Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; task_mgmt_tests_fault[i].test_fn != NULL; i++)
	{
		task_mgmt_tests_fault[i].test_fn();
		nanvix_puts(task_mgmt_tests_fault[i].name);
	}

	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; task_mgmt_tests_stress[i].test_fn != NULL; i++)
	{
		task_mgmt_tests_stress[i].test_fn();
		nanvix_puts(task_mgmt_tests_stress[i].name);
	}

#endif /* __NANVIX_USE_TASKS */
}

