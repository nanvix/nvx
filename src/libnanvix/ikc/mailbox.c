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

#if __TARGET_HAS_MAILBOX

#include <posix/errno.h>

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

	ret = kcall2(
		NR_mailbox_create,
		(word_t) local,
		(word_t) port
	);

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

	ret = kcall2(
		NR_mailbox_open,
		(word_t) remote,
		(word_t) remote_port
	);

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

	return (ret);
}

/*============================================================================*
 * kmailbox_awrite()                                                          *
 *============================================================================*/

/**
 * @details The kmailbox_awrite() asynchronously write @p size bytes
 * of data pointed to by @p buffer to the output mailbox @p mbxid.
 */
ssize_t kmailbox_awrite(int mbxid, const void *buffer, size_t size)
{
	int ret;

	/* Invalid buffer size. */
	if ((size == 0) || (size > KMAILBOX_MESSAGE_SIZE))
		return (-EINVAL);

	ret = kcall3(
		NR_mailbox_awrite,
		(word_t) mbxid,
		(word_t) buffer,
		(word_t) size
	);

	return (ret);
}

/*============================================================================*
 * kmailbox_aread()                                                           *
 *============================================================================*/

/**
 * @details The kmailbox_aread() asynchronously read @p size bytes of
 * data pointed to by @p buffer from the input mailbox @p mbxid.
 */
ssize_t kmailbox_aread(int mbxid, void *buffer, size_t size)
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
	} while (ret == -ETIMEDOUT);

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
ssize_t kmailbox_write(int mbxid, const void *buffer, size_t size)
{
	int ret;
	char buffer2[KMAILBOX_MESSAGE_SIZE];

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid buffer size. */
	if ((size == 0) || (size > KMAILBOX_MESSAGE_SIZE))
		return (-EINVAL);

	kmemcpy(buffer2, buffer, size);

	if ((ret = kmailbox_awrite(mbxid, buffer2, KMAILBOX_MESSAGE_SIZE)) < 0)
		return (ret);

#if 0
	if ((ret = kmailbox_wait(mbxid)) < 0)
		return (ret);
#endif

	return (size);
}

/*============================================================================*
 * kmailbox_read()                                                            *
 *============================================================================*/

/**
 * @details The kmailbox_read() synchronously read @p size bytes of
 * data pointed to by @p buffer from the input mailbox @p mbxid.
 *
 * @todo Uncomment kmailbox_wait() call when microkernel properly supports it.
 */
ssize_t kmailbox_read(int mbxid, void *buffer, size_t size)
{
	int ret;
	char buffer2[KMAILBOX_MESSAGE_SIZE];

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid buffer size. */
	if ((size == 0) || (size > KMAILBOX_MESSAGE_SIZE))
		return (-EINVAL);

	do
	{
		ret = kmailbox_aread(mbxid, buffer2, KMAILBOX_MESSAGE_SIZE);
		if ((ret < 0) && (ret != -ETIMEDOUT))
			return (ret);
	} while(ret < 0);

#if 0
	if ((ret = kmailbox_wait(mbxid)) < 0)
		return (ret);
#endif

	kmemcpy(buffer, buffer2, size);

	return (size);
}

/*============================================================================*
 * kmailbox_ioctl()                                                           *
 *============================================================================*/

/**
 * @details The kmailbox_ioctl() reads the measurement parameter associated
 * with the request id @p request of the mailbox @p mbxid.
 */
int kmailbox_ioctl(int mbxid, unsigned request, ...)
{
	int ret;
	va_list args;

	va_start(args, request);

		dcache_invalidate();

		ret = kcall3(
			NR_mailbox_ioctl,
			(word_t) mbxid,
			(word_t) request,
			(word_t) &args
		);

		dcache_invalidate();

	va_end(args);

	return (ret);
}

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_MAILBOX */
