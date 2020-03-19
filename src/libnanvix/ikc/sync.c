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
 * ksync_create()                                                             *
 *============================================================================*/

/**
 * @details The ksync_create() function creates an input sync
 * and attaches it to the list NoC node @p nodes.
 */
int ksync_create(const int *nodes, int nnodes, int type)
{
	int ret;
	int nodenum;

	/* Invalid nodes list. */
	if (nodes == NULL)
		return (-EINVAL);

	/* Invalid number of nodes. */
	if (!WITHIN(nnodes, 2, (PROCESSOR_NOC_NODES_NUM + 1)))
		return(-EINVAL);

	/* Invalid sync type. */
	if ((type != SYNC_ONE_TO_ALL) && (type != SYNC_ALL_TO_ONE))
		return (-EINVAL);

	/* Gets the local node number. */
	nodenum = processor_node_get_num(core_get_id());

	ret = kcall4(
		NR_sync_create,
		(word_t) nodes,
		(word_t) nnodes,
		(word_t) type,
		(word_t) nodenum
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
	int nodenum;

	/* Invalid list of nodes. */
	if (nodes == NULL)
		return (-EINVAL);

	/* Invalid number of nodes. */
	if (!WITHIN(nnodes, 2, (PROCESSOR_NOC_NODES_NUM + 1)))
		return(-EINVAL);

	/* Invalid sync type. */
	if ((type != SYNC_ONE_TO_ALL) && (type != SYNC_ALL_TO_ONE))
		return (-EINVAL);

	/* Gets the local node number. */
	nodenum = processor_node_get_num(core_get_id());

	ret = kcall4(
		NR_sync_open,
		(word_t) nodes,
		(word_t) nnodes,
		(word_t) type,
		(word_t) nodenum
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
