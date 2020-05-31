/*
 * MIT License
 *
 * Copyright(c) 2011-2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
 *              2015-2016 Davidson Francis     <davidsondfgl@gmail.com>
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

#include <nanvix/sys/sync.h>
#include <nanvix/sys/noc.h>
#include <posix/errno.h>

#include "test.h"

#if __TARGET_HAS_SYNC

/**
 * @brief Test's parameters
 */
#define NR_NODES       2
#define NR_NODES_MAX   PROCESSOR_NOC_NODES_NUM
#define MASTER_NODENUM 0
#ifdef __mppa256__
	#define SLAVE_NODENUM  8
#else
	#define SLAVE_NODENUM  1
#endif

/**
 * @brief Auxiliar array
 */
int nodenums[NR_NODES] = {
	SLAVE_NODENUM,
	MASTER_NODENUM,
};

/*============================================================================*
 * API Test: Create Unlink                                                    *
 *============================================================================*/

/**
 * @brief API Test: Synchronization Point Create Unlink
 */
void test_api_sync_create_unlink(void)
{
	int tmp;
	int syncid;
	int nodes[NR_NODES];

	nodes[0] = knode_get_num();

	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodes[0])
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncid = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	test_assert(ksync_unlink(syncid) == 0);

	tmp = nodes[0];
	nodes[0] = nodes[1];
	nodes[1] = tmp;

	test_assert((syncid = ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
	test_assert(ksync_unlink(syncid) == 0);
}

/*============================================================================*
 * API Test: Open Close                                                       *
 *============================================================================*/

/**
 * @brief API Test: Synchronization Point Open Close
 */
void test_api_sync_open_close(void)
{
	int tmp;
	int syncid;
	int nodes[NR_NODES];

	nodes[0] = knode_get_num();

	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodes[0])
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncid = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
	test_assert(ksync_close(syncid) == 0);

	tmp = nodes[0];
	nodes[0] = nodes[1];
	nodes[1] = tmp;

	test_assert((syncid = ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	test_assert(ksync_close(syncid) == 0);
}

/*============================================================================*
 * API Test: Get latency                                                      *
 *============================================================================*/

/**
 * @brief API Test: Sync Get latency
 */
static void test_api_sync_get_latency(void)
{
	int tmp;
	int syncin;
	int syncout;
	uint64_t latency;
	int nodes[NR_NODES];

	nodes[0] = knode_get_num();

	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodes[0])
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);
		test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);

	test_assert(ksync_close(syncout) == 0);
	test_assert(ksync_unlink(syncin) == 0);

	tmp = nodes[0];
	nodes[0] = nodes[1];
	nodes[1] = tmp;

	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);
		test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);

	test_assert(ksync_close(syncout) == 0);
	test_assert(ksync_unlink(syncin) == 0);
}

/*============================================================================*
 * API Test: Get counters                                                     *
 *============================================================================*/

/**
 * @brief API Test: Sync Get latency
 */
static void test_api_sync_get_counters(void)
{
	int syncin;
	int syncout;
	uint64_t c0;
	uint64_t c1;
	int nodes[NR_NODES];

	nodes[0] = knode_get_num();

	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodes[0])
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NCREATES, &c0) == 0);
		test_assert(c0 == 5);
		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NOPENS, &c1) == 0);
		test_assert(c1 == 4);
		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NCLOSES, &c1) == 0);
		test_assert(c1 == 4);

	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NCREATES, &c0) == 0);
		test_assert(c0 == 5);
		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NOPENS, &c0) == 0);
		test_assert(c0 == 5);
		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NUNLINKS, &c1) == 0);
		test_assert(c1 == 4);
		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NCLOSES, &c1) == 0);
		test_assert(c1 == 4);

		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NWAITS, &c1) == 0);
		test_assert(c1 == 0);
		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NSIGNALS, &c1) == 0);
		test_assert(c1 == 0);

	test_assert(ksync_close(syncout) == 0);

		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_NCLOSES, &c1) == 0);
		test_assert(c1 == 5);

	test_assert(ksync_unlink(syncin) == 0);
}

/*============================================================================*
 * API Test: Virtualization                                                   *
 *============================================================================*/

#define TEST_VIRTUALIZATION_SYNCS_NR 5

/**
 * @brief API Test: Synchronization Point virtualization
 */
void test_api_sync_virtualization(void)
{
	int tmp;
	int sync_in[TEST_VIRTUALIZATION_SYNCS_NR*2];
	int sync_out[TEST_VIRTUALIZATION_SYNCS_NR*2];
	int nodes[NR_NODES];

	nodes[0] = knode_get_num();

	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodes[0])
			continue;

		nodes[j++] = nodenums[i];
	}

	/* Creates multiple virtual synchronization points. */
	for (unsigned i = 0; i < TEST_VIRTUALIZATION_SYNCS_NR; ++i)
		test_assert((sync_in[i] = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

	tmp = nodes[0];
	nodes[0] = nodes[1];
	nodes[1] = tmp;

	for (unsigned i = TEST_VIRTUALIZATION_SYNCS_NR; i < TEST_VIRTUALIZATION_SYNCS_NR*2; ++i)
	{
		test_assert((sync_in[i] = ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
		test_assert((sync_out[i] = ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	}

	tmp = nodes[0];
	nodes[0] = nodes[1];
	nodes[1] = tmp;

	for (unsigned i = 0; i < TEST_VIRTUALIZATION_SYNCS_NR; ++i)
		test_assert((sync_out[i] = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

	/* Deletion of the created virtual synchronization points. */
	for (unsigned i = 0; i < TEST_VIRTUALIZATION_SYNCS_NR*2; ++i)
	{
		test_assert(ksync_unlink(sync_in[i]) == 0);
		test_assert(ksync_close(sync_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Signal Wait                                                      *
 *============================================================================*/

/**
 * @brief API Test: Synchronization Point Signal Wait
 */
void test_api_sync_signal_wait(void)
{
	int syncin;
	int syncout;
	int nodenum;
	int nodes[NR_NODES];
	uint64_t latency;
	uint64_t counter;

	nodenum = knode_get_num();
	nodes[0] = MASTER_NODENUM;

	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == MASTER_NODENUM)
			continue;

		nodes[j++] = nodenums[i];
	}

	if (nodenum != MASTER_NODENUM)
	{
		test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
		test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);
		test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);

		test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_NWAITS, &counter) == 0);
		test_assert(counter == 0);
		test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_NSIGNALS, &counter) == 0);
		test_assert(counter == 0);

		for (int i = 0; i < NITERATIONS; i++)
		{
			test_assert(ksync_wait(syncin) == 0);
			test_assert(ksync_signal(syncout) == 0);
		}
	}
	else
	{
		test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
		test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

		test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);
		test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);

		test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_NWAITS, &counter) == 0);
		test_assert(counter == 0);
		test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_NSIGNALS, &counter) == 0);
		test_assert(counter == 0);

		test_delay(1, CLUSTER_FREQ);

		for (int i = 0; i < NITERATIONS; i++)
		{
			test_assert(ksync_signal(syncout) == 0);
			test_assert(ksync_wait(syncin) == 0);
		}
	}

	test_assert(ksync_ioctl(syncin, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);
	test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_NWAITS, &counter) == 0);
	test_assert(counter == NITERATIONS);
	test_assert(ksync_ioctl(syncout, KSYNC_IOCTL_GET_NSIGNALS, &counter) == 0);
	test_assert(counter == NITERATIONS);

	test_assert(ksync_close(syncout) == 0);
	test_assert(ksync_unlink(syncin) == 0);
}

/*============================================================================*
 * API Test: Multiplexation                                                   *
 *============================================================================*/

/**
 * @brief API Test: Synchronization Point multiplexation
 */
void test_api_sync_multiplexation(void)
{
	int syncin[2];
	int syncout[2];
	int nodenum;
	int nodes[NR_NODES];
	uint64_t latency;

	nodenum = knode_get_num();
	nodes[0] = (nodenum == MASTER_NODENUM) ? MASTER_NODENUM : SLAVE_NODENUM;

	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	/* Initialize the synchronization points. */
	test_assert((syncin[1] = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	test_assert((syncout[0] = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

	test_assert(ksync_ioctl(syncin[1], KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);
	test_assert(ksync_ioctl(syncout[0], KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);

	nodes[NR_NODES - 1] = (nodenum == MASTER_NODENUM) ? MASTER_NODENUM : SLAVE_NODENUM;

	for (int i = 0, j = 0; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncin[0] = ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout[1] = ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

	test_assert(ksync_ioctl(syncin[0], KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);
	test_assert(ksync_ioctl(syncout[1], KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);

	if (nodenum != MASTER_NODENUM)
	{
		for (int i = 1; i < NITERATIONS; i++)
		{
			test_assert(ksync_wait(syncin[0]) == 0);
			test_assert(ksync_wait(syncin[1]) == 0);

			test_assert(ksync_signal(syncout[0]) == 0);
			test_assert(ksync_signal(syncout[1]) == 0);
		}
	}
	else
	{
		test_delay(1, CLUSTER_FREQ);

		for (int i = 1; i < NITERATIONS; i++)
		{
			test_assert(ksync_signal(syncout[0]) == 0);
			test_assert(ksync_signal(syncout[1]) == 0);

			test_assert(ksync_wait(syncin[0]) == 0);
			test_assert(ksync_wait(syncin[1]) == 0);
		}
	}

	/* Destroy the used synchronization points. */
	for (int i = 0; i < 2; ++i)
	{
		test_assert(ksync_ioctl(syncin[i], KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency > 0);
		test_assert(ksync_ioctl(syncout[i], KSYNC_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency > 0);

		test_assert(ksync_close(syncout[i]) == 0);
		test_assert(ksync_unlink(syncin[i]) == 0);
	}
}

/*============================================================================*
 * Fault Test: Invalid Create                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Invalid Create
 */
void test_fault_sync_invalid_create(void)
{
	int nodenum;
	int remote;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	remote = (nodenum == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;
	nodes[0] = remote;

	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == (nodenum == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM))
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((ksync_create(NULL, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_create(nodes, -1, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_create(nodes, 0, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_create(nodes, 1, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_create(nodes, NR_NODES_MAX + 1, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_create(nodes, NR_NODES, -1)) == -EINVAL);
	nodes[1] = nodes[0];
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
	nodes[0] = -1;
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
	nodes[0] = PROCESSOR_NOC_NODES_NUM;
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
	nodes[0] = remote;
	nodes[1] = remote;
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Create                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Bad Create
 */
void test_fault_sync_bad_create1(void)
{
	int nodenum;
	int nodes[NR_NODES];

	nodenum = knode_get_num();

	/* Underlying NoC node is the sender. */
	nodes[0] = nodenum;
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);

	/* Underlying NoC node is not listed. */
	nodes[0] = (nodenum == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);

	/* Underlying NoC node appears twice in the list. */
	nodes[NR_NODES - 1] = nodenum;
	nodes[NR_NODES - 2] = nodenum;
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
}

/**
 * @brief Fault Test: Synchronization Point Bad Create
 */
void test_fault_sync_bad_create2(void)
{
	int nodenum;
	int nodes[NR_NODES];

	nodenum = knode_get_num();

	/* Underlying NoC node is not the receiver. */
	nodes[0] = (nodenum == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == (nodenum == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM))
			continue;

		nodes[j++] = nodenums[i];
	}
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) == -EINVAL);

	/* Underlying NoC node is not listed. */
	nodes[0] = (nodenum == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) == -EINVAL);

	/* Underlying NoC node appears twice in the list. */
	nodes[NR_NODES - 1] = nodenum;
	nodes[NR_NODES - 2] = nodenum;
	test_assert((ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) == -EINVAL);
}

/**
 * @brief Fault Test: Synchronization Point Bad Create
 */
void test_fault_sync_bad_create(void)
{
	test_fault_sync_bad_create1();
	test_fault_sync_bad_create2();
}

/*============================================================================*
 * Fault Test: Invalid Open                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Invalid Open
 */
void test_fault_sync_invalid_open(void)
{
	int nodenum;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((ksync_open(NULL, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_open(nodes, -1, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_open(nodes, 0, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_open(nodes, 1, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_open(nodes, NR_NODES_MAX + 1, SYNC_ONE_TO_ALL)) == -EINVAL);
	test_assert((ksync_open(nodes, NR_NODES, -1)) == -EINVAL);
	nodes[1] = nodes[0];
	test_assert((ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
	nodes[0] = -1;
	test_assert((ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
	nodes[0] = PROCESSOR_NOC_NODES_NUM;
	test_assert((ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
	nodes[0] = nodenum;
	nodes[1] = nodenum;
	test_assert((ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Open                                                       *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Bad Open
 */
void test_fault_sync_bad_open1(void)
{
	int nodenum;
	int nodes[NR_NODES];

	nodenum = knode_get_num();

	/* Underlying NoC node is not the sender. */
	nodes[NR_NODES - 1] = nodenum;
	for (int i = 0, j = 0; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}
	test_assert((ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);

	/* Underlying NoC node is not listed. */
	test_assert((ksync_open(nodes, NR_NODES - 1, SYNC_ONE_TO_ALL)) == -EINVAL);

	/* Underlying NoC node appears twice in the list. */
	nodes[0] = nodenum;
	nodes[NR_NODES - 1] = nodenum;
	test_assert((ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) == -EINVAL);
}

/**
 * @brief Fault Test: Synchronization Point Bad Open
 */
void test_fault_sync_bad_open2(void)
{
	int nodenum;
	int nodes[NR_NODES];

	nodenum = knode_get_num();

	/* Underlying NoC node is not the sender. */
	nodes[0] = nodenum;
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}
	test_assert((ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) == -EINVAL);

	/* Underlying NoC node is not listed. */
	test_assert((ksync_open(&nodes[1], NR_NODES - 1, SYNC_ALL_TO_ONE)) == -EINVAL);

	/* Underlying NoC node appears twice in the list. */
	nodes[0] = nodenum;
	nodes[NR_NODES - 1] = nodenum;
	test_assert((ksync_open(&nodes[1], NR_NODES, SYNC_ALL_TO_ONE)) == -EINVAL);
}

/**
 * @brief Fault Test: Synchronization Point Bad Open
 */
void test_fault_sync_bad_open(void)
{
	test_fault_sync_bad_open1();
	test_fault_sync_bad_open2();
}

/*============================================================================*
 * Fault Test: Invalid Unlink                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Invalid Unlink
 */
void test_fault_sync_invalid_unlink(void)
{
	test_assert(ksync_unlink(-1) == -EINVAL);
	test_assert(ksync_unlink(KSYNC_MAX) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Unlink                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Bad Unlink
 */
void test_fault_sync_bad_unlink(void)
{
	int nodenum;
	int syncid;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncid = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

		test_assert(ksync_unlink(syncid) == -EBADF);

	test_assert(ksync_close(syncid) == 0);
}

/*============================================================================*
 * Fault Test: Double Unlink                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Double Unlink
 */
void test_fault_sync_double_unlink(void)
{
	int nodenum;
	int syncid;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncid = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	test_assert(ksync_unlink(syncid) == 0);
	test_assert(ksync_unlink(syncid) == -EBADF);
}

/*============================================================================*
 * Fault Test: Invalid Close                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Invalid Close
 */
void test_fault_sync_invalid_close(void)
{
	test_assert(ksync_close(-1) == -EINVAL);
	test_assert(ksync_close(KSYNC_MAX) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Close                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Bad Close
 */
void test_fault_sync_bad_close(void)
{
	int nodenum;
	int syncid;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncid = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

		test_assert(ksync_close(syncid) == -EBADF);

	test_assert(ksync_unlink(syncid) == 0);
}

/*============================================================================*
 * Fault Test: Double Close                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Double Close
 */
void test_fault_sync_double_close(void)
{
	int nodenum;
	int syncid;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncid = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
	test_assert(ksync_close(syncid) == 0);
	test_assert(ksync_close(syncid) == -EBADF);
}

/*============================================================================*
 * Fault Test: Invalid Signal                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Invalid Signal
 */
void test_fault_sync_invalid_signal(void)
{
	test_assert(ksync_signal(-1) == -EINVAL);
	test_assert(ksync_signal(KSYNC_MAX) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Signal                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Bad Signal
 */
void test_fault_sync_bad_signal(void)
{
	int nodenum;
	int syncid;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncid = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

		test_assert(ksync_signal(syncid) == -EBADF);

	test_assert(ksync_unlink(syncid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Wait                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Invalid Wait
 */
void test_fault_sync_invalid_wait(void)
{
	test_assert(ksync_wait(-1) == -EINVAL);
	test_assert(ksync_wait(KSYNC_MAX) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Wait                                                       *
 *============================================================================*/

/**
 * @brief Fault Test: Synchronization Point Bad Wait
 */
void test_fault_sync_bad_wait(void)
{
	int nodenum;
	int syncid;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncid = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

		test_assert(ksync_wait(syncid) == -EBADF);

	test_assert(ksync_close(syncid) == 0);
}

/*============================================================================*
 * Fault Test: Bad Syncid                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Unused Synchronization Point operations
 */
void test_fault_sync_bad_syncid(void)
{
	int nodenum;
	int syncin;
	int syncout;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

	test_assert(ksync_close(syncout + 2) == -EBADF);
	test_assert(ksync_unlink(syncin + 2) == -EBADF);
	test_assert(ksync_signal(syncout + 2) == -EBADF);
	test_assert(ksync_wait(syncin + 2) == -EBADF);

	test_assert(ksync_close(syncout) == 0);
	test_assert(ksync_unlink(syncin) == 0);
}

/*============================================================================*
 * Fault Test: Invalid ioctl                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid ioctl
 */
static void test_fault_sync_invalid_ioctl(void)
{
	int syncid;
	int nodenum;
	uint64_t latency;
	int nodes[NR_NODES];

	nodenum = knode_get_num();
	nodes[0] = nodenum;

	/* Build nodes list. */
	for (int i = 0, j = 1; i < NR_NODES; i++)
	{
		if (nodenums[i] == nodenum)
			continue;

		nodes[j++] = nodenums[i];
	}

	test_assert(ksync_ioctl(-1, KSYNC_IOCTL_GET_LATENCY, &latency) == -EINVAL);
	test_assert(ksync_ioctl(KSYNC_MAX, KSYNC_IOCTL_GET_LATENCY, &latency) == -EINVAL);

	test_assert((syncid = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

		test_assert(ksync_ioctl(syncid, -1, &latency) == -ENOTSUP);
		test_assert(ksync_ioctl(syncid, KSYNC_IOCTL_GET_LATENCY, NULL) == -EFAULT);

	test_assert(ksync_unlink(syncid) == 0);
}


/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
static struct test sync_tests_api[] = {
	{ test_api_sync_create_unlink,  "[test][sync][api] sync create/unlink  [passed]" },
	{ test_api_sync_open_close,     "[test][sync][api] sync open/close     [passed]" },
	{ test_api_sync_get_latency,    "[test][sync][api] sync get latency    [passed]" },
	{ test_api_sync_get_counters,   "[test][sync][api] sync get counters   [passed]" },
	{ test_api_sync_virtualization, "[test][sync][api] sync virtualization [passed]" },
	{ test_api_sync_signal_wait,    "[test][sync][api] sync wait           [passed]" },
	{ test_api_sync_multiplexation, "[test][sync][api] sync multiplexation [passed]" },
	{ NULL,                          NULL                                            },
};

/**
 * @brief Fault tests.
 */
static struct test sync_tests_fault[] = {
	{ test_fault_sync_invalid_create, "[test][sync][fault] sync invalid create [passed]" },
	{ test_fault_sync_bad_create,     "[test][sync][fault] sync bad create     [passed]" },
	{ test_fault_sync_invalid_open,   "[test][sync][fault] sync invalid open   [passed]" },
	{ test_fault_sync_bad_open,       "[test][sync][fault] sync bad open       [passed]" },
	{ test_fault_sync_invalid_unlink, "[test][sync][fault] sync invalid unlink [passed]" },
	{ test_fault_sync_bad_unlink,     "[test][sync][fault] sync bad unlink     [passed]" },
	{ test_fault_sync_double_unlink,  "[test][sync][fault] sync double unlink  [passed]" },
	{ test_fault_sync_invalid_close,  "[test][sync][fault] sync invalid close  [passed]" },
	{ test_fault_sync_bad_close,      "[test][sync][fault] sync bad close      [passed]" },
	{ test_fault_sync_double_close,   "[test][sync][fault] sync double close   [passed]" },
	{ test_fault_sync_invalid_signal, "[test][sync][fault] sync invalid signal [passed]" },
	{ test_fault_sync_bad_signal,     "[test][sync][fault] sync bad signal     [passed]" },
	{ test_fault_sync_invalid_wait,   "[test][sync][fault] sync invalid wait   [passed]" },
	{ test_fault_sync_bad_wait,       "[test][sync][fault] sync bad wait       [passed]" },
	{ test_fault_sync_bad_syncid,     "[test][sync][fault] sync bad syncid     [passed]" },
	{ test_fault_sync_invalid_ioctl,  "[test][sync][fault] sync invalid ioctl  [passed]" },
	{ NULL,                            NULL                                              },
};

/**
 * The test_thread_mgmt() function launches testing units on thread manager.
 *
 * @author Pedro Henrique Penna
 */
void test_sync(void)
{
	int nodenum;

	nodenum = knode_get_num();

	if (nodenum == MASTER_NODENUM || nodenum == SLAVE_NODENUM)
	{
		/* API Tests */
		if (nodenum == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------");
		for (int i = 0; sync_tests_api[i].test_fn != NULL; i++)
		{
			sync_tests_api[i].test_fn();

			if (nodenum == MASTER_NODENUM)
				nanvix_puts(sync_tests_api[i].name);
		}

		/* Fault Tests */
		if (nodenum == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------");
		for (int i = 0; sync_tests_fault[i].test_fn != NULL; i++)
		{
			sync_tests_fault[i].test_fn();

			if (nodenum == MASTER_NODENUM)
				nanvix_puts(sync_tests_fault[i].name);
		}
	}
}

#endif /* __TARGET_SYNC */
