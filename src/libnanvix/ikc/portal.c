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

#if __TARGET_HAS_PORTAL && !__NANVIX_IKC_USES_ONLY_MAILBOX

#include <nanvix/sys/noc.h>
#include <posix/errno.h>
#include "task.h"

/**
 * @brief Protection for allows variable.
 */
PRIVATE spinlock_t kportal_lock = SPINLOCK_UNLOCKED;

/**
 * @brief Store allows information.
 */
PRIVATE struct
{
	int remote; /**< Remote ID.      */
	int port;   /**< Remote port ID. */
} kportal_allows[KPORTAL_MAX];

/*============================================================================*
 * kportal_create()                                                           *
 *============================================================================*/

/**
 * @details The kportal_create() function creates an input portal and
 * attaches it to the local port @p local_port in the local NoC node @p
 * local.
 */
int kportal_create(int local, int local_port)
{
	int ret;

	/* Invalid local number for the requesting core ID. */
	if (local != knode_get_num())
		return (-EINVAL);

	ret = kcall2(
		NR_portal_create,
		(word_t) local,
		(word_t) local_port
	);

	if (ret >= 0)
	{
		spinlock_lock(&kportal_lock);
			kportal_allows[ret].remote = -1;
			kportal_allows[ret].port   = -1;
		spinlock_unlock(&kportal_lock);
	}

	return (ret);
}

/*============================================================================*
 * kportal_allow()                                                            *
 *============================================================================*/

/**
 * @details The kportal_allow() function allow read data from a input portal
 * associated with the NoC node @p remote.
 */
int kportal_allow(int portalid, int remote, int remote_port)
{
	int ret;

	/* Invalid portalid. */
	if (UNLIKELY(!WITHIN(portalid, 0, KPORTAL_MAX)))
		return (-EINVAL);

	/* Invalid remote. */
	if (UNLIKELY(!WITHIN(remote, 0, PROCESSOR_NOC_NODES_NUM)))
		return (-EINVAL);

	/* Invalid remote port. */
	if (UNLIKELY(!WITHIN(remote_port, 0, KPORTAL_PORT_NR)))
		return (-EINVAL);

	ret = kcall3(
		NR_portal_allow,
		(word_t) portalid,
		(word_t) remote,
		(word_t) remote_port
	);

	if (LIKELY(ret == 0))
	{
		spinlock_lock(&kportal_lock);
			kportal_allows[portalid].remote = remote;
			kportal_allows[portalid].port   = remote_port;
		spinlock_unlock(&kportal_lock);
	}

	return (ret);
}

/*============================================================================*
 * kportal_open()                                                             *
 *============================================================================*/

/**
 * @details The kportal_open() function opens an output portal to the remote
 * NoC node @p remote and attaches it to the local NoC node @p local.
 */
int kportal_open(int local, int remote, int remote_port)
{
	int ret;

	/* Invalid local number for the requesting core ID. */
	if (local != knode_get_num())
		return (-EINVAL);

	ret = kcall3(
		NR_portal_open,
		(word_t) local,
		(word_t) remote,
		(word_t) remote_port
	);

	return (ret);
}

/*============================================================================*
 * kportal_unlink()                                                           *
 *============================================================================*/

/**
 * @details The kportal_unlink() function removes and releases the underlying
 * resources associated to the input portal @p portalid.
 */
int kportal_unlink(int portalid)
{
	int ret;

	ret = kcall1(
		NR_portal_unlink,
		(word_t) portalid
	);

	return (ret);
}

/*============================================================================*
 * kportal_close()                                                            *
 *============================================================================*/

/**
 * @details The kportal_close() function closes and releases the
 * underlying resources associated to the output portal @p portalid.
 */
int kportal_close(int portalid)
{
	int ret;

	ret = kcall1(
		NR_portal_close,
		(word_t) portalid
	);

	return (ret);
}

/*============================================================================*
 * kportal_aread()                                                            *
 *============================================================================*/

ssize_t __kportal_aread(int portalid, void * buffer, size_t size)
{
	return (
		kcall3(
			NR_portal_aread,
			(word_t) portalid,
			(word_t) buffer,
			(word_t) size
		)
	);
}

/**
 * @details The kportal_aread() asynchronously read @p size bytes of
 * data pointed to by @p buffer from the input portal @p portalid.
 */
ssize_t kportal_aread(int portalid, void * buffer, size_t size)
{
	/* Invalid portalid. */
	if (UNLIKELY(!WITHIN(portalid, 0, KPORTAL_MAX)))
		return (-EINVAL);

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

#if !__NANVIX_USE_COMM_WITH_TASKS

	ssize_t ret;

	/* Invalid size. */
	if (size == 0 || size > KPORTAL_MESSAGE_DATA_SIZE)
		return (-EINVAL);

	do
		ret = __kportal_aread(portalid, buffer, size);
	while ((ret == -EBUSY) || (ret == -ENOMSG));

	return (ret);

#else

	int remote;
	int port;

	/* Invalid size. */
	if (size == 0 || size > KPORTAL_MAX_SIZE)
		return (-EINVAL);

	spinlock_lock(&kportal_lock);
		remote = kportal_allows[portalid].remote;
		port   = kportal_allows[portalid].port;
	spinlock_unlock(&kportal_lock);

	return (
		ikc_flow_config(
			IKC_FLOW_PORTAL_READ,
			(word_t) portalid,
			(word_t) buffer,
			(word_t) size,
			(word_t) remote,
			(word_t) port
		)
	);

#endif /* !__NANVIX_USE_COMM_WITH_TASKS */
}

/*============================================================================*
 * kportal_awrite()                                                           *
 *============================================================================*/

ssize_t __kportal_awrite(int portalid, const void * buffer, size_t size)
{
	return (
		kcall3(
			NR_portal_awrite,
			(word_t) portalid,
			(word_t) buffer,
			(word_t) size
		)
	);
}

/**
 * @details The kportal_awrite() asynchronously write @p size bytes
 * of data pointed to by @p buffer to the output portal @p portalid.
 */
ssize_t kportal_awrite(int portalid, const void * buffer, size_t size)
{
	/* Invalid portalid. */
	if (UNLIKELY(!WITHIN(portalid, 0, KPORTAL_MAX)))
		return (-EINVAL);

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

#if !__NANVIX_USE_COMM_WITH_TASKS

	ssize_t ret;

	/* Invalid size. */
	if (size == 0 || size > KPORTAL_MESSAGE_DATA_SIZE)
		return (-EINVAL);

	do
		ret = __kportal_awrite(portalid, buffer, size);
	while ((ret == -EACCES) || (ret == -EBUSY));

	return (ret);

#else

	/* Invalid size. */
	if (size == 0 || size > KPORTAL_MAX_SIZE)
		return (-EINVAL);

	return (
		ikc_flow_config(
			IKC_FLOW_PORTAL_WRITE,
			(word_t) portalid,
			(word_t) buffer,
			(word_t) size,
			(word_t) (-1),
			(word_t) (-1)
		)
	);

#endif /* !__NANVIX_USE_COMM_WITH_TASKS */
}

/*============================================================================*
 * kportal_wait()                                                             *
 *============================================================================*/

int __kportal_wait(int portalid)
{
	return (kcall1(NR_portal_wait, (word_t) portalid));
}

/**
 * @details The kportal_wait() waits for asyncrhonous operations in
 * the input/output portal @p portalid to complete.
 */
int kportal_wait(int portalid)
{
	/* Invalid portalid. */
	if (UNLIKELY(!WITHIN(portalid, 0, KPORTAL_MAX)))
		return (-EINVAL);

#if !__NANVIX_USE_COMM_WITH_TASKS

	return (__kportal_wait(portalid));

#else

	return (ikc_flow_wait(IKC_FLOW_PORTAL, (word_t) portalid));

#endif /* !__NANVIX_USE_COMM_WITH_TASKS */
}

/*============================================================================*
 * kportal_write()                                                            *
 *============================================================================*/

/**
 * @details The kportal_write() synchronously write @p size bytes of
 * data pointed to by @p buffer to the output portal @p portalid.
 */
ssize_t kportal_write(int portalid, const void * buffer, size_t size)
{
	ssize_t ret; /* Return value. */

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid size. */
	if (size == 0 || size > KPORTAL_MAX_SIZE)
		return (-EINVAL);

#if !__NANVIX_USE_COMM_WITH_TASKS

	size_t n;         /* Size of current data piece. */
	size_t remainder; /* Remainder of total data.    */
	size_t times;     /* Number of pieces.           */

	times     = size / KPORTAL_MESSAGE_DATA_SIZE;
	remainder = size - (times * KPORTAL_MESSAGE_DATA_SIZE);

	for (size_t t = 0; t < times + (remainder != 0); ++t)
	{
		n = (t != times) ? KPORTAL_MESSAGE_DATA_SIZE : remainder;

		/* Sends a piece of the message. */
		if ((ret = kportal_awrite(portalid, buffer, n)) < 0)
			return (ret);

		/* Waits for the asynchronous operation to complete. */
		if ((ret = kportal_wait(portalid)) != 0)
			return (ret);

		/* Next pieces. */
		buffer += n;
	}

#else

	/* Sends the entire message. */
	if ((ret = kportal_awrite(portalid, buffer, size)) < 0)
		return (ret);

	/* Waits for the asynchronous operation to complete. */
	if ((ret = kportal_wait(portalid)) != 0)
		return (ret);

#endif

	return (size);
}

/*============================================================================*
 * kportal_read()                                                             *
 *============================================================================*/

/**
 * @details The kportal_read() synchronously read @p size bytes of
 * data pointed to by @p buffer from the input portal @p portalid.
 */
ssize_t kportal_read(int portalid, void * buffer, size_t size)
{
	ssize_t ret; /* Return value. */

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid size. */
	if (size == 0 || size > KPORTAL_MAX_SIZE)
		return (-EINVAL);

#if !__NANVIX_USE_COMM_WITH_TASKS

	size_t n;         /* Size of current data piece. */
	size_t remainder; /* Remainder of total data.    */
	size_t times;     /* Number of pieces.           */
	int remote;       /* Number of target remote.    */
	int port;         /* Number of target port.      */

	times     = size / KPORTAL_MESSAGE_DATA_SIZE;
	remainder = size - (times * KPORTAL_MESSAGE_DATA_SIZE);
	ret       = (-EINVAL);
	spinlock_lock(&kportal_lock);
		remote = kportal_allows[portalid].remote;
		port   = kportal_allows[portalid].port;
	spinlock_unlock(&kportal_lock);

	for (size_t t = 0; t < times + (remainder != 0); ++t)
	{
		n = (t != times) ? KPORTAL_MESSAGE_DATA_SIZE : remainder;

		/* Repeat while reading valid messages for another ports. */
		do
		{
			/* Consecutive reads must be allowed. */
			if (t != 0 && ret >= 0)
				kportal_allow(portalid, remote, port);

			/* Reads a piece of the message. */
			if ((ret = kportal_aread(portalid, buffer, n)) < 0)
				return (ret);

		/* Waits for the asynchronous operation to complete. */
		} while ((ret = kportal_wait(portalid)) > 0);

		/* Wait failed. */
		if (ret < 0)
			return (ret);

		/* Next pieces. */
		buffer += n;
	}

#else

	/* Sends the entire message. */
	if ((ret = kportal_aread(portalid, buffer, size)) < 0)
		return (ret);

	/* Waits for the asynchronous operation to complete. */
	if ((ret = kportal_wait(portalid)) != 0)
		return (ret);

#endif

	/* Complete a allowed read. */
	spinlock_lock(&kportal_lock);
		kportal_allows[portalid].remote = -1;
		kportal_allows[portalid].port   = -1;
	spinlock_unlock(&kportal_lock);

	return (size);
}

/*============================================================================*
 * ktask_portal_allow()                                                       *
 *============================================================================*/

/**
 * @details Function to build a task to operate a kportal_allow.
 */
PUBLIC int ktask_portal_allow(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int ret;

	UNUSED(arg1);
	UNUSED(arg2);

	if ((ret = kportal_allow((int) arg0, (int) arg3, (int) arg4)) < 0)
		ktask_exit0(ret, KTASK_MANAGEMENT_ERROR);

	/* Success, pass arguments to the waiting task. */
	ktask_exit5(ret,
		KTASK_MANAGEMENT_USER0,
		KTASK_MERGE_ARGS_FN_REPLACE,
		arg0, arg1, arg2, arg3, arg4
	);

	return (ret);
}

/*============================================================================*
 * ktask_portal_aread()                                                      *
 *============================================================================*/

/**
 * @details Function to build a task to operate a kportal_aread.
 */
PUBLIC int ktask_portal_aread(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int ret;

	UNUSED(arg3);
	UNUSED(arg4);

	if ((ret = kportal_aread((int) arg0, (void *) arg1, (size_t) arg2)) < 0)
		ktask_exit0(ret, KTASK_MANAGEMENT_ERROR);

	/* Success, pass arguments to the waiting task. */
	ktask_exit5(ret,
		KTASK_MANAGEMENT_USER0,
		KTASK_MERGE_ARGS_FN_REPLACE,
		arg0, arg1, arg2, arg3, arg4
	);

	return (ret);
}

/*============================================================================*
 * ktask_portal_awrite()                                                     *
 *============================================================================*/

/**
 * @details Function to build a task to operate a kportal_awrite.
 */
PUBLIC int ktask_portal_awrite(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int ret;

	UNUSED(arg3);
	UNUSED(arg4);

	if ((ret = kportal_awrite((int) arg0, (const void *) arg1, (size_t) arg2)) < 0)
		ktask_exit0(ret, KTASK_MANAGEMENT_ERROR);

	/* Success, pass arguments to the waiting task. */
	ktask_exit5(ret,
		KTASK_MANAGEMENT_USER0,
		KTASK_MERGE_ARGS_FN_REPLACE,
		arg0, arg1, arg2, arg3, arg4
	);

	return (ret);
}

/*============================================================================*
 * ktask_portal_wait()                                                       *
 *============================================================================*/

/**
 * @details Function to build a task to operate a kportal_wait.
 */
PUBLIC int ktask_portal_wait(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int ret;

	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	if ((ret = kportal_wait((int) arg0)) < 0)
		ktask_exit0(ret, KTASK_MANAGEMENT_ERROR);

	return (ret);
}

/*============================================================================*
 * ktask_portal_write()                                                      *
 *============================================================================*/

/**
 * @details Build a write flow.
 *
 * -> awrite -> wait ->
 */
PUBLIC int ktask_portal_write(ktask_t * awrite, ktask_t * wait)
{
	int ret;

	if (UNLIKELY((ret = ktask_create(awrite, ktask_portal_awrite, KTASK_PRIORITY_LOW, 0, 0)) < 0))
		return (ret);

	if (UNLIKELY((ret = ktask_create(wait, ktask_portal_wait, KTASK_PRIORITY_HIGH, 0, KTASK_TRIGGER_DEFAULT)) < 0))
		return (ret);

	return (
		ktask_connect(
			awrite,
			wait,
			KTASK_CONN_IS_FLOW,
			KTASK_CONN_IS_PERSISTENT,
			KTASK_TRIGGER_DEFAULT
		)
	);
}

/*============================================================================*
 * ktask_portal_read()                                                       *
 *============================================================================*/

/**
 * @details Build a read flow.
 *
 * allow -> aread -> wait ->
 */
PUBLIC int ktask_portal_read(ktask_t * allow, ktask_t * aread, ktask_t * wait)
{
	int ret;

	if (UNLIKELY((ret = ktask_create(allow, ktask_portal_allow, KTASK_PRIORITY_LOW, 0, 0)) < 0))
		return (ret);

	if (UNLIKELY((ret = ktask_create(aread, ktask_portal_aread, KTASK_PRIORITY_LOW, 0, 0)) < 0))
		return (ret);

	if (UNLIKELY((ret = ktask_create(wait, ktask_portal_wait, KTASK_PRIORITY_HIGH, 0, KTASK_TRIGGER_DEFAULT)) < 0))
		return (ret);

	ret = ktask_connect(
		allow,
		aread,
		KTASK_CONN_IS_FLOW,
		KTASK_CONN_IS_PERSISTENT,
		KTASK_TRIGGER_DEFAULT
	);

	if (UNLIKELY(ret < 0))
		return (ret);

	return (
		ktask_connect(
			aread,
			wait,
			KTASK_CONN_IS_DEPENDENCY,
			KTASK_CONN_IS_PERSISTENT,
			KTASK_TRIGGER_DEFAULT
		)
	);
}

/*============================================================================*
 * kportal_ioctl()                                                            *
 *============================================================================*/

/**
 * @details The kportal_ioctl() reads the measurement parameter associated
 * with the request id @p request of the portal @p portalid.
 */
int kportal_ioctl(int portalid, unsigned request, ...)
{
	int ret;
	va_list args;

	va_start(args, request);

		dcache_invalidate();

		ret = kcall3(
			NR_portal_ioctl,
			(word_t) portalid,
			(word_t) request,
			(word_t) &args
		);

		dcache_invalidate();

	va_end(args);

	return (ret);
}

/*============================================================================*
 * kportal_get_port()                                                         *
 *============================================================================*/

/**
 * @details Get port details.
 */
PUBLIC int kportal_get_port(int portalid)
{
	int ret; /* Return value. */

	/* Invalid portalid. */
	if (!WITHIN(portalid, 0, KPORTAL_MAX))
		return (-EINVAL);

	ret = kcomm_get_port(portalid, COMM_TYPE_PORTAL);

	return (ret);
}

/*============================================================================*
 * kportal_init()                                                             *
 *============================================================================*/

/**
 * @details The kportal_init() Initializes portal system.
 */
PUBLIC void kportal_init(void)
{
	kprintf("[user][portal] Initializes portal module");
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_PORTAL && !__NANVIX_IKC_USES_ONLY_MAILBOX */
