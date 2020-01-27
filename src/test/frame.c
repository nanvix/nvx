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

#include <nanvix/sys/frame.h>
#include "test.h"

/**
 * @name Extra Tests
 */
/**@{*/
#define TEST_STRESS_FRAME_ALLOC 1 /**< Run stress test for frame allocation? */
/**@}*/

/**
 * @brief Workload for stress tests.
 */
#define NUM_FRAMES 8

/*============================================================================*
 * API Tests                                                                  *
 *============================================================================*/

/**
 * @brief API Test: Page Frame Allocation
 *
 * @author Pedro Henrique Penna
 */
PRIVATE void test_api_kframe_allocation(void)
{
	frame_t frame;

	KASSERT((frame = kframe_alloc()) != FRAME_NULL);
	KASSERT(kframe_free(frame) == 0);
}

/*============================================================================*
 * Fault Injection Tests                                                      *
 *============================================================================*/

/**
 * @brief Fault Injection Test: Invalid Page Frame Release
 *
 * @author Pedro Henrique Penna
 */
PRIVATE void test_fault_kframe_invalid_free(void)
{
	KASSERT(kframe_free(0) == -EINVAL);
	KASSERT(kframe_free((UBASE_VIRT + UMEM_SIZE) >> PAGE_SHIFT) == -EINVAL);
}

/**
 * @brief Fault Injection Test: Bad Page Frame Release
 *
 * @author Pedro Henrique Penna
 */
PRIVATE void test_fault_kframe_bad_free(void)
{
	KASSERT(kframe_free(frame_id_to_num(0)) == -EFAULT);
	KASSERT(kframe_free(frame_id_to_num(NUM_UFRAMES - 1)) == -EFAULT);
}

/**
 * @brief Fault Injection Test: Double Page Frame Release
 *
 * @author Pedro Henrique Penna
 */
PRIVATE void test_fault_kframe_double_free(void)
{
	frame_t frame;

	KASSERT((frame = kframe_alloc()) != FRAME_NULL);
	KASSERT(kframe_free(frame) == 0);
	KASSERT(kframe_free(frame) == -EFAULT);
}

/*============================================================================*
 * Stress Tests                                                               *
 *============================================================================*/

#if TEST_STRESS_FRAME_ALLOC

/**
 * @brief Stress Test: Page Frame Allocation
 *
 * @author Pedro Henrique Penna
 */
PRIVATE void test_stress_kframe_allocation(void)
{
	/* Allocate all frame frames. */
	for (frame_t  i = 0; i < NUM_FRAMES; i++)
		KASSERT(kframe_alloc() != FRAME_NULL);

	/* Release all frame frames. */
	for (frame_t i = 0; i < NUM_FRAMES; i++)
		KASSERT(kframe_free(frame_id_to_num(i)) == 0);
}

#endif

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
static struct test tests_api_kframe[] = {
	{ test_api_kframe_allocation, "[test][frame][api] frame allocation" },
	{ NULL,                        NULL                                 },
};

/**
 * @brief Fault injection tests.
 */
static struct test tests_fault_kframe[] = {
	{ test_fault_kframe_invalid_free, "[test][frame][fault] release invalid frame" },
	{ test_fault_kframe_bad_free,     "[test][frame][fault] release bad frame"     },
	{ test_fault_kframe_double_free,  "[test][frame][fault] release double frame"  },
	{ NULL,                            NULL                                        },
};

/**
 * @brief Stress tests.
 */
static struct test tests_stress_kframe[] = {
#if TEST_STRESS_FRAME_ALLOC
	{ test_stress_kframe_allocation, "[test][frame][stress] frame allocation" },
#endif
	{ NULL,                           NULL                                    },
};

/**
 * The test_kframe_mgmt() function launches testing units on the frameory
 * manager.
 *
 * @author Pedro Henrique Penna
 */
void test_kframe_mgmt(void)
{
	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; tests_api_kframe[i].test_fn != NULL; i++)
	{
		tests_api_kframe[i].test_fn();
		nanvix_puts(tests_api_kframe[i].name);
	}

	/* Fault Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; tests_fault_kframe[i].test_fn != NULL; i++)
	{
		tests_fault_kframe[i].test_fn();
		nanvix_puts(tests_fault_kframe[i].name);
	}

	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; tests_stress_kframe[i].test_fn != NULL; i++)
	{
		tests_stress_kframe[i].test_fn();
		nanvix_puts(tests_stress_kframe[i].name);
	}
}
