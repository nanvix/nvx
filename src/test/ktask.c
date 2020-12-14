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

/**
 * @brief Specific value for tests.
 */
#define TEST_TASK_SPECIFIC_VALUE 123

/**
 * @brief Dummy task.
 *
 * @param arg Unused argument.
 */
static void *task(void *arg)
{
	UNUSED(arg);

	return (NULL);
}

static int dummy(ktask_args_t * args)
{
	UNUSED(args);

	return (TASK_RET_SUCCESS);
}

static int task_value;
static int task_value2;

static int setter(ktask_args_t * args)
{
	task_value = (int) args->arg0;
	args->ret  = 0;

	return (TASK_RET_SUCCESS);
}

static spinlock_t master_lock;
static spinlock_t slave_lock;

static int current_fn(ktask_args_t * args)
{
	UNUSED(args);

	spinlock_unlock(&master_lock);
	spinlock_lock(&slave_lock);

	args->ret = 0;

	return (TASK_RET_SUCCESS);
}

static int parent_simple(ktask_args_t * args)
{
	UNUSED(args);

	task_value *= 5;
	args->ret   = 0;

	return (TASK_RET_SUCCESS);
}

static int child_simple(ktask_args_t * args)
{
	UNUSED(args);

	task_value /= 10;
	args->ret   =  0;

	return (TASK_RET_SUCCESS);
}

static int parent_children(ktask_args_t * args)
{
	UNUSED(args);

	task_value  *= 5;
	task_value2 /= 5;
	args->ret    = 0;

	return (TASK_RET_SUCCESS);
}

static int child0_children(ktask_args_t * args)
{
	UNUSED(args);

	task_value /= 10;
	args->ret   =  0;

	return (TASK_RET_SUCCESS);
}

static int child1_children(ktask_args_t * args)
{
	UNUSED(args);

	task_value2 *= 2;

	return (TASK_RET_SUCCESS);
}

static int parent0_parent(ktask_args_t * args)
{
	UNUSED(args);

	task_value *= 5;
	args->ret   = (0);

	return (TASK_RET_SUCCESS);
}

static int parent1_parent(ktask_args_t * args)
{
	UNUSED(args);

	task_value2 /= 5;
	args->ret    = 0;

	return (TASK_RET_SUCCESS);
}

static int child_parent(ktask_args_t * args)
{
	UNUSED(args);

	task_value  /= 10;
	task_value2 *=  2;
	args->ret    =  0;

	return (TASK_RET_SUCCESS);
}

static int periodic(ktask_args_t * args)
{
	if ((++task_value) < (int) args->arg0)
		return (TASK_RET_AGAIN);

	return (TASK_RET_SUCCESS);
}

/*============================================================================*
 * API Testing Units                                                          *
 *============================================================================*/

/**
 * @brief API test for task create.
 */
static void test_api_ktask_create(void)
{
	ktask_t t;

	t.args.arg0 = TEST_TASK_SPECIFIC_VALUE;

	test_assert(ktask_create(&t, dummy, &t.args, 0) == 0);

	/* Configuration. */
	test_assert(t.state == TASK_STATE_NOT_STARTED);
	test_assert(t.id    == TASK_NULL_ID);

	/* Arguments and returns. */
	test_assert(t.fn == dummy);
	test_assert((int) t.args.arg0 == TEST_TASK_SPECIFIC_VALUE);
	test_assert((int) t.args.ret == (-EINVAL));

	/* Dependency. */
	test_assert(t.parents == 0);
	test_assert(t.children.size == 0);

	test_assert(ktask_unlink(&t) == 0);
}

/**
 * @brief API test for task connect two tasks.
 */
static void test_api_ktask_connect(void)
{
	ktask_t t0, t1, t2, t3;

	test_assert(ktask_create(&t0, dummy, NULL, 0) == 0);
	test_assert(ktask_create(&t1, dummy, NULL, 0) == 0);
	test_assert(ktask_create(&t2, dummy, NULL, 0) == 0);
	test_assert(ktask_create(&t3, dummy, NULL, 0) == 0);

	test_assert(ktask_connect(&t1, &t2) == 0);

	/* Parent. */
	test_assert(t1.parents == 0);
	test_assert(t1.children.size == 1);

	/* Children. */
	test_assert(t2.parents == 1);
	test_assert(t2.children.size == 0);

	test_assert(ktask_connect(&t1, &t3) == 0);

	/* Parent. */
	test_assert(t1.parents == 0);
	test_assert(t1.children.size == 2);

	/* Children. */
	test_assert(t3.parents == 1);
	test_assert(t3.children.size == 0);

	test_assert(ktask_connect(&t0, &t1) == 0);
	test_assert(ktask_connect(&t0, &t3) == 0);

	/* Parent. */
	test_assert(t0.parents == 0);
	test_assert(t0.children.size == 2);

	test_assert(t1.parents == 1);
	test_assert(t1.children.size == 2);

	/* Children. */
	test_assert(t2.parents == 1);
	test_assert(t2.children.size == 0);

	test_assert(t3.parents == 2);
	test_assert(t3.children.size == 0);

	test_assert(ktask_disconnect(&t0, &t3) == 0);
	test_assert(ktask_disconnect(&t0, &t1) == 0);
	test_assert(ktask_disconnect(&t1, &t3) == 0);
	test_assert(ktask_disconnect(&t1, &t2) == 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
	test_assert(ktask_unlink(&t2) == 0);
	test_assert(ktask_unlink(&t3) == 0);
}

/**
 * @brief API test for dispatch a task.
 */
static void test_api_ktask_dispatch(void)
{
	ktask_t t;

	task_value  = 0ULL;
	t.args.arg0 = TEST_TASK_SPECIFIC_VALUE;

	test_assert(ktask_create(&t, setter, &t.args, 0) == 0);

	test_assert(ktask_dispatch(&t) == 0);
	test_assert(ktask_wait(&t) == 0);

	/* Complete the task. */
	test_assert(t.state    == TASK_STATE_COMPLETED);
	test_assert(t.id       != TASK_NULL_ID);
	test_assert(t.args.ret == 0);

	/* Check the value. */
	test_assert(task_value == TEST_TASK_SPECIFIC_VALUE);

	test_assert(ktask_unlink(&t) == 0);
}

/**
 * @brief API test for identificate a task.
 */
static void test_api_ktask_identification(void)
{
	ktask_t t;

	test_assert(ktask_create(&t, dummy, &t.args, 0) == 0);

	test_assert(ktask_get_id(&t) == TASK_NULL_ID);

		test_assert(ktask_dispatch(&t) == 0);
		test_assert(ktask_wait(&t) == 0);

	test_assert(ktask_get_id(&t) != TASK_NULL_ID);

	test_assert(ktask_unlink(&t) == 0);
}

/**
 * @brief API test for get the current task.
 */
static void test_api_ktask_current(void)
{
	ktask_t t;

	spinlock_init(&master_lock);
	spinlock_init(&slave_lock);

	spinlock_lock(&master_lock);
	spinlock_lock(&slave_lock);

	test_assert(ktask_create(&t, current_fn, &t.args, 0) == 0);

	test_assert(ktask_dispatch(&t) == 0);

		spinlock_lock(&master_lock);

			test_assert(ktask_current() == &t);

		spinlock_unlock(&slave_lock);

	test_assert(ktask_wait(&t) == 0);

	test_assert(ktask_unlink(&t) == 0);
}

/**
 * @brief API test for dispatch tasks with dependency.
 */
static void test_api_ktask_dependendy(void)
{
	ktask_t t0, t1;

	task_value = 20;

	test_assert(ktask_create(&t0, parent_simple, NULL, 0) == 0);
	test_assert(ktask_create(&t1, child_simple, NULL, 0) == 0);

	test_assert(ktask_connect(&t0, &t1) == 0);

	/* Parent. */
	test_assert(t0.parents == 0);
	test_assert(t0.children.size == 1);

	/* Children. */
	test_assert(t1.parents == 1);
	test_assert(t1.children.size == 0);

	test_assert(ktask_dispatch(&t1) == (-1));
	test_assert(ktask_dispatch(&t0) == 0);
	test_assert(ktask_wait(&t0) == 0);
	test_assert(ktask_wait(&t1) == 0);

	test_assert(task_value == 10);

	/* Parent. */
	test_assert(t0.parents == 0);
	test_assert(t0.children.size == 0);

	/* Children. */
	test_assert(t1.parents == 0);
	test_assert(t1.children.size == 0);

	test_assert(ktask_disconnect(&t0, &t1) < 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
}

/**
 * @brief API test for dispatch a task with multiple children.
 */
static void test_api_ktask_children(void)
{
	ktask_t t0, t1, t2;

	task_value  = 20;
	task_value2 = 20;

	test_assert(ktask_create(&t0, parent_children, NULL, 0) == 0);
	test_assert(ktask_create(&t1, child0_children, NULL, 0) == 0);
	test_assert(ktask_create(&t2, child1_children, NULL, 0) == 0);

	test_assert(ktask_connect(&t0, &t1) == 0);
	test_assert(ktask_connect(&t0, &t2) == 0);

	/* Parent. */
	test_assert(t0.parents == 0);
	test_assert(t0.children.size == 2);

	/* Children. */
	test_assert(t1.parents == 1);
	test_assert(t1.children.size == 0);
	test_assert(t2.parents == 1);
	test_assert(t2.children.size == 0);

	test_assert(ktask_dispatch(&t2) == (-1));
	test_assert(ktask_dispatch(&t1) == (-1));
	test_assert(ktask_dispatch(&t0) == 0);

	test_assert(ktask_wait(&t1) == 0);
	test_assert(task_value == 10);

	test_assert(ktask_wait(&t2) == 0);
	test_assert(task_value2 == 8);

	/* Parent. */
	test_assert(t0.parents == 0);
	test_assert(t0.children.size == 0);

	/* Children. */
	test_assert(t1.parents == 0);
	test_assert(t1.children.size == 0);
	test_assert(t2.parents == 0);
	test_assert(t2.children.size == 0);

	test_assert(ktask_disconnect(&t0, &t2) < 0);
	test_assert(ktask_disconnect(&t0, &t1) < 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
	test_assert(ktask_unlink(&t2) == 0);
}

/**
 * @brief API test for dispatch multiple parents with the same children.
 */
static void test_api_ktask_parent(void)
{
	ktask_t t0, t1, t2;

	task_value  = 20;
	task_value2 = 20;

	test_assert(ktask_create(&t0, parent0_parent, NULL, 0) == 0);
	test_assert(ktask_create(&t1, parent1_parent, NULL, 0) == 0);
	test_assert(ktask_create(&t2, child_parent, NULL, 0) == 0);

	test_assert(ktask_connect(&t0, &t2) == 0);
	test_assert(ktask_connect(&t1, &t2) == 0);

	/* Parent. */
	test_assert(t0.parents == 0);
	test_assert(t0.children.size == 1);
	test_assert(t1.parents == 0);
	test_assert(t1.children.size == 1);

	/* Children. */
	test_assert(t2.parents == 2);
	test_assert(t2.children.size == 0);

	test_assert(ktask_dispatch(&t0) == 0);
	test_assert(ktask_wait(&t0) == 0);

	test_assert(t0.parents == 0);
	test_assert(t0.children.size == 0);
	test_assert(t2.parents == 1);
	test_assert(t2.state == TASK_STATE_NOT_STARTED);

	test_assert(ktask_dispatch(&t1) == 0);
	test_assert(ktask_wait(&t1) == 0);

	test_assert(ktask_wait(&t2) == 0);
	test_assert(task_value == 10);
	test_assert(task_value2 == 8);

	test_assert(t1.parents == 0);
	test_assert(t1.children.size == 0);

	test_assert(ktask_disconnect(&t1, &t2) < 0);
	test_assert(ktask_disconnect(&t0, &t2) < 0);

	test_assert(ktask_unlink(&t0) == 0);
	test_assert(ktask_unlink(&t1) == 0);
	test_assert(ktask_unlink(&t2) == 0);
}

/**
 * @brief API test for dispatch a task.
 */
static void test_api_ktask_periodic(void)
{
	ktask_t t;

	task_value  = 0;
	t.args.arg0 = 10;

	test_assert(ktask_create(&t, periodic, &t.args, 10) == 0);

	test_assert(ktask_dispatch(&t) == 0);
	test_assert(ktask_wait(&t) == 0);

	/* Check the value. */
	test_assert(task_value == 10);

	test_assert(ktask_unlink(&t) == 0);
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
static struct test task_mgmt_tests_api[] = {
	{ test_api_ktask_create,         "[test][task][api] task create         [passed]" },
	{ test_api_ktask_connect,        "[test][task][api] task connect        [passed]" },
	{ test_api_ktask_dispatch,       "[test][task][api] task dispatch       [passed]" },
	{ test_api_ktask_identification, "[test][task][api] task identification [passed]" },
	{ test_api_ktask_current,        "[test][task][api] task current        [passed]" },
	{ test_api_ktask_dependendy,     "[test][task][api] task dependency     [passed]" },
	{ test_api_ktask_children,       "[test][task][api] task children       [passed]" },
	{ test_api_ktask_parent,         "[test][task][api] task parent         [passed]" },
	{ test_api_ktask_periodic,       "[test][task][api] task periodic       [passed]" },
	{ NULL,                           NULL                                            },
};

/**
 * @brief Fault tests.
 */
static struct test task_mgmt_tests_fault[] = {
	{ NULL,                           NULL                                            },
};

/**
 * @brief Stress tests.
 */
static struct test task_mgmt_tests_stress[] = {
	{ NULL,                           NULL                                            },
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

#else

#endif /* __NANVIX_USE_TASKS */
}
