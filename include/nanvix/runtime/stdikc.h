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

#ifndef NANVIX_RUNTIME_STDIKC_H_
#define NANVIX_RUNTIME_STDIKC_H_

	#include <nanvix/sys/mailbox.h>
	#include <nanvix/sys/noc.h>
	#include <nanvix/sys/portal.h>
	#include <nanvix/sys/sync.h>

/*============================================================================*
 * Kernel Standard Synchronization Point                                      *
 *============================================================================*/

	/**
	 * @brief Gets the kernel standard sync.
	 *
	 * @return The kernel standard sync.
	 */
	extern int stdsync_get(void);

	/**
	 * @brief Waits/Releases the standard kernel fence.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int stdsync_fence(void);

	/**
	 * @brief Initializes the kernel standard sync facility.
	 *
	 * @return Upon successful completion, a non-negative number is
	 * returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	extern int __stdsync_setup(void);

	/**
	 * @brief Initializes the kernel standard sync facility.
	 *
	 * @return Upon successful completion, a non-negative number is
	 * returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	extern int __stdsync_cleanup(void);

/*============================================================================*
 * Kernel Standard Mailbox                                                    *
 *============================================================================*/

	/**
	 * @brief Gets the port number of the standard input mailbox.
	 *
	 * @returns The port number where the standard input mailbox is
	 * hooked up.
	 */
	extern int stdinbox_get_port(void);

	/**
	 * @brief Gets the kernel standard input mailbox.
	 *
	 * @return The kernel standard input mailbox.
	 */
	extern int stdinbox_get(void);

	/**
	 * @brief Initializes the kernel standard mailbox facility.
	 *
	 * @return Upon successful completion, a non-negative number is
	 * returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	extern int __stdmailbox_setup(void);

	/**
	 * @brief Cleans up the kernel standard mailbox facility.
	 *
	 * @return Upon successful completion, a non-negative number is
	 * returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	extern int __stdmailbox_cleanup(void);

/*============================================================================*
 * Kernel Standard Portal                                                     *
 *============================================================================*/

	/**
	 * @brief Gets the port number of the standard input portal.
	 *
	 * @returns The port number where the standard input portal is
	 * hooked up.
	 */
	extern int stdinportal_get_port(void);

	/**
	 * @brief Returns the kernel standard input portal.
	 *
	 * @return The kernel standard input portal.
	 */
	extern int stdinportal_get(void);

	/**
	 * @brief Initializes the kernel standard portal facility.
	 *
	 * @return Upon successful completion, a non-negative number is
	 * returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	extern int __stdportal_setup(void);

	/**
	 * @brief Cleans up the kernel standard portal facility.
	 *
	 * @return Upon successful completion, a non-negative number is
	 * returned. Upon failure, a negative error code is returned
	 * instead.
	 */
	extern int __stdportal_cleanup(void);

#endif /* NANVIX_RUNTIME_STDIKC_H_ */

