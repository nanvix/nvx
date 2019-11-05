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

#include <nanvix/sys/sync.h>

#if __TARGET_HAS_SYNC

#include <nanvix/sys/perf.h>
#include <nanvix/sys/thread.h>
#include <nanvix/runtime/stdikc.h>

/**
 * @brief Kernel standard sync.
 */
static int __stdsync[THREAD_MAX] = {
	[0 ... (THREAD_MAX - 1)] = -1
};

/**
 * @brief Forces a platform-independent delay.
 *
 * @param cycles Delay in cycles.
 *
 * @author João Vicente Souto
 */
static void delay(uint64_t cycles)
{
	uint64_t t0, t1;

	for (int i = 0; i < PROCESSOR_CLUSTERS_NUM; ++i)
	{
		kclock(&t0);

		do
			kclock(&t1);
		while ((t1 - t0) < cycles);
	}
}

/*
 * Build a list of the node IDs
 *
 * @param nodes
 * @param nioclusters
 * @param ncclusters
 *
 * @author João Vicente Souto
 */
static void build_node_list(int *nodes, int nioclusters, int ncclusters)
{
	int base;
	int step;
	int index;
	int max_clusters;

	index = 0;

	/* Build node IDs of the IO Clusters. */
	base         = 0;
	max_clusters = PROCESSOR_IOCLUSTERS_NUM;
	step         = (PROCESSOR_NOC_IONODES_NUM / PROCESSOR_IOCLUSTERS_NUM);
	for (int i = 0; i < max_clusters && i < nioclusters; i++, index++)
		nodes[index] = base + (i * step);

	/* Build node IDs of the Compute Clusters. */
	base         = PROCESSOR_IOCLUSTERS_NUM * (PROCESSOR_NOC_IONODES_NUM / PROCESSOR_IOCLUSTERS_NUM);
	max_clusters = PROCESSOR_CCLUSTERS_NUM;
	step         = (PROCESSOR_NOC_CNODES_NUM / PROCESSOR_CCLUSTERS_NUM);
	for (int i = 0; i < max_clusters && i < ncclusters; i++, index++)
		nodes[index] = base + (i * step);
}


/**
 * @todo TODO: provide a detailed description for this function.
 */
int __stdsync_setup(void)
{
	int tid;
	int nodes[PROCESSOR_CLUSTERS_NUM];

	tid = kthread_self();

	build_node_list(nodes, PROCESSOR_IOCLUSTERS_NUM, PROCESSOR_CCLUSTERS_NUM);

	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
	{
		return (((__stdsync[tid] = ksync_create(nodes, PROCESSOR_CLUSTERS_NUM, SYNC_ALL_TO_ONE)) < 0) ?
			-1 : 0
		);
	}

	/* Slave cluster. */
	return (((__stdsync[tid] = ksync_open(nodes, PROCESSOR_CLUSTERS_NUM, SYNC_ALL_TO_ONE)) < 0) ?
		-1 : -0
	);
}

/**
 * @todo TODO: provide a detailed description for this function.
 */
int __stdsync_cleanup(void)
{
	int tid;

	tid = kthread_self();

	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
		return (ksync_unlink(__stdsync[tid]));

	return (ksync_close(__stdsync[tid]));
}

/**
 * @todo TODO: provide a detailed description for this function.
 */
int stdsync_fence(void)
{
	int tid;

	tid = kthread_self();

	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
		return (ksync_wait(__stdsync[tid]));

#ifndef __unix64__
	/* Waits one second. */
	delay(CLUSTER_FREQ);
#endif

	return (ksync_signal(__stdsync[tid]));
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_MAILBOX */
