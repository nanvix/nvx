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

#define __NANVIX_MICROKERNEL

#include <nanvix/kernel/kernel.h>

#if __TARGET_HAS_MAILBOX

#include <posix/errno.h>

#if __NANVIX_IKC_USES_ONLY_MAILBOX

#include <nanvix/sys/noc.h>

/**
 * @brief Protections.
 */
PRIVATE spinlock_t global_lock = SPINLOCK_UNLOCKED;

/**
 * @brief Counter control.
 */
PRIVATE bool user_mailboxes[KMAILBOX_MAX] = {
	[0 ... (KMAILBOX_MAX - 1)] = false
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
} mailbox_counters;

#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

/*============================================================================*
 * kmailbox_create()                                                          *
 *============================================================================*/

/**
 * @details The kmailbox_create() function creates an input mailbox
 * and attaches it to the local NoC node @p local in the port @p port.
 */
int kmailbox_create(int local, int port)
{
	int ret;

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	if (!WITHIN(port, 0, KMAILBOX_PORT_NR))
		return (-EINVAL);
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	ret = kcall2(
		NR_mailbox_create,
		(word_t) local,
		(word_t) port
	);

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	if (ret >= 0)
	{
		spinlock_lock(&global_lock);
			mailbox_counters.ncreates++;
		spinlock_unlock(&global_lock);

		user_mailboxes[ret] = true;
	}
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	return (ret);
}

/*============================================================================*
 * kmailbox_open()                                                            *
 *============================================================================*/

/**
 * @details The kmailbox_open() function opens an output mailbox to
 * the remote NoC node @p remote in the port @p port.
 */
int kmailbox_open(int remote, int remote_port)
{
	int ret;

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	if (!WITHIN(remote_port, 0, KMAILBOX_PORT_NR))
		return (-EINVAL);
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	ret = kcall2(
		NR_mailbox_open,
		(word_t) remote,
		(word_t) remote_port
	);

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	if (ret >= 0)
	{
		spinlock_lock(&global_lock);
			mailbox_counters.nopens++;
		spinlock_unlock(&global_lock);

		user_mailboxes[ret] = true;
	}
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	return (ret);
}

/*============================================================================*
 * kmailbox_unlink()                                                          *
 *============================================================================*/

/**
 * @details The kmailbox_unlink() function removes and releases the underlying
 * resources associated to the input mailbox @p mbxid.
 */
int kmailbox_unlink(int mbxid)
{
	int ret;

	ret = kcall1(
		NR_mailbox_unlink,
		(word_t) mbxid
	);

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	if (ret >= 0)
	{
		spinlock_lock(&global_lock);
			mailbox_counters.nunlinks++;
		spinlock_unlock(&global_lock);

		user_mailboxes[mbxid] = false;
	}
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	return (ret);
}

/*============================================================================*
 * kmailbox_close()                                                           *
 *============================================================================*/

/**
 * @details The kmailbox_close() function closes and releases the
 * underlying resources associated to the output mailbox @p mbxid.
 */
int kmailbox_close(int mbxid)
{
	int ret;

	ret = kcall1(
		NR_mailbox_close,
		(word_t) mbxid
	);

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	if (ret >= 0)
	{
		spinlock_lock(&global_lock);
			mailbox_counters.ncloses++;
		spinlock_unlock(&global_lock);

		user_mailboxes[mbxid] = false;
	}
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	return (ret);
}

/*============================================================================*
 * kmailbox_awrite()                                                          *
 *============================================================================*/

/**
 * @details The kmailbox_awrite() asynchronously write @p size bytes
 * of data pointed to by @p buffer to the output mailbox @p mbxid.
 */
ssize_t kmailbox_awrite(int mbxid, const void * buffer, size_t size)
{
	int ret;

	/* Invalid buffer size. */
	if ((size == 0) || (size > KMAILBOX_MESSAGE_SIZE))
		return (-EINVAL);

	do
	{
		ret = kcall3(
			NR_mailbox_awrite,
			(word_t) mbxid,
			(word_t) buffer,
			(word_t) size
		);
	} while ((ret == -ETIMEDOUT) || (ret == -EAGAIN) || (ret == -EBUSY));

	return (ret);
}

/*============================================================================*
 * kmailbox_aread()                                                           *
 *============================================================================*/

/**
 * @details The kmailbox_aread() asynchronously read @p size bytes of
 * data pointed to by @p buffer from the input mailbox @p mbxid.
 */
ssize_t kmailbox_aread(int mbxid, void * buffer, size_t size)
{
	int ret;

	/* Invalid buffer size. */
	if ((size == 0) || (size > KMAILBOX_MESSAGE_SIZE))
		return (-EINVAL);

	do
	{
		ret = kcall3(
			NR_mailbox_aread,
			(word_t) mbxid,
			(word_t) buffer,
			(word_t) size
		);
	} while ((ret == -ETIMEDOUT) || (ret == -EBUSY) || (ret == -ENOMSG));

	return (ret);
}

/*============================================================================*
 * kmailbox_wait()                                                            *
 *============================================================================*/

/**
 * @details The kmailbox_wait() waits for asyncrhonous operations in
 * the input/output mailbox @p mbxid to complete.
 */
int kmailbox_wait(int mbxid)
{
	int ret;

	ret = kcall1(
		NR_mailbox_wait,
		(word_t) mbxid
	);

	return (ret);
}

/*============================================================================*
 * kmailbox_write()                                                           *
 *============================================================================*/

/**
 * @details The kmailbox_write() synchronously write @p size bytes of
 * data pointed to by @p buffer to the output mailbox @p mbxid.
 *
 * @todo Uncomment kmailbox_wait() call when microkernel properly supports it.
 */
ssize_t kmailbox_write(int mbxid, const void * buffer, size_t size)
{
	int ret;

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid buffer size. */
	if ((size == 0) || (size > KMAILBOX_MESSAGE_SIZE))
		return (-EINVAL);

	if ((ret = kmailbox_awrite(mbxid, buffer, size)) < 1)
		return (ret);

	if ((ret = kmailbox_wait(mbxid)) < 0)
		return (ret);

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	spinlock_lock(&global_lock);
		if (user_mailboxes[mbxid])
			mailbox_counters.nwrites++;
	spinlock_unlock(&global_lock);
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	return (size);
}

/*============================================================================*
 * kmailbox_read()                                                            *
 *============================================================================*/

/**
 * @details The kmailbox_read() synchronously read @p size bytes of
 * data pointed to by @p buffer from the input mailbox @p mbxid.
 */
ssize_t kmailbox_read(int mbxid, void * buffer, size_t size)
{
	int ret;

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid buffer size. */
	if ((size == 0) || (size > KMAILBOX_MESSAGE_SIZE))
		return (-EINVAL);

	/* Repeat while reading valid messages for another ports. */
	do
	{
		if ((ret = kmailbox_aread(mbxid, buffer, size)) < 0)
			return (ret);
	} while ((ret = kmailbox_wait(mbxid)) > 0);

	/* Wait failed. */
	if (ret < 0)
		return (ret);

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	spinlock_lock(&global_lock);
		if (user_mailboxes[mbxid])
			mailbox_counters.nreads++;
	spinlock_unlock(&global_lock);
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	return (size);
}

/*============================================================================*
 * kmailbox_ioctl()                                                           *
 *============================================================================*/

PRIVATE int kmailbox_ioctl_valid(void * ptr, size_t size)
{
	return ((ptr != NULL) && mm_check_area(VADDR(ptr), size, UMEM_AREA));
}

/**
 * @details The kmailbox_ioctl() reads the measurement parameter associated
 * with the request id @p request of the mailbox @p mbxid.
 */
int kmailbox_ioctl(int mbxid, unsigned request, ...)
{
	int ret;
	va_list args;

	va_start(args, request);

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	spinlock_lock(&global_lock);

		/* Parse request. */
		switch (request)
		{
			/* Get the amount of data transferred so far. */
			case KMAILBOX_IOCTL_GET_VOLUME:
			case KMAILBOX_IOCTL_GET_LATENCY:
			{
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

				dcache_invalidate();

				ret = kcall3(
					NR_mailbox_ioctl,
					(word_t) mbxid,
					(word_t) request,
					(word_t) &args
				);

				dcache_invalidate();

#if __NANVIX_IKC_USES_ONLY_MAILBOX
			} break;

			case KMAILBOX_IOCTL_GET_NCREATES:
			case KMAILBOX_IOCTL_GET_NUNLINKS:
			case KMAILBOX_IOCTL_GET_NOPENS:
			case KMAILBOX_IOCTL_GET_NCLOSES:
			case KMAILBOX_IOCTL_GET_NREADS:
			case KMAILBOX_IOCTL_GET_NWRITES:
			{
				ret = (-EINVAL);

				/* Bad mailbox. */
				if ((ret = kcomm_get_port(mbxid, COMM_TYPE_MAILBOX)) < 0)
					goto error;

				uint64_t * var = va_arg(args, uint64_t *);

				ret = (-EFAULT);

				/* Bad buffer. */
				if (!kmailbox_ioctl_valid(var, sizeof(uint64_t)))
					goto error;

				ret = 0;

				switch(request)
				{
					case KMAILBOX_IOCTL_GET_NCREATES:
						*var = mailbox_counters.ncreates;
						break;

					case KMAILBOX_IOCTL_GET_NUNLINKS:
						*var = mailbox_counters.nunlinks;
						break;

					case KMAILBOX_IOCTL_GET_NOPENS:
						*var = mailbox_counters.nopens;
						break;

					case KMAILBOX_IOCTL_GET_NCLOSES:
						*var = mailbox_counters.ncloses;
						break;

					case KMAILBOX_IOCTL_GET_NREADS:
						*var = mailbox_counters.nreads;
						break;

					case KMAILBOX_IOCTL_GET_NWRITES:
						*var = mailbox_counters.nwrites;
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

error:
	spinlock_unlock(&global_lock);
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */

	va_end(args);

	return (ret);
}

/*============================================================================*
 * kmailbox_init()                                                            *
 *============================================================================*/

/**
 * @details The kmailbox_init() Initializes mailbox system.
 */
PUBLIC void kmailbox_init(void)
{
	kprintf("[user][mailbox] Initializes mailbox module");

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	mailbox_counters.ncreates = 0ULL;
	mailbox_counters.nunlinks = 0ULL;
	mailbox_counters.nopens   = 0ULL;
	mailbox_counters.ncloses  = 0ULL;
	mailbox_counters.nreads   = 0ULL;
	mailbox_counters.nwrites  = 0ULL;
#endif /* __NANVIX_IKC_USES_ONLY_MAILBOX */
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_MAILBOX */
