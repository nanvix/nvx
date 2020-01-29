/*
 * MIT License
 *
 * Copyright(c) 2011-2019 The Maintainers of Nanvix
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


#include <nanvix/sys/excp.h>
#include "test.h"

#if (THREAD_MAX > 1)

extern int page_alloc(vaddr_t vaddr);

/**
 * @name Extra Tests
 */
/**@{*/
#define TEST_EXCP_HANDLE 1 /**< Test exception handling? */
/**@}*/

/**
 * @brief Bad address for exception handle test.
 */
#define BAD_ADDRESS (UBASE_VIRT)

/*============================================================================*
 * API Testing Units                                                          *
 *============================================================================*/

/**
 * @brief API test for controlling exceptions.
 */
static void test_api_excp_ctrl(void)
{
	/* Handle all exceptions; */
	for (int i = 0; i < EXCEPTIONS_NUM; i++)
		KASSERT(excp_ctrl(i, EXCP_ACTION_HANDLE) == 0);

	/* Ignore all exceptions. */
	for (int i = 0; i < EXCEPTIONS_NUM; i++)
		KASSERT(excp_ctrl(i, EXCP_ACTION_IGNORE) == 0);
}

#if TEST_EXCP_HANDLE

/**
 * @brief User-space exception handler.
 */
static void *handler(void *args)
{
	struct exception excp;

	UNUSED(args);

		KASSERT(excp_pause(&excp) == 0);

			KASSERT(exception_get_addr(&excp) == BAD_ADDRESS);

			KASSERT(page_alloc(BAD_ADDRESS) == 0);

		KASSERT(excp_resume() == 0);

	return (NULL);
}

/**
 * @brief API test for controlling exceptions.
 */
static void test_api_excp_handle(void)
{
	kthread_t tid;
	unsigned char *p = (unsigned char *) BAD_ADDRESS;

	KASSERT(excp_ctrl(EXCEPTION_PAGE_FAULT, EXCP_ACTION_HANDLE) == 0);

	KASSERT(kthread_create(&tid, &handler, NULL) == 0);

	*p = 0xff;

	test_assert(kthread_join(tid, NULL) == 0);
}

#endif

#endif

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

#if (THREAD_MAX > 1)

/**
 * @brief API tests.
 */
static struct test tests_api_excp_mgmt[] = {
	{ test_api_excp_ctrl,   "[test][excp][api] exception control [passed]" },
#if TEST_EXCP_HANDLE
	{ test_api_excp_handle, "[test][excp][api] exception handle  [passed]" },
#endif
	{ NULL,                  NULL                                          },
};

#endif

void test_excp_mgmt(void)
{
#if (THREAD_MAX > 1)
	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; tests_api_excp_mgmt[i].test_fn != NULL; i++)
	{
		tests_api_excp_mgmt[i].test_fn();
		nanvix_puts(tests_api_excp_mgmt[i].name);
	}
#endif
}
