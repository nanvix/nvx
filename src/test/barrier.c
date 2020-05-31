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

#include <nanvix/sys/perf.h>
#include <nanvix/sys/sync.h>
#include <nanvix/sys/noc.h>
#include <posix/errno.h>

#if (__TARGET_HAS_SYNC)

/*----------------------------------------------------------------------------*
 * Delay                                                                      *
 *----------------------------------------------------------------------------*/

/**
 * @brief Forces a platform-independent delay.
 *
 * @param cycles Delay in cycles.
 *
 * @author João Vicente Souto
 */
PUBLIC void test_delay(int times, uint64_t cycles)
{
	uint64_t t0, t1;

	for (int i = 0; i < times; ++i)
	{
		kclock(&t0);

		do
			kclock(&t1);
		while ((t1 - t0) < cycles);
	}
}

/*----------------------------------------------------------------------------*
 * Nodes                                                                      *
 *----------------------------------------------------------------------------*/

PRIVATE int _syncin         = -1;
PRIVATE int _syncout        = -1;
PRIVATE int _node_is_master = 0;

/**
 * @brief Barrier setup function.
 */
PUBLIC void test_barrier_nodes_setup(const int * nodes, int nnodes, int is_master)
{
	KASSERT((nodes != NULL) && (nnodes > 0));

	_node_is_master = is_master;

	if (_node_is_master)
	{
		_syncin  = ksync_create(nodes, nnodes, SYNC_ALL_TO_ONE);
		_syncout = ksync_open(nodes, nnodes, SYNC_ONE_TO_ALL);
	}
	else
	{
		test_delay(1, CLUSTER_FREQ);

		_syncout = ksync_open(nodes, nnodes, SYNC_ALL_TO_ONE);
		_syncin  = ksync_create(nodes, nnodes, SYNC_ONE_TO_ALL);
	}

	KASSERT((_syncout >= 0) && (_syncin >= 0));
}

/**
 * @brief Barrier function.
 */
PUBLIC void test_barrier_nodes(void)
{
	if (_node_is_master)
	{
		ksync_wait(_syncin);
		ksync_signal(_syncout);
	}
	else
	{
		ksync_signal(_syncout);
		ksync_wait(_syncin);
	}
}

/**
 * @brief Barrier cleanup function.
 */
PUBLIC void test_barrier_nodes_cleanup(void)
{
	ksync_unlink(_syncin);
	ksync_close(_syncout);

	_syncin    = -1;
	_syncout   = -1;
	_node_is_master = 0;
}

#endif /* __TARGET_HAS_SYNC */
