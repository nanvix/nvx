/*
 * MIT License
 *
 * Copyright(c) 2011-2020 The Maintainers of Nanvix
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

/**
 * @addtogroup nanvix Nanvix System
 */
/**@{*/

#ifndef NANVIX_SYS_MAILBOX_H_
#define NANVIX_SYS_MAILBOX_H_

	#include <nanvix/kernel/kernel.h>
	#include <posix/sys/types.h>

	/**
	 * @brief Creates an input mailbox.
	 *
	 * @param local Target local NoC node.
	 *
	 * @return Upon successful completion, the ID of the created input
	 * mailbox is returned. Upon failure, a negative error code is
	 * returned instead.
	 */
	extern int kmailbox_create(int local);

	/**
	 * @brief Opens an output mailbox
	 *
	 * @param local Target remote NoC node.
	 *
	 * @return Upon successful completion, the ID of the opened output
	 * mailbox is returned. Upon failure, a negative error code is
	 * returned instead.
	 */
	extern int kmailbox_open(int remote);

	/**
	 * @brief Removes an input mailbox.
	 *
	 * @param mbxid ID of the target input mailbox.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int kmailbox_unlink(int mbxid);

	/**
	 * @brief Closes an output mailbox.
	 *
	 * @param mbxid ID of the target output mailbox.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int kmailbox_close(int);

	/**
	 * @brief Asynchronously write to an output mailbox.
	 *
	 * @param mbxid  ID of the target output mailbox.
	 * @param buffer Target data buffer.
	 * @param size   Size in bytes of the data buffer.
	 *
	 * @return Upon successful completion, the number of bytes written
	 * to the output mailbox @p mbxid is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern ssize_t kmailbox_awrite(int mbxid, const void *buffer, size_t size);

	/**
	 * @brief Synchronously write to an output mailbox.
	 *
	 * @param mbxid  ID of the target output mailbox.
	 * @param buffer Target data buffer.
	 * @param size   Size in bytes of the data buffer.
	 *
	 * @return Upon successful completion, the number of bytes written
	 * to the output mailbox @p mbxid is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern ssize_t kmailbox_write(int mbxid, const void *buffer, size_t size);

	/**
	 * @brief Asynchronously read from an input mailbox.
	 *
	 * @param mbxid  ID of the target input mailbox.
	 * @param buffer Target data buffer.
	 * @param size   Size in bytes of the data buffer.
	 *
	 * @return Upon successful completion, the number of bytes read
	 * from the input mailbox @p mbxid is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern ssize_t kmailbox_aread(int mbxid, void *buffer, size_t size);

	/**
	 * @brief Synchronously read from an input mailbox.
	 *
	 * @param mbxid  ID of the target input mailbox.
	 * @param buffer Target data buffer.
	 * @param size   Size in bytes of the data buffer.
	 *
	 * @return Upon successful completion, the number of bytes read
	 * from the input mailbox @p mbxid is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern ssize_t kmailbox_read(int mbxid, void *buffer, size_t size);

	/**
	 * @brief Waits for an synchronous operation to complete.
	 *
	 * @param mbxid ID of the target input/output mailbox.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int kmailbox_wait(int mbxid);

	/**
	 * @brief Performs control operations in a mailbox.
	 *
	 * @param mbxid   Target mailbox.
	 * @param request Request.
	 * @param ...     Additional arguments.
	 *
	 * @param Upon successful completion, zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 */
	extern int kmailbox_ioctl(int, unsigned, ...);

#endif /* NANVIX_SYS_MAILBOX_H_ */

/**@}*/
