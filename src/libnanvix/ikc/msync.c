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

#define __NEED_RESOURCE
#define __NANVIX_MICROKERNEL

#include <nanvix/kernel/kernel.h>

#if __TARGET_HAS_MAILBOX && __NANVIX_IKC_USES_ONLY_MAILBOX

#include <nanvix/sys/perf.h>
#include <nanvix/sys/noc.h>
#include <nanvix/sys/mailbox.h>
#include <posix/errno.h>

/**
 * @brief Indicates if ksync_init is called.
 */
PRIVATE bool ksync_is_initialized = false;

/**
 * @name Protections.
 */
/**@{*/
PRIVATE spinlock_t global_lock = SPINLOCK_UNLOCKED;
PRIVATE spinlock_t wait_lock   = SPINLOCK_UNLOCKED;
PRIVATE spinlock_t signal_lock = SPINLOCK_UNLOCKED;
/**@}*/

/**
 * @name Mailbox channels.
 */
/**@{*/
PRIVATE int inbox = -1;
PRIVATE int outboxes[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = -1,
};
/**@}*/

/*============================================================================*
 * Counters structure.                                                        *
 *============================================================================*/

/**
 * @brief Communicator counters.
 */
PRIVATE struct
{
	uint64_t ncreates; /**< Number of creates. */
	uint64_t nunlinks; /**< Number of unlinks. */
	uint64_t nopens;   /**< Number of opens.   */
	uint64_t ncloses;  /**< Number of closes.  */
	uint64_t nwaits;   /**< Number of watis.   */
	uint64_t nsignals; /**< Number of signals. */
} msync_counters;

/*============================================================================*
 * Sync hash                                                                  *
 *============================================================================*/

/**
 * @name Hash macros.
 */
/**@{*/
#define MSYNC_HASH_SIZE (sizeof(struct msync_hash))
#define MSYNC_HASH_NULL ((struct msync_hash){-1, 0, -1, 0, 0, 0})
/**@}*/

/**
 * @brief Hash structure.
 */
struct msync_hash
{
	uint64_t source    :  5; /**< Source nodenum.   */
	uint64_t type      :  1; /**< Type of the sync. */
	uint64_t master    :  5; /**< Master nodenum.   */
	uint64_t nodeslist : 24; /**< Nodes list.       */
	uint64_t barrier   : 24; /**< Barrier variable. */
	uint64_t unused    :  5; /**< Unused.           */
};

/*============================================================================*
 * Sync Structures.                                                           *
 *============================================================================*/

/**
 * @brief Table of active synchronization points.
 */
PRIVATE struct msync
{
	/*
	 * XXX: Don't Touch! This Must Come First!
	 */
	struct resource resource;               /**< Generic resource information. */
	int refcount;                           /**< Counter of references.        */
	int nbarriers;                          /**< Counter of barriers.          */
	struct msync_hash hash;                 /**< Sync hash.                    */
	int nreceived[PROCESSOR_NOC_NODES_NUM]; /**< Number of signals received.   */
	uint64_t latency;                       /**< Latency counter.              */
} ALIGN(sizeof(dword_t)) msyncs[(KSYNC_MAX)] = {
	[0 ... (KSYNC_MAX - 1)] = {
		.resource  = {0, },
		.refcount  = 0,
		.nbarriers = 0,
		.hash      = {-1, 0, -1, 0, 0, 0},
		.latency   = 0ULL,
	},
};

/**
 * @brief Resource pool.
 */
PRIVATE const struct resource_pool msyncpool = {
	msyncs, (KSYNC_MAX), sizeof(struct msync)
};

/*============================================================================*
 * ksync_build_nodeslist()                                                    *
 *============================================================================*/

PRIVATE uint64_t ksync_build_nodeslist(const int * nodes, int nnodes)
{
	uint64_t nodeslist = 0ULL; /* Bit-stream of nodes. */

	for (int j = 0; j < nnodes; j++)
		nodeslist |= (1ULL << nodes[j]);

	return (nodeslist);
}

/*============================================================================*
 * ksync_search()                                                             *
 *============================================================================*/

PRIVATE int ksync_search(struct msync_hash * hash, int input)
{
	for (unsigned i = 0; i < (KSYNC_MAX); ++i)
	{
		if (!resource_is_used(&msyncs[i].resource))
			continue;

		if (input)
		{
			if (!resource_is_readable(&msyncs[i].resource))
				continue;
		}

		/* Output. */
		else if (!resource_is_writable(&msyncs[i].resource))
			continue;

		if (msyncs[i].hash.type != hash->type)
			continue;

		if (msyncs[i].hash.master != hash->master)
			continue;

		if (msyncs[i].hash.nodeslist != hash->nodeslist)
			continue;

		return (i);
	}

	return (-EINVAL);
}

/*============================================================================*
 * ksync_nodelist_is_valid()                                                  *
 *============================================================================*/

/**
 * @brief Node list validation.
 *
 * @param local  Logic ID of local node.
 * @param nodes  IDs of target NoC nodes.
 * @param nnodes Number of target NoC nodes.
 *
 * @return Non zero if node list is valid and zero otherwise.
 */
PRIVATE int ksync_nodelist_is_valid(const int * nodes, int nnodes, int is_the_one)
{
	int local;       /* Local node.          */
	uint64_t checks; /* Bit-stream of nodes. */

	checks = 0ULL;
	local  = knode_get_num();

	/* Is the local the one? */
	if (is_the_one && (nodes[0] != local))
		return (0);

	/* Isn't the local the one? */
	if (!is_the_one && (nodes[0] == local))
		return (0);

	/* Build nodelist. */
	for (int i = 0; i < nnodes; ++i)
	{
		/* Invalid node. */
		if (!WITHIN(nodes[i], 0, PROCESSOR_NOC_NODES_NUM))
			return (0);

		/* Does a node appear twice? */
		if (checks & (1ULL << nodes[i]))
			return (0);

		checks |= (1ULL << nodes[i]);
	}

	/* Is the local node founded? */
	return (checks & (1ULL << local));
}

/*============================================================================*
 * ksync_create()                                                             *
 *============================================================================*/

/**
 * @details The ksync_create() function creates an input sync
 * and attaches it to the list NoC node @p nodes.
 */
PRIVATE int do_ksync_alloc(const int * nodes, int nnodes, int type, int input)
{
	int syncid;             /* Synchronization point.            */
	int is_the_one;         /* Indicates rule of the local node. */
	struct msync_hash hash; /* Sync hash.                        */

	KASSERT(ksync_is_initialized);

	/* Invalid nodes list. */
	if (nodes == NULL)
		return (-EINVAL);

	/* Invalid number of nodes. */
	if (!WITHIN(nnodes, 2, (PROCESSOR_NOC_NODES_NUM + 1)))
		return(-EINVAL);

	/* Invalid sync type. */
	if ((type != SYNC_ONE_TO_ALL) && (type != SYNC_ALL_TO_ONE))
		return (-EINVAL);

	is_the_one = input ? (type == SYNC_ALL_TO_ONE) : (type == SYNC_ONE_TO_ALL);

	/* Invalid nodes list. */
	if (!ksync_nodelist_is_valid(nodes, nnodes, is_the_one))
		return (-EINVAL);

	hash.source    = knode_get_num();
	hash.type      = type;
	hash.master    = nodes[0];
	hash.nodeslist = ksync_build_nodeslist(nodes, nnodes);

	if (input)
		hash.barrier = 0;
	else
	{
		/* Who should it sent to? */
		hash.barrier = (type == SYNC_ONE_TO_ALL)   ?
			(hash.nodeslist & ~(1 << hash.master)) : /**< Master notifies All. */
			(hash.nodeslist & (1 << hash.master));   /**< All notify Master.   */
	}

	spinlock_lock(&global_lock);

		/* Previous alloc. */
		if ((syncid = ksync_search(&hash, input)) >= 0)
			msyncs[syncid].refcount++;

		/* First alloc. */
		else
		{
			/* Allocate a msync point. */
			if ((syncid = resource_alloc(&msyncpool)) >= 0)
			{
				msyncs[syncid].refcount  = 1;
				msyncs[syncid].nbarriers = 0;
				msyncs[syncid].hash      = hash;
				msyncs[syncid].latency   = 0ULL;
				kmemset(msyncs[syncid].nreceived, 0, PROCESSOR_NOC_NODES_NUM * sizeof(int));

				if (input)
				{
					resource_set_rdonly(&msyncs[syncid].resource);
					msync_counters.ncreates++;
				}
				else
				{
					resource_set_wronly(&msyncs[syncid].resource);
					msync_counters.nopens++;
				}
			}
		}

	spinlock_unlock(&global_lock);

	return (syncid);
}
/*============================================================================*
 * ksync_create()                                                             *
 *============================================================================*/

/**
 * @details The ksync_create() function creates an input sync
 * and attaches it to the list NoC node @p nodes.
 */
PUBLIC int ksync_create(const int * nodes, int nnodes, int type)
{
	return (do_ksync_alloc(nodes, nnodes, type, true));
}

/*============================================================================*
 * ksync_open()                                                               *
 *============================================================================*/

/**
 * @details The ksync_open() function opens an output sync to the
 * NoC node list @p nodes.
 */
PUBLIC int ksync_open(const int * nodes, int nnodes, int type)
{
	return (do_ksync_alloc(nodes, nnodes, type, false));
}

/*============================================================================*
 * do_ksync_release()                                                         *
 *============================================================================*/

/**
 * @details The do_ksync_release() function release @p syncid.
 */
PRIVATE int do_ksync_release(int syncid, int input)
{
	int ret; /* Return value. */

	/* Invalid syncid. */
	if (!WITHIN(syncid, 0, KSYNC_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		if (!resource_is_used(&msyncs[syncid].resource))
			goto error;

		if (input)
		{
			if (!resource_is_readable(&msyncs[syncid].resource))
				goto error;
		}

		/* Output. */
		else if (!resource_is_writable(&msyncs[syncid].resource))
			goto error;

		ret = (-EBUSY);

		if (resource_is_busy(&msyncs[syncid].resource))
			goto error;

		/* Decrement references. */
		msyncs[syncid].refcount--;

		if (!msyncs[syncid].refcount)
		{
			resource_free(&msyncpool, syncid);
			msyncs[syncid].hash = MSYNC_HASH_NULL;
		}

		if (input)
			msync_counters.nunlinks++;
		else
			msync_counters.ncloses++;

		ret = (0);

error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * ksync_unlink()                                                             *
 *============================================================================*/

/**
 * @details The ksync_unlink() function removes and releases the underlying
 * resources associated to the input sync @p syncid.
 */
PUBLIC int ksync_unlink(int syncid)
{
	return (do_ksync_release(syncid, true));
}

/*============================================================================*
 * ksync_wait()                                                               *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * ksync_close()                                                              *
 *----------------------------------------------------------------------------*/

/**
 * @details The ksync_close() function closes and releases the
 * underlying resources associated to the output sync @p syncid.
 */
PUBLIC int ksync_close(int syncid)
{
	return (do_ksync_release(syncid, false));
}

/*----------------------------------------------------------------------------*
 * ksync_ignore_signal()                                                      *
 *----------------------------------------------------------------------------*/

PRIVATE void ksync_ignore_signal(char * message, struct msync_hash * hash)
{
	int source    = hash->source;
	int type      = hash->type;
	int master    = hash->master;
	int nodeslist = hash->nodeslist;

	kprintf("[sync] Dropping signal: %s | hash = (source:%d, type:%d, master:%d, nodeslist:%d)",
		message,
		source,
		type,
		master,
		nodeslist
	);
}

/*----------------------------------------------------------------------------*
 * ksync_barrier_is_complete()                                                *
 *----------------------------------------------------------------------------*/

PRIVATE int ksync_barrier_is_complete(struct msync * rx)
{
	uint64_t received; /* Received signals. */
	uint64_t expected; /* Expected signals. */

	received = rx->hash.barrier;

	/* Does master notifies it? */
	if (rx->hash.type == SYNC_ONE_TO_ALL)
		expected = (rx->hash.nodeslist & (1ULL << rx->hash.master));

	/* Does slaves notifies it? */
	else
		expected = (rx->hash.nodeslist & ~(1ULL << rx->hash.master));

	return (received == expected);
}

/*============================================================================*
 * ksync_barrier_reset()                                                      *
 *============================================================================*/

PRIVATE void ksync_barrier_reset(struct msync * rx)
{
	uint64_t mask;
	for (uint64_t i = 0; i < PROCESSOR_NOC_NODES_NUM; ++i)
	{
		mask = (1ULL << i);

		if (rx->hash.barrier & mask)
		{
			/**
			 * Consume a signals and reset barrier if there are no
			 * signals from that node.
			 **/
			rx->nreceived[i] = (rx->nreceived[i] - 1);
			if (rx->nreceived[i] == 0)
				rx->hash.barrier &= (~mask);
		}
	}
}

/*============================================================================*
 * ksync_barrier_consume()                                                    *
 *============================================================================*/

PRIVATE int ksync_barrier_consume(struct msync * sync)
{
	int consumed; /* Indicates if the barrier is consumed. */

	spinlock_lock(&global_lock);

		consumed = sync->nbarriers;

		/* Is the barrier complete? */
		if (consumed)
			sync->nbarriers = (sync->nbarriers - 1);

	spinlock_unlock(&global_lock);

	return (consumed);
}

/*----------------------------------------------------------------------------*
 * do_ksync_wait()                                                            *
 *----------------------------------------------------------------------------*/

PRIVATE int do_ksync_wait(struct msync * sync)
{
	int syncid;             /* Synchronization point. */
	struct msync_hash hash; /* Hash buffer.           */

	/* Is the previous wait released me? */
	if (ksync_barrier_consume(sync))
		return (0);

	spinlock_lock(&wait_lock);

		/* Is other core released me? */
		if (ksync_barrier_consume(sync))
		{
			spinlock_unlock(&wait_lock);
			return (0);
		}

		/* Reads a signal. */ 
		if (kmailbox_read(inbox, &hash, MSYNC_HASH_SIZE) != MSYNC_HASH_SIZE)
		{
			spinlock_unlock(&wait_lock);
			return (-EAGAIN);
		}

		spinlock_lock(&global_lock);

			if (!node_is_valid(hash.source))
			{
				ksync_ignore_signal("Invalid Source", &hash);
				goto release;
			}

			if ((syncid = ksync_search(&hash, true)) < 0)
			{
				ksync_ignore_signal("Sync point not found.", &hash);
				goto release;
			}

			msyncs[syncid].hash.barrier |= (1ULL << hash.source);
			msyncs[syncid].nreceived[hash.source]++;

			if (ksync_barrier_is_complete(&msyncs[syncid]))
			{
				ksync_barrier_reset(&msyncs[syncid]);
				msyncs[syncid].nbarriers++;
			}

release:
		spinlock_unlock(&global_lock);
	spinlock_unlock(&wait_lock);

	/* Do again. */
	return (1);
}

/*----------------------------------------------------------------------------*
 * ksync_wait()                                                               *
 *----------------------------------------------------------------------------*/

/**
 * @details The ksync_wait() waits incomming signal on a input sync @p syncid.
 */
PUBLIC int ksync_wait(int syncid)
{
	int ret;     /* Return value. */
	uint64_t t0; /* Clock value.  */
	uint64_t t1; /* Clock value.  */

	/* Invalid syncid. */
	if (!WITHIN(syncid, 0, KSYNC_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&msyncs[syncid].resource))
			goto error;

		/* Bad sync. */
		if (!resource_is_readable(&msyncs[syncid].resource))
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&msyncs[syncid].resource))
			goto error;

		resource_set_busy(&msyncs[syncid].resource);

	spinlock_unlock(&global_lock);

	kclock(&t0);
		while ((ret = do_ksync_wait(&msyncs[syncid])) > 0);
	kclock(&t1);

	spinlock_lock(&global_lock);
		if (ret >= 0)
		{
			msyncs[syncid].latency += (t1 - t0);
			msync_counters.nwaits++;
		}
		resource_set_notbusy(&msyncs[syncid].resource);
error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * ksync_signal()                                                             *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * do_ksync_signal()                                                          *
 *----------------------------------------------------------------------------*/

PRIVATE int do_ksync_signal(int syncid)
{
	int ret; /* Return value. */

	ret = (-EINVAL);

	spinlock_lock(&signal_lock);

		/* Sends a signal to each target node. */
		for (unsigned target = 0; target < PROCESSOR_NOC_NODES_NUM; ++target)
		{
			/* Is the target valid? */
			if (msyncs[syncid].hash.barrier & (1 << target))
			{
				ret = kmailbox_write(
					outboxes[target],
					&msyncs[syncid].hash,
					MSYNC_HASH_SIZE
				);

				/* Error occurred? */
				if (ret != MSYNC_HASH_SIZE)
					break;
			}
		}

	spinlock_unlock(&signal_lock);

	return (ret);
}

/*----------------------------------------------------------------------------*
 * ksync_signal()                                                             *
 *----------------------------------------------------------------------------*/

/**
 * @details The ksync_signal() emmit a signal from a output sync @p syncid.
 */
PUBLIC int ksync_signal(int syncid)
{
	int ret;     /* Return value. */
	uint64_t t0; /* Clock value.  */
	uint64_t t1; /* Clock value.  */

	/* Invalid syncid. */
	if (!WITHIN(syncid, 0, KSYNC_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&msyncs[syncid].resource))
			goto error;

		/* Bad sync. */
		if (!resource_is_writable(&msyncs[syncid].resource))
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&msyncs[syncid].resource))
			goto error;

		resource_set_busy(&msyncs[syncid].resource);

	spinlock_unlock(&global_lock);

	kclock(&t0);
		ret = do_ksync_signal(syncid);
	kclock(&t1);

	spinlock_lock(&global_lock);
		if (ret >= 0)
		{
			msyncs[syncid].latency += (t1 - t0);
			msync_counters.nsignals++;
		}
		resource_set_notbusy(&msyncs[syncid].resource);
error:
	spinlock_unlock(&global_lock);

	return (ret < 0) ? (ret) : (0);
}

/*============================================================================*
 * ksync_ioctl()                                                            *
 *============================================================================*/

PRIVATE int ksync_ioctl_valid(void * ptr, size_t size)
{
	return ((ptr != NULL) && mm_check_area(VADDR(ptr), size, UMEM_AREA));
}

/**
 * @details The ksync_ioctl() reads the measurement parameter associated
 * with the request id @p request of the sync @p syncid.
 */
PUBLIC int ksync_ioctl(int syncid, unsigned request, ...)
{
	int ret;        /* Return value.              */
	va_list args;   /* Argument list.             */
	uint64_t * var; /* Auxiliar variable pointer. */

	if (!WITHIN(syncid, 0, KSYNC_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&msyncs[syncid].resource))
			goto error0;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&msyncs[syncid].resource))
			goto error0;

		ret = (-EFAULT);

		va_start(args, request);

			var = va_arg(args, uint64_t *);

			/* Bad buffer. */
			if (!ksync_ioctl_valid(var, sizeof(uint64_t)))
				goto error1;

			ret = 0;

			/* Parse request. */
			switch (request)
			{
				case KSYNC_IOCTL_GET_LATENCY:
					*var = msyncs[syncid].latency;
					break;

				case KSYNC_IOCTL_GET_NCREATES:
					*var = msync_counters.ncreates;
					break;

				case KSYNC_IOCTL_GET_NUNLINKS:
					*var = msync_counters.nunlinks;
					break;

				case KSYNC_IOCTL_GET_NOPENS:
					*var = msync_counters.nopens;
					break;

				case KSYNC_IOCTL_GET_NCLOSES:
					*var = msync_counters.ncloses;
					break;

				case KSYNC_IOCTL_GET_NWAITS:
					*var = msync_counters.nwaits;
					break;

				case KSYNC_IOCTL_GET_NSIGNALS:
					*var = msync_counters.nsignals;
					break;

				/* Operation not supported. */
				default:
					ret = (-ENOTSUP);
					break;
			}

error1:
		va_end(args);
error0:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * ksync_init()                                                               *
 *============================================================================*/

/**
 * @details The ksync_init() Initializes underlying mailboxes.
 */
PUBLIC void ksync_init(void)
{
	int local; /* Local nodenum. */

	kprintf("[user][sync] Initializes sync module (mailbox implementation)");

	local = knode_get_num();

	msync_counters.ncreates = 0ULL;
	msync_counters.nunlinks = 0ULL;
	msync_counters.nopens   = 0ULL;
	msync_counters.ncloses  = 0ULL;
	msync_counters.nwaits   = 0ULL;
	msync_counters.nsignals = 0ULL;

	for (unsigned i = 0; i < KSYNC_MAX; i++)
	{
		msyncs[i].resource  = RESOURCE_INITIALIZER;
		msyncs[i].refcount  = 0;
		msyncs[i].nbarriers = 0;
		msyncs[i].hash.source = -1;
		msyncs[i].latency   = 0ULL;

		for (unsigned j = 0; j < PROCESSOR_NOC_NODES_NUM; j++)
			msyncs[i].nreceived[j] = 0;
	}

	/* Create input mailbox. */
	KASSERT(
		(inbox = kcall2(
			NR_mailbox_create,
			(word_t) local,
			(word_t) (MAILBOX_PORT_NR - 1)
		)) >= 0
	);

	/* Opens output mailboxes. */
	for (unsigned i = 0; i < PROCESSOR_NOC_NODES_NUM; ++i)
	{
		KASSERT(
			(outboxes[i] = kcall2(
				NR_mailbox_open,
				(word_t) i,
				(word_t) (MAILBOX_PORT_NR - 1)
			)) >= 0
		);
	}

	ksync_is_initialized = true;
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_MAILBOX && __NANVIX_IKC_USES_ONLY_MAILBOX */
