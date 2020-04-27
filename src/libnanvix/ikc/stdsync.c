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

#include <nanvix/sys/sync.h>

#if __TARGET_HAS_SYNC

#include <nanvix/sys/perf.h>
#include <nanvix/sys/thread.h>
#include <nanvix/runtime/stdikc.h>

/**
 * @brief Kernel standard sync.
 */
static int __stdbarrier[THREAD_MAX + 1][2] = {
	[0 ... (THREAD_MAX)] = { -1, -1 },
};

#ifndef __unix64__

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

#endif /* !__unix64__ */

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
	int ret;
	int nodes[PROCESSOR_CLUSTERS_NUM];

	tid = kthread_self();

	build_node_list(nodes, PROCESSOR_IOCLUSTERS_NUM, PROCESSOR_CCLUSTERS_NUM);

	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
	{
		ret = __stdbarrier[tid][0] = ksync_create(
			nodes, PROCESSOR_CLUSTERS_NUM, SYNC_ALL_TO_ONE
		);
		if (ret >= 0)
		{
			ret = __stdbarrier[tid][1] = ksync_open(
				nodes, PROCESSOR_CLUSTERS_NUM, SYNC_ONE_TO_ALL
			);

			if (ret < 0)
				ksync_unlink(__stdbarrier[tid][0]);
		}
	}

	else
	{
		ret = __stdbarrier[tid][0] = ksync_open(
			nodes, PROCESSOR_CLUSTERS_NUM, SYNC_ALL_TO_ONE
		);
		if (ret >= 0)
		{
			ret = __stdbarrier[tid][1] = ksync_create(
				nodes, PROCESSOR_CLUSTERS_NUM, SYNC_ONE_TO_ALL
			);

			if (ret < 0)
				ksync_close(__stdbarrier[tid][0]);
		}
	}

	/* Slave cluster. */
	return ((ret < 0) ? -1 : -0);
}

/**
 * @todo TODO: provide a detailed description for this function.
 * @bug FIXME: return value on bad cases.
 */
int __stdsync_cleanup(void)
{
	int tid;
	int ret;

	if ((tid = kthread_self()) > THREAD_MAX)
		return (-1);

	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
	{
		ret = ksync_unlink(__stdbarrier[tid][0]);
		ret = ksync_close(__stdbarrier[tid][1]);
	}

	else
	{
		ret = ksync_close(__stdbarrier[tid][0]);
		ret = ksync_unlink(__stdbarrier[tid][1]);
	}

	return (ret);
}

/**
 * @todo TODO: provide a detailed description for this function.
 * @bug FIXME: return value on bad cases.
 */
int stdsync_fence(void)
{
	int tid;

	if ((tid = kthread_self()) > THREAD_MAX)
		return (-1);


	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
	{
		KASSERT(ksync_wait(__stdbarrier[tid][0]) == 0);
		KASSERT(ksync_signal(__stdbarrier[tid][1]) == 0);
	}

	/* Slave cluster. */
	else
	{
		/* Waits one second. */
		#ifndef __unix64__
			delay(CLUSTER_FREQ);
		#endif

		KASSERT(ksync_signal(__stdbarrier[tid][0]) == 0);
		KASSERT(ksync_wait(__stdbarrier[tid][1]) == 0);
	}

	return (0);
}

/**
 * @todo TODO: provide a detailed description for this function.
 */
int stdsync_get(void)
{
	int tid;

	if ((tid = kthread_self()) > THREAD_MAX)
		return (-1);

	return (__stdbarrier[tid][0]);
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_MAILBOX */
