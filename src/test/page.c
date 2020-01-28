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

#include <nanvix/sys/page.h>
#include "test.h"

/**
 * @name Extra Tests
 */
/**@{*/
#define TEST_PAGE_STRESS_ALLOC 1 /**< Run stress test for page allocation? */
#define TEST_PAGE_STRESS_WRITE 1 /**< Run stress test for page write?      */
/**@}*/

/**
 * @brief Unmapped address.
 */
#define BAD_ADDRESS 0xdeadbeef

/**
 * @brief Workload for stress tests.
 */
#define NUM_PAGES 8

/*============================================================================*
 * API Tests                                                                  *
 *============================================================================*/

/**
 * @brief API Test: User Page Allocation
 *
 * @author Pedro Henrique Penna
 */
static void test_api_page_allocation(void)
{
	KASSERT(page_alloc(UBASE_VIRT) == 0);
	KASSERT(page_free(UBASE_VIRT) == 0);
}

/**
 * @brief API Test: User Page Write
 *
 * @author Pedro Henrique Penna
 */
static void test_api_page_write(void)
{
	unsigned *pg;
	const unsigned magic = BAD_ADDRESS;

	pg = (unsigned  *)(UBASE_VIRT);

	/* Allocate.*/
	KASSERT(page_alloc(VADDR(pg)) == 0);

	/* Write. */
	for (size_t i = 0; i < PAGE_SIZE/sizeof(unsigned); i++)
		pg[i] = magic;
	for (size_t i = 0; i < PAGE_SIZE/sizeof(unsigned); i++)
		KASSERT(pg[i] == magic);

	/* Unmap. */
	KASSERT(page_free(VADDR(pg)) == 0);
}

/*============================================================================*
 * Fault Injection Tests                                                      *
 *============================================================================*/

/**
 * @brief Fault Injection Test: Invalid User Page Allocation
 *
 * @author Pedro Henrique Penna
 */
static void test_fault_page_invalid_allocation(void)
{
	KASSERT(page_alloc(KBASE_VIRT) == -EFAULT);
	KASSERT(page_alloc(UBASE_VIRT - PAGE_SIZE) == -EFAULT);
	KASSERT(page_alloc(UBASE_VIRT + PAGE_SIZE - 1) == -EINVAL);
}

/**
 * @brief Fault Injection Test: Double User Allocation Test
 *
 * @author Pedro Henrique Penna
 */
static void test_fault_page_double_allocation(void)
{
	KASSERT(page_alloc(UBASE_VIRT) == 0);
	KASSERT(page_alloc(UBASE_VIRT) == -EADDRINUSE);
	KASSERT(page_free(UBASE_VIRT) == 0);
}

/**
 * @brief Fault Injection Test: Invalid User Page Release
 *
 * @author Pedro Henrique Penna
 */
static void test_fault_page_invalid_free(void)
{
	KASSERT(page_free(KBASE_VIRT) == -EFAULT);
	KASSERT(page_free(UBASE_VIRT - PAGE_SIZE) == -EFAULT);
	KASSERT(page_free(UBASE_VIRT + PAGE_SIZE - 1) == -EINVAL);
}

/**
 * @brief Fault Injection Test: Bad User Page Release
 *
 * @author Pedro Henrique Penna
 */
static void test_fault_page_bad_free(void)
{
	KASSERT(page_free(UBASE_VIRT) == -EFAULT);
}

/**
 * @brief Fault Injection Test: Double User Page Release
 *
 * @author Pedro Henrique Penna
 */
static void test_fault_page_double_free(void)
{
	KASSERT(page_alloc(UBASE_VIRT) == 0);
	KASSERT(page_free(UBASE_VIRT) == 0);
	KASSERT(page_free(UBASE_VIRT) == -EFAULT);
}

/*============================================================================*
 * Stress Tests                                                               *
 *============================================================================*/

#if TEST_PAGE_STRESS_ALLOC

/**
 * @brief Stress Test: User Page Allocation
 *
 * @author Pedro Henrique Penna
 */
static void test_stress_page_allocation(void)
{
	/* Allocate pages. */
	for (unsigned i = 0; i < NUM_PAGES; i++)
		KASSERT(page_alloc(UBASE_VIRT + i*PAGE_SIZE) == 0);

	/* Release pages. */
	for (unsigned i = 0; i < NUM_PAGES; i++)
		KASSERT(page_free(UBASE_VIRT + i*PAGE_SIZE) == 0);
}

#endif

#if TEST_PAGE_STRESS_WRITE

/**
 * @brief Stress Test: User Page Write
 *
 * @author Pedro Henrique Penna
 */
static void test_stress_page_write(void)
{
	unsigned *pg;
	const unsigned magic = BAD_ADDRESS;

	/* Allocate pages. */
	for (unsigned i = 0; i < NUM_PAGES; i++)
	{
		pg = (void *)(UBASE_VIRT + i*PAGE_SIZE);

		KASSERT(page_alloc(VADDR(pg)) == 0);

		/* Write to kernel page. */
		for (unsigned j = 0; j < PAGE_SIZE/sizeof(unsigned); j++)
			pg[j] = magic;
	}

	/* Release pages. */
	for (unsigned i = 0; i < NUM_PAGES; i++)
	{
		pg = (void *)(UBASE_VIRT + i*PAGE_SIZE);

		/* Sanity check. */
		for (unsigned j = 0; j < PAGE_SIZE/sizeof(unsigned); j++)
			KASSERT(pg[j] == magic);

		KASSERT(page_free(VADDR(pg)) == 0);
	}
}

#endif


/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
static struct test tests_api_page[] = {
	{ test_api_page_allocation, "[test][page][api] page allocation" },
	{ test_api_page_write,      "[test][page][api] page write"      },
	{ NULL,                     NULL                                },
};

/**
 * @brief Fault injection tests.
 */
static struct test tests_fault_page[] = {
	{ test_fault_page_invalid_allocation, "[test][page][fault] allocate invalid page" },
	{ test_fault_page_double_allocation,  "[test][page][fault] allocate page twice"   },
	{ test_fault_page_invalid_free,       "[test][page][fault] release invalid page"  },
	{ test_fault_page_bad_free,           "[test][page][fault] release bad page"      },
	{ NULL,                                NULL                                       },
};

/**
 * @brief Stress tests.
 */
static struct test tests_stress_page[] = {
#if TEST_PAGE_STRESS_ALLOC
	{ test_stress_page_allocation,          "[test][page][stress] page allocation" },
#endif
#if TEST_PAGE_STRESS_WRITE
	{ test_stress_page_write,               "[test][page][stress] page write"      },
#endif
	{ NULL,                                  NULL                                  },
};

/**
 * The test_page_mgmt() function launches testing units on the pageory
 * manager.
 *
 * @author Pedro Henrique Penna
 */
void test_page_mgmt(void)
{
	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; tests_api_page[i].test_fn != NULL; i++)
	{
		tests_api_page[i].test_fn();
		nanvix_puts(tests_api_page[i].name);
	}

	/* Fault Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; tests_fault_page[i].test_fn != NULL; i++)
	{
		tests_fault_page[i].test_fn();
		nanvix_puts(tests_fault_page[i].name);
	}

	/* Stress Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; tests_stress_page[i].test_fn != NULL; i++)
	{
		tests_stress_page[i].test_fn();
		nanvix_puts(tests_stress_page[i].name);
	}
}
