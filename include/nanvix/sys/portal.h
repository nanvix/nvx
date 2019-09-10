/*
 * MIT License
 *
 * Copyright(c) 2011-2019 The Maintainers of Nanvix
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

#ifndef NANVIX_SYS_PORTAL_H_
#define NANVIX_SYS_PORTAL_H_

	#include <nanvix/kernel/kernel.h>
	#include <sys/types.h>
	#include <stdbool.h>
	#include <stdint.h>

	/**
	 * @brief Creates a portal.
	 *
	 * @param local Logic ID of the Local Node.
	 *
	 * @returns Upon successful completion, the ID of a newly created
	 * portal is returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	extern int kportal_create(int local);

	/**
	 * @brief Enables read operations from a remote.
	 *
	 * @param portalid ID of the Target Portal.
	 * @param remote   Logic ID of Target Node.
	 *
	 * @returns Upons successful completion zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 */
	extern int kportal_allow(int portalid, int remote);

	/**
	 * @brief Opens a portal.
	 *
	 * @param local  Logic ID of the local NoC node.
	 * @param remote Logic ID of the target NoC node.
	 *
	 * @returns Upon successful completion, the ID of the target portal is
	 * returned. Upon failure, a negative error code is returned instead.
	 */
	extern int kportal_open(int local, int remote);

	/**
	 * @brief Destroys a portal.
	 *
	 * @param portalid ID of the Target Portal.
	 *
	 * @returns Upon successful completion zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int kportal_unlink(int portalid);
	
	/**
	 * @brief Closes a portal.
	 *
	 * @param portalid ID of the Target Portal.
	 *
	 * @returns Upon successful completion zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int kportal_close(int portalid);

	/**
	 * @brief Reads data from a portal.
	 *
	 * @param portalid ID of the Target Portal.
	 * @param buffer   Location from where data should be written.
	 * @param size     Number of bytes to read.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int kportal_read(int portalid, void * buffer, size_t size);

	/**
	 * @brief Reads data asynchronously from a portal.
	 *
	 * @param portalid ID of the Target Portal.
	 * @param buffer   Location from where data should be written.
	 * @param size     Number of bytes to read.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int kportal_aread(int portalid, void * buffer, size_t size);

	/**
	 * @brief Writes data to a portal.
	 *
	 * @param portalid ID of the Target Portal.
	 * @param buffer   Location from where data should be read.
	 * @param size     Number of bytes to write.
	 *
	 * @returns Upon successful, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int kportal_awrite(int portalid, const void * buffer, size_t size);

	/**
	 * @brief Writes data asynchronously to a portal.
	 *
	 * @param portalid ID of the Target Portal.
	 * @param buffer   Location from where data should be read.
	 * @param size     Number of bytes to write.
	 *
	 * @returns Upon successful, zero is returned. Upon failure, a
	 * negative error code is returned instead.
	 */
	extern int kportal_awrite(int portalid, const void * buffer, size_t size);

	/**
	 * @brief Waits for an asynchronous operation on a portal to complete.
	 *
	 * @param portalid ID of the Target Portal.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int kportal_wait(int portalid);

	/**
	 * @brief Performs control operations in a portal.
	 *
	 * @param mbxid   Target portal.
	 * @param request Request.
	 * @param ...     Additional arguments.
	 *
	 * @param Upon successful completion, zero is returned. Upon failure,
	 * a negative error code is returned instead.
	 */
	extern int kportal_ioctl(int portalid, unsigned request, ...);

#endif /* NANVIX_SYS_PORTAL_H_ */

/**@}*/
