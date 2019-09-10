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
#include <nanvix/sys/noc.h>
#include <posix/errno.h>

#if __TARGET_HAS_PORTAL

/*============================================================================*
 * kportal_create()                                                           *
 *============================================================================*/

/*
 * @see kernel_portal_create()
 */
int kportal_create(int local)
{
	int ret;

	/* Invalid local number for the requesting core ID. */
	if (local != knode_get_num())
		return (-EINVAL);

	ret = kcall1(
		NR_portal_create,
		(word_t) local
	);

	return (ret);
}

/*============================================================================*
 * kportal_allow()                                                            *
 *============================================================================*/

/*
 * @see kernel_portal_allow()
 */
int kportal_allow(int portalid, int remote)
{
	int ret;

	/* Invalid remote number for the requesting core ID. */
	if (remote == knode_get_num())
		return (-EINVAL);

	ret = kcall2(
		NR_portal_allow,
		(word_t) portalid,
		(word_t) remote
	);

	return (ret);
}

/*============================================================================*
 * kportal_open()                                                             *
 *============================================================================*/

/*
 * @see kernel_portal_open()
 */
int kportal_open(int local, int remote)
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

	/* Invalid remote number for the requesting core ID. */
	if (remote == knode_get_num())
		return (-EINVAL);

	ret = kcall2(
		NR_portal_open,
		(word_t) local,
		(word_t) remote
	);

	return (ret);
}

/*============================================================================*
 * kportal_unlink()                                                           *
 *============================================================================*/

/*
 * @see kernel_portal_unlink()
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

/*
 * @see kernel_portal_close()
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

/*
 * @see kernel_portal_aread()
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

	ret = kcall3(
		NR_portal_aread,
		(word_t) portalid,
		(word_t) buffer,
		(word_t) size
	);

	return (ret);
}

/*============================================================================*
 * kportal_awrite()                                                           *
 *============================================================================*/

/*
 * @see kernel_portal_awrite()
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

	ret = kcall3(
		NR_portal_awrite,
		(word_t) portalid,
		(word_t) buffer,
		(word_t) size
	);

	return (ret);
}

/*============================================================================*
 * kportal_wait()                                                             *
 *============================================================================*/

/*
 * @see kernel_portal_wait()
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
 */
ssize_t kportal_write(int portalid, const void *buffer, size_t size)
{
	int ret;

	if ((ret = kportal_awrite(portalid, buffer, size)) < 0)
		return (ret);

	if ((ret = kportal_wait(portalid)) < 0)
		return (ret);

	return (size);
}

/*============================================================================*
 * kportal_read()                                                             *
 *============================================================================*/

/**
 * @details The kportal_read() synchronously read @p size bytes of
 * data pointed to by @p buffer from the input portal @p portalid.
 */
ssize_t kportal_read(int portalid, void *buffer, size_t size)
{
	int ret;

	if ((ret = kportal_aread(portalid, buffer, size)) < 0)
		return (ret);

	if ((ret = kportal_wait(portalid)) < 0)
		return (ret);

	return (size);
}

#endif /* __TARGET_HAS_PORTAL */
