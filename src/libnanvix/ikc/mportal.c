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
#include <nanvix/sys/portal.h>
#include <posix/errno.h>

/**
 * @brief Indicates if kportal_init is called.
 */
PRIVATE bool kportal_is_initialized = false;

/**
 * @name Protections.
 */
/**@{*/
PRIVATE spinlock_t global_lock            = SPINLOCK_UNLOCKED;
PRIVATE spinlock_t local_lock             = SPINLOCK_UNLOCKED;
PRIVATE spinlock_t allow_lock             = SPINLOCK_UNLOCKED;
PRIVATE spinlock_t buffer_lock            = SPINLOCK_UNLOCKED;
PRIVATE spinlock_t read_lock[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = SPINLOCK_UNLOCKED
};
PRIVATE spinlock_t write_lock[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = SPINLOCK_UNLOCKED
};
/**@}*/

/**
 * @name Mailbox channels.
 */
/**@{*/
PRIVATE int mallow_in = -1;
PRIVATE int mallow_outs[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = -1,
};
PRIVATE int mdata_ins[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = -1,
};
PRIVATE int mdata_outs[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = -1,
};
/**@}*/

/**
 * @name Fair use of read channels.
 */
/**@{*/
PRIVATE struct resource read_channels[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = {0, }
};
PRIVATE struct resource write_channels[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = {0, }
};
/**@}*/

/**
 * @brief Write control.
 */
PRIVATE bool remote_is_allowed[PROCESSOR_NOC_NODES_NUM] = {
	[0 ... (PROCESSOR_NOC_NODES_NUM - 1)] = false
};

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
	uint64_t nreads;   /**< Number of reads.   */
	uint64_t nwrites;  /**< Number of writes.  */
} mportal_counters;

/*============================================================================*
 * Portal configuration                                                       *
 *============================================================================*/

/**
 * @name Configuration macros.
 */
/**@{*/
#define MPORTAL_CONFIG_SIZE (sizeof(struct mportal_config))           /**< Size of config struct. */
#define MPORTAL_CONFIG_NULL ((struct mportal_config){-1, -1, -1, -1}) /**< Invalid configuration. */
/**@}*/

/**
 * @brief Configuration structure.
 */
struct mportal_config
{
	int local;       /**< Local nodenum.  */
	int local_port;  /**< Local port id.  */
	int remote;      /**< Remote nodenum. */
	int remote_port; /**< Remote port id. */
};

/*============================================================================*
 * Portal structure                                                           *
 *============================================================================*/

/**
 * @brief Table of portals.
 */
PRIVATE struct mportal
{
	/*
	 * XXX: Don't Touch! This Must Come First!
	 */
	struct resource resource;     /**< Generic resource information.    */
	int refcount;                 /**< Counter of references (Avoided). */

	/**
	 * @name Communication.
	 */
	/**@{*/
	int mallow;                   /**< Mailbox channel to allow.        */
	int mdata;                    /**< Mailbox channel to data.         */
	struct mportal_config config; /**< Configuration.                   */
	/**@}*/

	/**
	 * @name Statistics.
	 */
	/**@{*/
	size_t volume;                /**< Volume.                          */
	uint64_t latency;             /**< Latency.                         */
	/**@}*/
} ALIGN(sizeof(dword_t)) mportals[KPORTAL_MAX] = {
	[0 ... (KPORTAL_MAX - 1)] = {
		.resource  = {0, },
		.refcount  = 0,
		.mallow    = -1,
		.mdata     = -1,
		.volume    = 0ULL,
		.latency   = 0ULL,
		.config    = {-1, -1, -1, -1},
	},
};

/**
 * @brief Portal pool.
 */
PRIVATE const struct resource_pool mportalpool = {
	mportals, (KPORTAL_MAX), sizeof(struct mportal)
};

/*============================================================================*
 * Portal message                                                             *
 *============================================================================*/

/**
 * @name Portal message macros.
 */
/**@{*/
#define MPORTAL_MESSAGE_SIZE      (sizeof(struct mportal_message)) /**< Size of config struct. */
#define MPORTAL_MESSAGE_DATA_SIZE (KMAILBOX_MESSAGE_SIZE - 6)      /**< Max data size.         */
/**@}*/

/**
 * @brief Portal message structure.
 */
struct mportal_message
{
	/**
	 * @name Control.
	 */
	/**@{*/
	char header : 1; /**< Indicates if contains the a valid config. */
	char eof    : 1; /**< Indicates if is the last message.         */
	char unused : 6; /**< Unused.                                   */
	char size;       /**< Data size.                                */
	/**@}*/

	/**
	 * @name Data.
	 */
	/**@{*/
	union
	{
		struct mportal_config config;         /**< Configuration.   */
		char data[MPORTAL_MESSAGE_DATA_SIZE]; /**< Data buffer.     */
	} _;             /**< Abstract union.                           */
	/**@}*/
};

/*============================================================================*
 * Portal Port (TX only)                                                      *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * Structure and variables                                                    *
 *----------------------------------------------------------------------------*/

/**
 * @brief Portal port structure.
 */
PRIVATE struct mportal_port
{
	/*
	 * XXX: Don't Touch! This Must Come First!
	 */
	struct resource resource; /**< Generic resource information. */
} mports[HAL_PORTAL_OPEN_MAX][KPORTAL_PORT_NR] = {
	[0 ... (HAL_PORTAL_OPEN_MAX - 1)] = {
		[0 ... (KPORTAL_PORT_NR - 1)] = {
			.resource = {0, },
		},
	},
};

/*----------------------------------------------------------------------------*
 * kportal_port_alloc()                                                       *
 *----------------------------------------------------------------------------*/

PRIVATE int kportal_port_alloc(int remote)
{
	for (unsigned i = 0; i < KPORTAL_PORT_NR; ++i)
	{
		if (!resource_is_used(&mports[remote][i].resource))
		{
			resource_set_used(&mports[remote][i].resource);
			return (i);
		}
	}

	return (-EBUSY);
}

/*----------------------------------------------------------------------------*
 * kportal_port_release()                                                     *
 *----------------------------------------------------------------------------*/

PRIVATE int kportal_port_release(int remote, int portid)
{
	KASSERT(WITHIN(portid, 0, KPORTAL_PORT_NR));
	KASSERT(resource_is_used(&mports[remote][portid].resource));
	resource_set_unused(&mports[remote][portid].resource);
	return (0);
}

/*============================================================================*
 * kportal_search()                                                           *
 *============================================================================*/

PRIVATE void kportal_print_message(char * message, struct mportal_config * config)
{
	if (!config)
		kprintf("[portal] %s", message);
	else
	{
		kprintf("[portal] %s (local:%d, local_port:%d, remote:%d, remote_port:%d)",
			message,
			config->local,
			config->local_port,
			config->remote,
			config->remote_port
		);
	}
}

/*============================================================================*
 * Portal Buffer                                                              *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * Structures and variables                                                   *
 *----------------------------------------------------------------------------*/

/**
 * @name Portal buffer macros.
 */
/**@{*/
#define MPORTAL_BUFFER_SIZE (KPORTAL_MESSAGE_DATA_SIZE) /**< Maximum buffer size.       */
#define MPORTAL_BUFFER_MAX  (40)                        /**< Maximum number of buffers. */
/**@}*/

/**
 * @brief Portal buffer structure.
 */
PRIVATE struct mportal_buffer
{
	/*
	 * XXX: Don't Touch! This Must Come First!
	 */
	struct resource resource;       /**< Generic resource information.    */

	/**
	 * @name Control.
	 */
	/**@{*/
	uint64_t age;                   /**< Buffer age.                      */
	uint64_t seq;                   /**< Sequence number.                 */
	size_t size;                    /**< Valid data size.                 */
	struct mportal_config config;   /**< Configuration                    */
	struct mportal_buffer * next;   /**< Next buffer of the sequence.     */
	/**@}*/

	/**
	 * @brief Data.
	 */
	char data[MPORTAL_BUFFER_SIZE]; /**< Data buffer.                     */
} mpbuffers[MPORTAL_BUFFER_MAX] = {
	[0 ... (MPORTAL_BUFFER_MAX - 1)] = {
		.resource = {0, },
		.age      = ~(0ULL),
		.seq      = ~(0ULL),
		.size     = 0,
		.next     = NULL,
		.config   = {-1, -1, -1, -1},
		.data     = {0, },
	},
};

/**
 * @brief Age counter of the portal buffers.
 */
PRIVATE uint64_t mbuffer_next_age = 0ULL;

/*----------------------------------------------------------------------------*
 * kportal_buffer_alloc()                                                     *
 *----------------------------------------------------------------------------*/

PRIVATE struct mportal_buffer * kportal_buffer_alloc(
	struct mportal_buffer * previous,
	struct mportal_config * config
)
{
	struct mportal_buffer * buf; /* Auxiliar buffer pointer. */

	buf = NULL;

	if (config == NULL)
		return (NULL);

	spinlock_lock(&buffer_lock);

		for (size_t i = 0; i < MPORTAL_BUFFER_MAX; ++i)
		{
			if (resource_is_used(&mpbuffers[i].resource))
				continue;

			buf = &mpbuffers[i];

			if (previous == NULL)
				buf->seq = 0ULL;
			else
			{
				buf->seq       = previous->seq + 1;
				previous->next = buf;
				resource_set_notbusy(&previous->resource);
			}

			buf->config = *config;
			buf->size   = 0ULL;
			buf->next   = NULL;
			buf->age    = mbuffer_next_age++;
			resource_set_used(&buf->resource);
			resource_set_busy(&buf->resource);

			break;
		}

	spinlock_unlock(&buffer_lock);

	return (buf);
}

/*----------------------------------------------------------------------------*
 * do_kportal_buffer_release()                                                *
 *----------------------------------------------------------------------------*/

PRIVATE struct mportal_buffer * do_kportal_buffer_release(struct mportal_buffer * buf)
{
	struct mportal_buffer * next; /* Auxiliar buffer pointer. */

	if (buf == NULL)
		return (NULL);

	if (!resource_is_used(&buf->resource))
		return (NULL);

	next = buf->next;

	buf->config = MPORTAL_CONFIG_NULL;
	buf->seq    = ~(0ULL);
	buf->size   = 0ULL;
	buf->next   = NULL;
	buf->age    = ~(0ULL);
	resource_set_unused(&buf->resource);

	return (next);
}

/*----------------------------------------------------------------------------*
 * kportal_buffer_release()                                                   *
 *----------------------------------------------------------------------------*/

PRIVATE void kportal_buffer_set_available(struct mportal_buffer * buf)
{
	if (buf)
	{
		spinlock_lock(&buffer_lock);
			resource_set_notbusy(&buf->resource);
		spinlock_unlock(&buffer_lock);
	}
}

/*----------------------------------------------------------------------------*
 * kportal_buffer_release()                                                   *
 *----------------------------------------------------------------------------*/

PRIVATE struct mportal_buffer * kportal_buffer_release(struct mportal_buffer * buf)
{
	struct mportal_buffer * next; /* Auxiliar buffer pointer. */

	if (buf == NULL)
		return (NULL);

	spinlock_lock(&buffer_lock);
		next = do_kportal_buffer_release(buf);
	spinlock_unlock(&buffer_lock);

	return (next);
}

/*----------------------------------------------------------------------------*
 * do_kportal_buffer_search()                                                 *
 *----------------------------------------------------------------------------*/

PRIVATE struct mportal_buffer * do_kportal_buffer_search(struct mportal * portal)
{
	struct mportal_buffer * buf; /* Auxiliar buffer pointer. */

	if (!node_is_local(portal->config.local))
		kportal_print_message("kportal_buffer_search failed", &portal->config);

	/* Sanity checks. */
	KASSERT(node_is_local(portal->config.local));

	buf = NULL;

	for (size_t i = 0; i < MPORTAL_BUFFER_MAX; ++i)
	{
		if (!resource_is_used(&mpbuffers[i].resource))
			continue;

		if (portal->config.local != mpbuffers[i].config.remote)
			continue;

		if (portal->config.local_port != mpbuffers[i].config.remote_port)
			continue;

		/* Is it a read operation? */
		if (portal->config.remote != -1)
		{
			if (portal->config.remote != mpbuffers[i].config.local)
				continue;

			if (portal->config.remote_port != mpbuffers[i].config.local_port)
				continue;

			if (mpbuffers[i].seq != 0)
				continue;

			if (buf != NULL && buf->age < mpbuffers[i].age)
				continue;

			if (resource_is_busy(&mpbuffers[i].resource))
				continue;
		}

		buf = &mpbuffers[i];

		/* Is it a find the first operation? */
		if (portal->config.remote == -1)
			break;
	}

	return (buf);
}

/*----------------------------------------------------------------------------*
 * kportal_buffer_search()                                                    *
 *----------------------------------------------------------------------------*/

PRIVATE struct mportal_buffer * kportal_buffer_search(struct mportal * portal)
{
	struct mportal_buffer * buf; /* Auxiliar buffer pointer. */

	if (portal == NULL)
		return (NULL);

	spinlock_lock(&buffer_lock);
		buf = do_kportal_buffer_search(portal);
	spinlock_unlock(&buffer_lock);

	return (buf);
}

/*----------------------------------------------------------------------------*
 * kportal_buffer_read()                                                      *
 *----------------------------------------------------------------------------*/

PRIVATE ssize_t kportal_buffer_read(
	struct mportal * portal,
	void ** buffer,
	size_t * remainder,
	struct mportal_buffer ** previous
)
{
	uint64_t t0;                 /* Clock value.             */
	uint64_t t1;                 /* Clock value.             */
	size_t copied;               /* Amount of data copied.   */
	struct mportal_buffer * buf; /* Auxiliar buffer pointer. */

	copied = 0ULL;
	buf    = NULL;

	if (buffer == NULL || *buffer == NULL)
		return (-EINVAL);

	spinlock_lock(&buffer_lock);

		buf = (*previous) ? (*previous)->next : do_kportal_buffer_search(portal);

		while (buf)
		{
			if (resource_is_busy(&buf->resource))
				break;

			/* Sanity check. */
			if (buf->size > *remainder)
			{
				/**
				 * Discard entire message because it lost the previous ones.
				 * TODO: Recover the message from "copied" buffer.
				 **/
				if (buf->seq > 0)
				{
					kportal_print_message("kportal_buffer_read failed: Discarting buffers...", &buf->config);

					while (do_kportal_buffer_release(buf));

					if (*previous)
					{
						do_kportal_buffer_release(*previous);
						*previous = NULL;
					}
				}

				copied = (-EINVAL);
				goto error;
			}

			/* Copy the message. */
			kclock(&t0);
				kmemcpy(*buffer, buf->data, buf->size);
			kclock(&t1);
			portal->latency += (t1 - t0);

			/* Updates parameters. */
			copied     += buf->size;
			*buffer    += buf->size;
			*remainder -= buf->size;

			/* Updates previous buffer. */
			*previous = (*previous) ? do_kportal_buffer_release(*previous) : buf;

			/* Get next buffer. */
			buf = (*previous)->next;
		}

		if ((*remainder == 0) && *previous)
		{
			do_kportal_buffer_release(*previous);
			*previous = NULL;
		}

error:
	spinlock_unlock(&buffer_lock);

	return (copied);
}

/*============================================================================*
 * kportal_search()                                                           *
 *============================================================================*/

PRIVATE int kportal_search(struct mportal_config * config, int input)
{
	if (!node_is_local(config->local))
		kportal_print_message("kportal_search failed", config);

	/* Sanity checks. */
	KASSERT(node_is_local(config->local));

	for (unsigned i = 0; i < KPORTAL_MAX; ++i)
	{
		if (!resource_is_used(&mportals[i].resource))
			continue;

		if (input)
		{
			if (!resource_is_readable(&mportals[i].resource))
				continue;
		}

		/* Output. */
		else if (!resource_is_writable(&mportals[i].resource))
			continue;

		if (mportals[i].config.local != config->local)
			continue;

		if (mportals[i].config.local_port != config->local_port)
			continue;

		if (!input)
		{
			if (mportals[i].config.remote != config->remote)
				continue;

			if (mportals[i].config.remote_port != config->remote_port)
				continue;
		}

		return (i);
	}

	return (-EINVAL);
}

/*============================================================================*
 * kportal_create()                                                           *
 *============================================================================*/

/**
 * @details The kportal_create() function creates an input portal and
 * attaches it to the local port @p local_port in the local NoC node @p
 * local.
 */
PUBLIC int kportal_create(int local, int local_port)
{
	int portalid;                 /* Portal ID.            */
	struct mportal_config config; /* Portal configuration. */

	KASSERT(kportal_is_initialized);

	/* Invalid local. */
	if (!node_is_valid(local))
		return (-EINVAL);

	/* Invalid local number for the requesting core ID. */
	if (!node_is_local(local))
		return (-EINVAL);

	/* Invalid portid. */
	if (!WITHIN(local_port, 0, KPORTAL_PORT_NR))
		return (-EINVAL);

	/* Configures portal. */
	config.local       = local;
	config.local_port  = local_port;
	config.remote      = -1;
	config.remote_port = -1;

	spinlock_lock(&global_lock);

		/**
		 * Previous create.
		 * TODO: Adds mportals[portalid].refcount++ to enable multiples
		 * creates of a same configuration.
		 **/
		if ((portalid = kportal_search(&config, true)) >= 0)
			portalid = (-EBUSY);

		/* First create. */
		else if ((portalid = resource_alloc(&mportalpool)) >= 0)
		{
			mportals[portalid].refcount = 1;
			mportals[portalid].volume   = 0ULL;
			mportals[portalid].latency  = 0ULL;
			mportals[portalid].config   = config;
			resource_set_rdonly(&mportals[portalid].resource);

			mportal_counters.ncreates++;
		}

	spinlock_unlock(&global_lock);

	return (portalid);
}

/*============================================================================*
 * kportal_allow()                                                            *
 *============================================================================*/

/**
 * @details The kportal_allow() function allow read data from a input portal
 * associated with the NoC node @p remote.
 */
PUBLIC int kportal_allow(int portalid, int remote, int remote_port)
{
	int ret; /* Return value. */

	ret = (-EINVAL);

	/* Invalid portalid. */
	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	/* Invalid local. */
	if (!node_is_valid(remote))
		return (-EINVAL);

	/* Invalid portid. */
	if (!WITHIN(remote_port, 0, KPORTAL_PORT_NR))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&mportals[portalid].resource))
			goto error;

		/* Bad sync. */
		if (!resource_is_readable(&mportals[portalid].resource))
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&mportals[portalid].resource))
			goto error;

		/* Already allowed. */
		if (mportals[portalid].config.remote != -1)
			goto error;

		mportals[portalid].mallow             = mallow_outs[remote];
		mportals[portalid].mdata              = mdata_ins[remote];
		mportals[portalid].config.remote      = remote;
		mportals[portalid].config.remote_port = remote_port;
		ret = (0);

error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * kportal_open()                                                             *
 *============================================================================*/

/**
 * @details The kportal_open() function opens an output portal to the remote
 * NoC node @p remote and attaches it to the local NoC node @p local.
 */
PUBLIC int kportal_open(int local, int remote, int remote_port)
{
	int portalid;                 /* Portal ID.            */
	int local_port;               /* Portal port.          */
	struct mportal_config config; /* Portal configuration. */

	KASSERT(kportal_is_initialized);

	/* Invalid local. */
	if (!node_is_valid(local))
		return (-EINVAL);

	/* Invalid remote. */
	if (!node_is_valid(remote))
		return (-EINVAL);

	/* Invalid local number for the requesting core ID. */
	if (!node_is_local(local))
		return (-EINVAL);

	/* Invalid portid. */
	if (!WITHIN(remote_port, 0, KPORTAL_PORT_NR))
		return (-EINVAL);

	/* Configures portal. */
	config.local       = local;
	config.remote      = remote;
	config.remote_port = remote_port;

	spinlock_lock(&global_lock);

		portalid = (-EBUSY);

		/* Alloc a local port. */
		if ((local_port = kportal_port_alloc(remote)) < 0)
			goto error;

		config.local_port  = local_port;

		/**
		 * Previous open.
		 * TODO: Adds mportals[portalid].refcount++ to enable multiples
		 * open of a same configuration.
		 **/
		if ((portalid = kportal_search(&config, false)) >= 0)
			portalid = (-EBUSY);

		/* First create. */
		else if ((portalid = resource_alloc(&mportalpool)) >= 0)
		{
			mportals[portalid].mallow   = mallow_in;
			mportals[portalid].mdata    = mdata_outs[remote];
			mportals[portalid].refcount = 1;
			mportals[portalid].volume   = 0;
			mportals[portalid].latency  = 0;
			mportals[portalid].config   = config;
			resource_set_wronly(&mportals[portalid].resource);

			mportal_counters.nopens++;
		}

error:
	spinlock_unlock(&global_lock);

	return (portalid);
}

/*============================================================================*
 * kportal_unlink()                                                           *
 *============================================================================*/

/**
 * @details The kportal_unlink() function removes and releases the underlying
 * resources associated to the input portal @p portalid.
 */
PUBLIC int kportal_unlink(int portalid)
{
	int ret; /* Return value. */

	/* Invalid portalid. */
	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&mportals[portalid].resource))
			goto error;

		/* Bad sync. */
		if (!resource_is_readable(&mportals[portalid].resource))
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&mportals[portalid].resource))
			goto error;

		/* Busy sync. */
		if (kportal_buffer_search(&mportals[portalid]) != NULL)
			goto error;

		/* Decrement references. */
		mportals[portalid].refcount--;

		/* Releases resource (for now, refcount always is 0 here). */
		if (!mportals[portalid].refcount)
		{
			mportals[portalid].mallow = -1;
			mportals[portalid].mdata  = -1;
			mportals[portalid].config = MPORTAL_CONFIG_NULL;
			resource_free(&mportalpool, portalid);
		}

		mportal_counters.nunlinks++;
		ret = (0);

error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * kportal_close()                                                            *
 *============================================================================*/

/**
 * @details The kportal_close() function closes and releases the
 * underlying resources associated to the output portal @p portalid.
 */
PUBLIC int kportal_close(int portalid)
{
	int ret; /* Return value. */

	/* Invalid portalid. */
	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&mportals[portalid].resource))
			goto error;

		/* Bad sync. */
		if (!resource_is_writable(&mportals[portalid].resource))
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&mportals[portalid].resource))
			goto error;

		/* Decrement references. */
		mportals[portalid].refcount--;

		/* Releases resource (for now, refcount always is 0 here). */
		if (!mportals[portalid].refcount)
		{
			kportal_port_release(
				mportals[portalid].config.remote,
				mportals[portalid].config.local_port
			);
			mportals[portalid].mallow = -1;
			mportals[portalid].mdata  = -1;
			mportals[portalid].config = MPORTAL_CONFIG_NULL;
			resource_free(&mportalpool, portalid);
		}

		mportal_counters.ncloses++;
		ret = (0);

error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * kportal_aread()                                                            *
 *============================================================================*/

PRIVATE int do_aread_message_drop(int mbxid, struct mportal_config * config)
{
	int ret;                        /* Return value.    */
	struct mportal_message message; /* Auxiliar buffer. */

	if (config)
		kportal_print_message("Aread dropping a message (Portal not opened)", config);
	else
		kportal_print_message("Aread dropping a message (read size < write size)", config);

	/* Reads and discard the rest of the message. */
	do
		ret = kmailbox_read(mbxid, &message, MPORTAL_MESSAGE_SIZE);
	while (!message.eof && ret > 0);

	return (ret);
}

/*----------------------------------------------------------------------------*
 * do_kportal_aread()                                                         *
 *----------------------------------------------------------------------------*/

PRIVATE ssize_t do_kportal_aread(struct mportal * portal, void * buffer, size_t size)
{
	ssize_t ret;                    /* Return value.                            */
	void * data;                    /* Auxiliar buffer pointer.                 */
	int remote;
	bool valid;                     /* Define if the messages will be ignored.  */
	bool buffering;                 /* Define where the data will be store.     */
	size_t received;                /* Received data counter.                   */
	struct mportal_buffer * buf;    /* Auxiliar buffer pointer.                 */
	struct mportal_message message; /* Message buffer.                          */
	struct mportal_config config;   /* Configuration pointer.                   */

	/* Valid portal. */
	KASSERT(portal && node_is_valid(portal->config.remote));

	buffering = true;
	remote    = portal->config.remote;

again:
	if (buffering)
	{
		buf       = NULL;
		valid     = true;
		buffering = false;
		received  = size;
	}

	/* Reads buffered message. */
	if ((ret = kportal_buffer_read(portal, &buffer, &received, &buf)) != 0)
	{
		/* Is it copied correctly? */
		ret = (ret < 0) ? (ret) : ((received != 0) ? (ret) : (ssize_t)(size));

		/* Is the read complete? */
		if (ret < 0 || received == 0)
			goto exit;
	}

	/* Intermediary read from buffers. */
	if (buf)
		goto again;

again2:
	spinlock_lock(&read_lock[remote]);

		/* Reads buffered message. */
		if ((ret = kportal_buffer_read(portal, &buffer, &received, &buf)) != 0)
		{
			/* Is it copied correctly? */
			ret = (ret < 0) ? (ret) : ((received != 0) ? (ret) : (ssize_t)(size));
			goto exit;
		}

		/* Is the channel busy? */
		if (resource_is_busy(&read_channels[remote]))
		{
			spinlock_unlock(&read_lock[remote]);
			goto again2;
		}

		/* Set channel busy. */
		resource_set_busy(&read_channels[remote]);

		/* Allows. */
		if ((ret = kmailbox_write(portal->mallow, &portal->config, MPORTAL_CONFIG_SIZE)) < 0)
			goto release;

		/* Reads header. */
		if ((ret = kmailbox_read(portal->mdata, &message, MPORTAL_MESSAGE_SIZE)) < 0)
			goto release;

		/* Sanity check. */
		KASSERT(message.header || !message.eof);

		/* Is the message to an existing portal? */
		config.local       = message._.config.remote;
		config.local_port  = message._.config.remote_port;
		config.remote      = message._.config.local;
		config.remote_port = message._.config.local_port;
		spinlock_lock(&global_lock);
			valid          = (kportal_search(&config, true) >= 0);
		spinlock_unlock(&global_lock);

		if (!valid)
		{
			do_aread_message_drop(portal->mdata, &message._.config);
			ret = (1);
			goto release;
		}

		/* Is the message to the current port? */
		buffering = (portal->config.remote_port != message._.config.local_port);

		/* Buffering mode allocates a auxiliar buffer. */
		if (buffering)
		{
			while ((buf = kportal_buffer_alloc(NULL, &message._.config)) == NULL);
			data = buf->data;
		}

		/* The message will be copied to the current buffer (if it is valid). */
		else
			data = buffer;

		received = 0ULL;

		/* Reads. */
		while (!message.eof)
		{
			/* Reads a piece of the message. */
			if ((ret = kmailbox_read(portal->mdata, &message, MPORTAL_MESSAGE_SIZE)) < 0)
				goto release;

			/* Sanity check. */
			KASSERT(!message.header);

			if (!buffering)
			{
				/* Isn't it ok read the message? */
				if ((received + message.size) > size)
				{
					do_aread_message_drop(portal->mdata, NULL);
					ret = (-EINVAL);
					goto release;
				}
			}
			else
			{
				/* Keeps previous buffer and alloc a new one. */
				if ((received + message.size) > MPORTAL_BUFFER_SIZE)
				{
					/* Complete the buffer. */
					kmemcpy(data, message._.data, (MPORTAL_BUFFER_SIZE - received));
					buf->size += (MPORTAL_BUFFER_SIZE - received);

					spinlock_unlock(&read_lock[remote]);
						while ((buf = kportal_buffer_alloc(buf, &buf->config)) == NULL);

						kmemcpy(
							buf->data,
							message._.data + (MPORTAL_BUFFER_SIZE - received),
							message.size - (MPORTAL_BUFFER_SIZE - received)
						);

						/* Copies the rest of the message in the new buffer. */
						received  = (message.size - (MPORTAL_BUFFER_SIZE - received));
						buf->size = received;
						data      = (buf->data + received);
					spinlock_lock(&read_lock[remote]);

					continue;
				}

				buf->size += message.size;
			}

			kmemcpy(data, message._.data, message.size);

			/* Next pieces. */
			data      += message.size;
			received  += message.size;
		}

		/* Makes buffer available. */
		kportal_buffer_set_available(buf);

		if (!buffering && (ret >= 0))
			ret = (received == size) ? ((ssize_t)(size)) : (-EINVAL);

release:
		resource_set_notbusy(&read_channels[remote]);
exit:
	spinlock_unlock(&read_lock[remote]);

	if (ret >= 0)
	{
		if (!valid || buffering || (ret < (ssize_t)(size)))
			goto again;

		portal->volume += size;
	}

	return (ret);
}


/*----------------------------------------------------------------------------*
 * do_kportal_aread_local()                                                   *
 *----------------------------------------------------------------------------*/

PRIVATE ssize_t do_kportal_aread_local(
	struct mportal * portal,
	void * buffer,
	size_t size
)
{
	ssize_t ret;                      /* Return value.            */
	size_t remainder;                 /* Remainder data size.     */
	struct mportal_buffer * previous; /* Auxiliar buffer pointer. */

	remainder = size;
	previous  = NULL;

again:
	spinlock_lock(&local_lock);

		/* Reads buffered message. */
		if ((ret = kportal_buffer_read(portal, &buffer, &remainder, &previous)) < 0)
			goto error;

		/* Successfully readed. */
		if (remainder == 0)
			portal->volume += (ret = size);

		/* Still reading. */
		else if (ret >= 0)
		{
			spinlock_unlock(&local_lock);
			goto again;
		}

		/* Error occurred. */
		else
			ret = (-EINVAL);

error:
	spinlock_unlock(&local_lock);

	return (ret);
}

/*----------------------------------------------------------------------------*
 * kportal_aread()                                                            *
 *----------------------------------------------------------------------------*/

/**
 * @details The kportal_aread() asynchronously read @p size bytes of
 * data pointed to by @p buffer from the input portal @p portalid.
 */
PUBLIC ssize_t kportal_aread(int portalid, void * buffer, size_t size)
{
	ssize_t ret;      /* Return value. */
	uint64_t latency; /* Latency.      */

	/* Invalid portalid. */
	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid size. */
	if (size == 0 || size > KPORTAL_MAX_SIZE)
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&mportals[portalid].resource))
			goto error;

		/* Bad sync. */
		if (!resource_is_readable(&mportals[portalid].resource))
			goto error;

		ret = (-EACCES);

		/* Bad sync. */
		if (mportals[portalid].config.remote == -1)
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&mportals[portalid].resource))
			goto error;

		resource_set_busy(&mportals[portalid].resource);

	spinlock_unlock(&global_lock);

	/* Is local communication? */
	if (node_is_local(mportals[portalid].config.remote))
		ret = do_kportal_aread_local(&mportals[portalid], buffer, size);
	else
	{
		ret = do_kportal_aread(&mportals[portalid], buffer, size);

		if (ret >= 0)
		{
			kmailbox_ioctl(mportals[portalid].mdata, KMAILBOX_IOCTL_GET_LATENCY, &latency);
			mportals[portalid].latency += latency;
		}
	}

	spinlock_lock(&global_lock);
		/* Complete the communication allowed. */
		mportals[portalid].mallow             = -1;
		mportals[portalid].mdata              = -1;
		mportals[portalid].config.remote      = -1;
		mportals[portalid].config.remote_port = -1;

		if (ret >= 0)
			mportal_counters.nreads++;

		resource_set_notbusy(&mportals[portalid].resource);
error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * kportal_awrite()                                                           *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * kportal_receive_allow()                                                    *
 *----------------------------------------------------------------------------*/

PRIVATE void kportal_receive_allow(struct mportal_config * config)
{
	/* Sanity checks. */
	KASSERT(node_is_local(config->remote));

	for (unsigned i = 0; i < KPORTAL_MAX; ++i)
	{
		if (!resource_is_used(&mportals[i].resource))
			continue;

		if (!resource_is_writable(&mportals[i].resource))
			continue;

		if (mportals[i].config.remote != config->local)
			continue;

		if (remote_is_allowed[config->local])
			kportal_print_message("Drop allow (double allowed)", config);
		else
			remote_is_allowed[config->local] = true;

		break;
	}

	if (!remote_is_allowed[config->local])
		kportal_print_message("Drop allow (any portal opened to remote)", config);
}


/*----------------------------------------------------------------------------*
 * do_kportal_wait_allow()                                                      *
 *----------------------------------------------------------------------------*/

PRIVATE int do_kportal_wait_allow(struct mportal * portal)
{
	int ret;
	bool released;
	struct mportal_config config; /* Hash buffer. */

	released = false;

	while (!released)
	{
		spinlock_lock(&allow_lock);

			/* Released. */
			if (!remote_is_allowed[portal->config.remote])
			{
				ret = (-EAGAIN);

				/* Waits allow message. */
				if ((ret = kmailbox_read(portal->mallow, &config, MPORTAL_CONFIG_SIZE)) < 0)
				{
					spinlock_unlock(&allow_lock);
					return (ret);
				}

				/* Allow remote communication. */
				kportal_receive_allow(&config);
			}

			/* Released. */
			if (remote_is_allowed[portal->config.remote])
			{
				released = true;
				remote_is_allowed[portal->config.remote] = false;
			}

		spinlock_unlock(&allow_lock);
	}

	return (0);
}

/*----------------------------------------------------------------------------*
 * do_kportal_awrite()                                                      *
 *----------------------------------------------------------------------------*/

PRIVATE ssize_t do_kportal_awrite(struct mportal * portal, const void * buffer, size_t size)
{
	ssize_t ret;                    /* Return value.               */
	size_t n;                       /* Size of current data piece. */
	size_t sended;                  /* Amount of data sended.      */
	size_t remainder;               /* Remainder of total data.    */
	size_t times;                   /* Number of pieces.           */
	struct mportal_message message; /* Message buffer.             */

	/* Waits allows. */
	if ((ret = do_kportal_wait_allow(portal)) < 0)
		return (ret);

	/* Sends header. */
	message.header   = true;
	message.eof      = false;
	message._.config = portal->config;

again:
	spinlock_lock(&write_lock[portal->config.remote]);

		if (resource_is_busy(&write_channels[portal->config.remote]))
		{
			spinlock_unlock(&write_lock[portal->config.remote]);
			goto again;
		}

		resource_set_busy(&write_channels[portal->config.remote]);

		/* Reads a piece of the message. */
		if ((ret = kmailbox_write(portal->mdata, &message, MPORTAL_MESSAGE_SIZE)) < 0)
			goto error;

		/* Sends data. */
		sended    = 0ULL;
		times     = size / MPORTAL_MESSAGE_DATA_SIZE;
		remainder = size - (times * MPORTAL_MESSAGE_DATA_SIZE);

		message.header = false;
		message.eof    = false;

		for (size_t t = 0; t < times + (remainder != 0); ++t)
		{
			n = (t != times) ? MPORTAL_MESSAGE_DATA_SIZE : remainder;

			kmemcpy(message._.data, buffer, n);

			sended      += (message.size = n);
			message.eof  = (sended == size);

			/* Reads a piece of the message. */
			if ((ret = kmailbox_write(portal->mdata, &message, MPORTAL_MESSAGE_SIZE)) < 0)
				goto error;

			/* Next pieces. */
			buffer += n;
		}

		ret = size;

error:
		resource_set_notbusy(&write_channels[portal->config.remote]);
	spinlock_unlock(&write_lock[portal->config.remote]);

	if (ret >= 0)
		portal->volume += size;

	return (ret);
}

/*----------------------------------------------------------------------------*
 * do_kportal_awrite_local()                                                  *
 *----------------------------------------------------------------------------*/

PRIVATE ssize_t do_kportal_awrite_local(struct mportal * portal, const void * buffer, size_t size)
{
	size_t n;                    /* Size of current data piece. */
	uint64_t t0;                 /* Clock value.                */
	uint64_t t1;                 /* Clock value.                */
	size_t remainder;            /* Remainder of total data.    */
	struct mportal_buffer * buf; /* Auxiliar buffer pointer.    */
	struct mportal_buffer * aux; /* Auxiliar buffer pointer.    */

	buf       = NULL;
	remainder = size;

again:
	spinlock_lock(&local_lock);

		while (remainder)
		{
			n = (remainder < MPORTAL_BUFFER_SIZE) ? remainder : MPORTAL_BUFFER_SIZE;

			/* Has any buffer free? */
			if ((aux = kportal_buffer_alloc(buf, &portal->config)) != NULL)
				buf = aux;
			else
			{
				spinlock_unlock(&local_lock);
				goto again;
			}

			kclock(&t0);
				kmemcpy(buf->data, buffer, n);
			kclock(&t1);
			portal->latency += (t1 - t0);

			remainder -= n;
			buffer    += n;
			buf->size += n;
		}

		/* Makes buffer available. */
		kportal_buffer_set_available(buf);


	spinlock_unlock(&local_lock);

	portal->volume += size;

	return (size);
}

/*----------------------------------------------------------------------------*
 * kportal_awrite()                                                           *
 *----------------------------------------------------------------------------*/

/**
 * @details The kportal_awrite() asynchronously write @p size bytes
 * of data pointed to by @p buffer to the output portal @p portalid.
 */
PUBLIC ssize_t kportal_awrite(int portalid, const void * buffer, size_t size)
{
	ssize_t ret;      /* Return value. */
	uint64_t latency; /* Latency.      */

	/* Invalid portalid. */
	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid size. */
	if (size == 0 || size > KPORTAL_MAX_SIZE)
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&mportals[portalid].resource))
			goto error;

		/* Bad sync. */
		if (!resource_is_writable(&mportals[portalid].resource))
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&mportals[portalid].resource))
			goto error;

		resource_set_busy(&mportals[portalid].resource);

	spinlock_unlock(&global_lock);

	/* Is local communication? */
	if (node_is_local(mportals[portalid].config.remote))
		ret = do_kportal_awrite_local(&mportals[portalid], buffer, size);
	else
	{
		ret = do_kportal_awrite(&mportals[portalid], buffer, size);

		if (ret >= 0)
		{
			kmailbox_ioctl(mportals[portalid].mdata, KMAILBOX_IOCTL_GET_LATENCY, &latency);
			mportals[portalid].latency += latency;
		}
	}

	spinlock_lock(&global_lock);
		if (ret >= 0)
			mportal_counters.nwrites++;

		resource_set_notbusy(&mportals[portalid].resource);
error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * kportal_wait()                                                             *
 *============================================================================*/

/**
 * @details The kportal_wait() waits for asyncrhonous operations in
 * the input/output portal @p portalid to complete.
 */
PUBLIC int kportal_wait(int portalid)
{
	int ret; /* Return value. */

	/* Invalid portalid. */
	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&mportals[portalid].resource))
			goto error;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&mportals[portalid].resource))
			goto error;

		ret = (0);

error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * kportal_read()                                                             *
 *============================================================================*/

/**
 * @details The kportal_read() synchronously read @p size bytes of
 * data pointed to by @p buffer from the input portal @p portalid.
 */
PUBLIC ssize_t kportal_read(int portalid, void * buffer, size_t size)
{
	return (kportal_aread(portalid, buffer, size));
}

/*============================================================================*
 * kportal_write()                                                            *
 *============================================================================*/

/**
 * @details The kportal_write() synchronously write @p size bytes of
 * data pointed to by @p buffer to the output portal @p portalid.
 */
PUBLIC ssize_t kportal_write(int portalid, const void * buffer, size_t size)
{
	return (kportal_awrite(portalid, buffer, size));
}

/*============================================================================*
 * kportal_ioctl()                                                            *
 *============================================================================*/

PRIVATE int kportal_ioctl_valid(void * ptr, size_t size)
{
	return ((ptr != NULL) && mm_check_area(VADDR(ptr), size, UMEM_AREA));
}

/**
 * @details The kportal_ioctl() reads the measurement parameter associated
 * with the request id @p request of the portal @p portalid.
 */
PUBLIC int kportal_ioctl(int portalid, unsigned request, ...)
{
	int ret;      /* Return value.  */
	va_list args; /* Argument list. */

	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&mportals[portalid].resource))
			goto error0;

		ret = (-EBUSY);

		/* Busy sync. */
		if (resource_is_busy(&mportals[portalid].resource))
			goto error0;

		ret = (-EFAULT);

		va_start(args, request);

			/* Parse request. */
			switch (request)
			{
				/* Get the amount of data transferred so far. */
				case KPORTAL_IOCTL_GET_VOLUME:
				{
					size_t * volume = va_arg(args, size_t *);

					/* Bad buffer. */
					if (!kportal_ioctl_valid(volume, sizeof(size_t)))
						goto error1;

					*volume = mportals[portalid].volume;
					ret = 0;
				} break;

				/* Get uint64_t parameter. */
				case KPORTAL_IOCTL_GET_LATENCY:
				case KPORTAL_IOCTL_GET_NCREATES:
				case KPORTAL_IOCTL_GET_NUNLINKS:
				case KPORTAL_IOCTL_GET_NOPENS:
				case KPORTAL_IOCTL_GET_NCLOSES:
				case KPORTAL_IOCTL_GET_NREADS:
				case KPORTAL_IOCTL_GET_NWRITES:
				{
					uint64_t * var = va_arg(args, uint64_t *);

					/* Bad buffer. */
					if (!kportal_ioctl_valid(var, sizeof(uint64_t)))
						goto error1;

					ret = 0;

					switch(request)
					{
						case KPORTAL_IOCTL_GET_LATENCY:
							*var = mportals[portalid].latency;
							break;

						case KPORTAL_IOCTL_GET_NCREATES:
							*var = mportal_counters.ncreates;
							break;

						case KPORTAL_IOCTL_GET_NUNLINKS:
							*var = mportal_counters.nunlinks;
							break;

						case KPORTAL_IOCTL_GET_NOPENS:
							*var = mportal_counters.nopens;
							break;

						case KPORTAL_IOCTL_GET_NCLOSES:
							*var = mportal_counters.ncloses;
							break;

						case KPORTAL_IOCTL_GET_NREADS:
							*var = mportal_counters.nreads;
							break;

						case KPORTAL_IOCTL_GET_NWRITES:
							*var = mportal_counters.nwrites;
							break;

						/* Operation not supported. */
						default:
							ret = (-ENOTSUP);
							break;
					}
				} break;

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
 * kportal_write()                                                            *
 *============================================================================*/

/**
 * @details Write details.
 */
PUBLIC int kportal_get_port(int portalid)
{
	int ret; /* Return value. */

	/* Invalid portalid. */
	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	spinlock_lock(&global_lock);

		ret = (-EBADF);

		/* Bad sync. */
		if (!resource_is_used(&mportals[portalid].resource))
			goto error;

		ret = mportals[portalid].config.local_port;

error:
	spinlock_unlock(&global_lock);

	return (ret);
}

/*============================================================================*
 * kportal_init()                                                             *
 *============================================================================*/

/**
 * @details The kportal_init() Initializes underlying mailboxes.
 */
PUBLIC void kportal_init(void)
{
	unsigned local = knode_get_num();

	kprintf("[user][portal] Initializes portal module (mailbox implementation)");

	mportal_counters.ncreates = 0ULL;
	mportal_counters.nunlinks = 0ULL;
	mportal_counters.nopens   = 0ULL;
	mportal_counters.ncloses  = 0ULL;
	mportal_counters.nreads   = 0ULL;
	mportal_counters.nwrites  = 0ULL;

	/* Create input mailbox. */
	KASSERT(
		(mallow_in = kcall2(
			NR_mailbox_create,
			(word_t) local,
			(word_t) (MAILBOX_PORT_NR - 2)
		)) >= 0
	);

	/* Opens output mailboxes. */
	for (unsigned i = 0; i < PROCESSOR_NOC_NODES_NUM; ++i)
	{
		if (i == local)
			continue;

		KASSERT(
			(mallow_outs[i] = kcall2(
				NR_mailbox_open,
				(word_t) i,
				(word_t) (MAILBOX_PORT_NR - 2)
			)) >= 0
		);

		KASSERT(
			(mdata_ins[i] = kcall2(
				NR_mailbox_create,
				(word_t) local,
				(word_t) (MAILBOX_PORT_NR - 3 - i)
			)) >= 0
		);

		KASSERT(
			(mdata_outs[i] = kcall2(
				NR_mailbox_open,
				(word_t) i,
				(word_t) (MAILBOX_PORT_NR - 3 - local)
			)) >= 0
		);

		/* Set channel busy. */
		resource_set_used(&read_channels[i]);
	}

	kportal_is_initialized = true;
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_MAILBOX && __NANVIX_IKC_USES_ONLY_MAILBOX */
