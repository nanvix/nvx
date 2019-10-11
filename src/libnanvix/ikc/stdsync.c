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
#include <nanvix/runtime/stdikc.h>

/**
 * @brief Kernel standard sync.
 */
static int __stdsync = -1;

/**
 * @todo TODO: provide a detailed description for this function.
 */
int __stdsync_setup(void)
{
	int nodes[PROCESSOR_CLUSTERS_NUM];

	for (int i = 0; i < PROCESSOR_CLUSTERS_NUM; i++)
		nodes[i] = i;

	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
		return ((__stdsync = ksync_create(nodes, PROCESSOR_CLUSTERS_NUM, SYNC_ALL_TO_ONE)));

	/* Slave cluster. */
	return (((__stdsync = ksync_open(nodes, PROCESSOR_CLUSTERS_NUM, SYNC_ALL_TO_ONE)) < 0) ?
		-1 : -0
	);
}

/**
 * @todo TODO: provide a detailed description for this function.
 */
int __stdsync_cleanup(void)
{
	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
		return (ksync_unlink(__stdsync));

	return (ksync_close(__stdsync));
}

/**
 * @todo TODO: provide a detailed description for this function.
 */
int stdsync_fence(void)
{
	/* Master cluster */
	if (cluster_get_num() == PROCESSOR_CLUSTERNUM_MASTER)
		return (ksync_wait(__stdsync));

	return (ksync_signal(__stdsync));
}
