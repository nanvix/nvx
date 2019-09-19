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

#include <nanvix/sys/noc.h>
#include "test.h"

/**
 * @brief Launch verbose tests?
 */
#define TEST_NOC_VERBOSE 0

/*============================================================================*
 * API Tests                                                                  *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * Query Logical NoC Node Number                                              *
 *----------------------------------------------------------------------------*/

/**
 * @brief API Test: Query Logical NoC Node Number
 */
PRIVATE void test_node_get_num(void)
{
	int nodenum;

	nodenum = knode_get_num();

#if (TEST_NOC_VERBOSE)
	kprintf("[test][processor][node][api] noc node %d online", nodenum);
#endif

	KASSERT(nodenum == PROCESSOR_NODENUM_MASTER);
}

/*----------------------------------------------------------------------------*
 * Exchange Logical NoC Node Number                                           *
 *----------------------------------------------------------------------------*/

/**
 * @brief API Test: Exchange Logical NoC Node Number
 */
PRIVATE void test_node_set_num(void)
{
	int nodenum;

	nodenum = knode_get_num();
	KASSERT(nodenum == PROCESSOR_NODENUM_MASTER);

#if (TEST_NOC_VERBOSE)
    kprintf("[test][processor][node][api] noc node %d online", nodenum);
#endif

        /* New nodenum (1 % (Interfaces available in only one IO Cluster)) */
        nodenum += (1 % (PROCESSOR_NOC_IONODES_NUM / PROCESSOR_IOCLUSTERS_NUM));

        #if (TEST_NOC_VERBOSE)
            kprintf("[test][processor][node][api] exchange noc node number to %d", nodenum);
        #endif

        KASSERT(knode_set_num(nodenum) == 0);
        KASSERT(knode_get_num() == nodenum);

        /* Restoure old nodenum. */
        nodenum -= (1 % (PROCESSOR_NOC_IONODES_NUM / PROCESSOR_IOCLUSTERS_NUM));

        KASSERT(knode_set_num(nodenum) == 0);

	nodenum = knode_get_num();
	KASSERT(nodenum == PROCESSOR_NODENUM_MASTER);
}

/**
 * @brief API Tests.
 */
PRIVATE struct test test_api_noc[] = {
	{ test_node_get_num,  "[test][processor][node][api] get logical noc node num [passed]\n" },
	{ test_node_set_num,  "[test][processor][node][api] set logical noc node num [passed]\n" },
	{ NULL,                NULL                                                              },
};

/*============================================================================*
 * FAULT Tests                                                                *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * Exchange Logical NoC Node Number with invalid arguments                    *
 *----------------------------------------------------------------------------*/

/**
 * @brief FAULT Test: Invalid Set Logical NoC Node Number
 */
PRIVATE void test_node_invalid_set_num(void)
{
	/* Invalid nodenum. */
	KASSERT(knode_set_num(-1) == -EINVAL);
	KASSERT(knode_set_num(PROCESSOR_NOC_NODES_NUM) == -EINVAL);
}

/*----------------------------------------------------------------------------*
 * Exchange Logical NoC Node Number with bad arguments                        *
 *----------------------------------------------------------------------------*/

/**
 * @brief FAULT Test: Bad Set Logical NoC Node Number
 */
PRIVATE void test_node_bad_set_num(void)
{
	int nodenum;

	/* nodenum + (Interfaces available in only one IO Cluster). */
	nodenum = PROCESSOR_NODENUM_MASTER + (PROCESSOR_NOC_IONODES_NUM / PROCESSOR_IOCLUSTERS_NUM);

	/* Bad nodenum. */
	KASSERT(knode_set_num(nodenum) == -EINVAL);
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API Tests.
 */
PRIVATE struct test test_fault_noc[] = {
	{ test_node_invalid_set_num, "[test][processor][node][fault] invalid set logical noc node num [passed]\n" },
	{ test_node_bad_set_num,     "[test][processor][node][fault] bad set logical noc node num     [passed]\n" },
	{ NULL,                       NULL                                                                        },
};

/**
 * The test_noc() function launches regression tests on the NoC
 * Interface of the Processor Abstraction Layer.
 *
 * @author Pedro Henrique Penna
 * @author João Vicente Souto
 */
PUBLIC void test_noc(void)
{
	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------\n");
	for (int i = 0; test_api_noc[i].test_fn != NULL; i++)
	{
		test_api_noc[i].test_fn();
        nanvix_puts(test_api_noc[i].name);
	}

	/* FAULT Tests */
	nanvix_puts("--------------------------------------------------------------------------------\n");
	for (int i = 0; test_fault_noc[i].test_fn != NULL; i++)
	{
		test_fault_noc[i].test_fn();
        nanvix_puts(test_fault_noc[i].name);
	}
}