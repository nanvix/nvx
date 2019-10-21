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

#include <nanvix/kernel/kernel.h>

#if __TARGET_HAS_SYNC

#include <nanvix/sys/noc.h>
#include <posix/errno.h>

/*============================================================================*
 * ksync_list_is_valid()                                                      *
 *============================================================================*/

/**
 * @brief Check if the list of RX/TX NoC nodes is valid.
 *
 * @param local  Logic ID of the local node.
 * @param nodes  Logic IDs of target NoC nodes.
 * @param nnodes Number of target NoC nodes.
 *
 * @return Non zero if only one occurrence of local was found and zero
 * otherwise.
 */
PRIVATE int ksync_list_is_valid(int local, const int *nodes, int nnodes)
{
	uint64_t checks;

	/*
	 * TODO: move this assertation to the HAL.
	 */
	KASSERT(PROCESSOR_NOC_NODES_NUM <= 64);

	checks = 0ULL;

	/* Build list of RX NoC nodes. */
	for (int i = 0; i < nnodes; ++i)
	{
		if (!WITHIN(nodes[i], 0, PROCESSOR_NOC_NODES_NUM))
			return (0);

		/* Does a node appear twice? */
		if (checks & (1ULL << nodes[i]))
			return (0);

		checks |= (1ULL << nodes[i]);
	}

	/* Local Node found. */
	return (checks & (1ULL << local));
}

/*============================================================================*
 * ksync_create()                                                             *
 *============================================================================*/

/**
 * @details The ksync_create() function creates an input sync
 * and attaches it to the list NoC node @p nodes.
 */
int ksync_create(const int *nodes, int nnodes, int type)
{
	int ret;
	int local;

	/* Invalid nodes list. */
	if (nodes == NULL)
		return (-EINVAL);

	/* Invalid number of nodes. */
	if (!WITHIN(nnodes, 2, (PROCESSOR_NOC_NODES_NUM + 1)))
		return(-EINVAL);

	local = knode_get_num();

	if (type == SYNC_ONE_TO_ALL)
	{
		/* Is the local node not the sender? */
		if (local == nodes[0])
			return (-EINVAL);
	}

	else if (type == SYNC_ALL_TO_ONE)
	{
		/* Is the local node not the receiver? */
		if (local != nodes[0])
			return (-EINVAL);
	}

	/* Invalid type. */
	else
		return (-EINVAL);

	/* Is the node list not valid? */
	if (!ksync_list_is_valid(local, nodes, nnodes))
			return (-EINVAL);

	ret = kcall3(
		NR_sync_create,
		(word_t) nodes,
		(word_t) nnodes,
		(word_t) type
	);

	return (ret);
}

/*============================================================================*
 * ksync_open()                                                               *
 *============================================================================*/

/**
 * @details The ksync_open() function opens an output sync to the
 * NoC node list @p nodes.
 */
int ksync_open(const int *nodes, int nnodes, int type)
{
	int ret;
	int local;

	/* Invalid list of nodes. */
	if (nodes == NULL)
		return (-EINVAL);

	/* Invalid number of nodes. */
	if (!WITHIN(nnodes, 2, (PROCESSOR_NOC_NODES_NUM + 1)))
		return(-EINVAL);

	local = knode_get_num();

	if (type == SYNC_ONE_TO_ALL)
	{
		/* Is the local node not the sender? */
		if (local != nodes[0])
			return (-EINVAL);
	}

	else if (type == SYNC_ALL_TO_ONE)
	{
		/* Is the local node not the receiver? */
		if (local == nodes[0])
			return (-EINVAL);
	}

	/* Invalid type. */
	else
		return (-EINVAL);

	/* Is the node list not valid? */
	if (!ksync_list_is_valid(local, nodes, nnodes))
			return (-EINVAL);

	ret = kcall3(
		NR_sync_open,
		(word_t) nodes,
		(word_t) nnodes,
		(word_t) type
	);

	return (ret);
}

/*============================================================================*
 * ksync_wait()                                                               *
 *============================================================================*/

/**
 * @details The ksync_wait() waits incomming signal on a input sync @p syncid.
 */
int ksync_wait(int syncid)
{
	int ret;

	ret = kcall1(
		NR_sync_wait,
		(word_t) syncid
	);

	return (ret);
}

/*============================================================================*
 * ksync_signal()                                                             *
 *============================================================================*/

/**
 * @details The ksync_signal() emmit a signal from a output sync @p syncid.
 */
int ksync_signal(int syncid)
{
	int ret;

	ret = kcall1(
		NR_sync_signal,
		(word_t) syncid
	);

	return (ret);
}

/*============================================================================*
 * ksync_close()                                                              *
 *============================================================================*/

/**
 * @details The ksync_close() function closes and releases the
 * underlying resources associated to the output sync @p syncid.
 */
int ksync_close(int syncid)
{
	int ret;

	ret = kcall1(
		NR_sync_close,
		(word_t) syncid
	);

	return (ret);
}

/*============================================================================*
 * ksync_unlink()                                                             *
 *============================================================================*/

/**
 * @details The ksync_unlink() function removes and releases the underlying
 * resources associated to the input sync @p syncid.
 */
int ksync_unlink(int syncid)
{
	int ret;

	ret = kcall1(
		NR_sync_unlink,
		(word_t) syncid
	);

	return (ret);
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_MAILBOX */
