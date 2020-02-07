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

#if __TARGET_HAS_PORTAL

#include <nanvix/sys/noc.h>
#include <posix/errno.h>

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

	/* Invalid remote number for the requesting core ID. */
	if (remote == knode_get_num())
		return (-EINVAL);

	ret = kcall3(
		NR_portal_allow,
		(word_t) portalid,
		(word_t) remote,
		(word_t) remote_port
	);

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
	int nodenum;

	nodenum = knode_get_num();

	/* Invalid local number for the requesting core ID. */
	if (local != nodenum)
		return (-EINVAL);

	/* Invalid remote number for the requesting core ID. */
	if (remote == nodenum)
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

/**
 * @details The kportal_aread() asynchronously read @p size bytes of
 * data pointed to by @p buffer from the input portal @p portalid.
 */
int kportal_aread(int portalid, void * buffer, size_t size)
{
	int ret;

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid size. */
	if (size == 0 || size > PORTAL_MAX_SIZE)
		return (-EINVAL);

	do
	{
		ret = kcall3(
			NR_portal_aread,
			(word_t) portalid,
			(word_t) buffer,
			(word_t) size);
	} while (ret == -EBUSY);

	return (ret);
}

/*============================================================================*
 * kportal_awrite()                                                           *
 *============================================================================*/

/**
 * @details The kportal_awrite() asynchronously write @p size bytes
 * of data pointed to by @p buffer to the output portal @p portalid.
 */
int kportal_awrite(int portalid, const void * buffer, size_t size)
{
	int ret;

	/* Invalid buffer. */
	if (buffer == NULL)
		return (-EINVAL);

	/* Invalid size. */
	if (size == 0 || size > PORTAL_MAX_SIZE)
		return (-EINVAL);

	do
	{
		ret = kcall3(
			NR_portal_awrite,
			(word_t) portalid,
			(word_t) buffer,
			(word_t) size);
	} while (ret == -EAGAIN);

	return (ret);
}

/*============================================================================*
 * kportal_wait()                                                             *
 *============================================================================*/

/**
 * @details The kportal_wait() waits for asyncrhonous operations in
 * the input/output portal @p portalid to complete.
 */
int kportal_wait(int portalid)
{
	int ret;

	ret = kcall1(
		NR_portal_wait,
		(word_t) portalid
	);

	return (ret);
}

/*============================================================================*
 * kportal_write()                                                            *
 *============================================================================*/

/**
 * @details The kportal_write() synchronously write @p size bytes of
 * data pointed to by @p buffer to the output portal @p portalid.
 *
 * @bug FIXME: Call kportal_wait() when the kernel properly supports it.
 */
ssize_t kportal_write(int portalid, const void *buffer, size_t size)
{
	int ret;

	if ((ret = kportal_awrite(portalid, buffer, size)) < 0)
		return (ret);

#if 0
	if ((ret = kportal_wait(portalid)) < 0)
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
 *
 * @bug FIXME: Call kportal_wait() when the kernel properly supports it.
 */
ssize_t kportal_read(int portalid, void *buffer, size_t size)
{
	int ret;

	if ((ret = kportal_aread(portalid, buffer, size)) < 0)
		return (ret);

#if 0
	if ((ret = kportal_wait(portalid)) < 0)
		return (ret);
#endif

	return (size);
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

#else
extern int make_iso_compilers_happy;
#endif /* __TARGET_HAS_PORTAL */
