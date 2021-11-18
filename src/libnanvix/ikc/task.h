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

#ifndef NANVIX_SYS_IKC_FLOW_H_
#define NANVIX_SYS_IKC_FLOW_H_

	#include <nanvix/kernel/kernel.h>
	#include <nanvix/sys/mailbox.h>
	#include <nanvix/sys/portal.h>
	#include <nanvix/sys/task.h>

#if __NANVIX_USE_COMM_WITH_TASKS

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

	#define IKC_FLOW_MAILBOX_READ  (0)
	#define IKC_FLOW_MAILBOX_WRITE (1)
	#define IKC_FLOW_PORTAL_READ   (2)
	#define IKC_FLOW_PORTAL_WRITE  (3)
	#define IKC_FLOW_INVALID       (4)

	#define IKC_FLOW_MAILBOX       IKC_FLOW_MAILBOX_READ
	#define IKC_FLOW_PORTAL        IKC_FLOW_PORTAL_READ

/*============================================================================*
 * General communication functions                                            *
 *============================================================================*/

	EXTERN ssize_t __kmailbox_awrite(int, const void *, size_t);
	EXTERN ssize_t __kmailbox_aread(int, void *, size_t);
	EXTERN int __kmailbox_wait(int);

	EXTERN ssize_t __kportal_awrite(int, const void *, size_t);
	EXTERN ssize_t __kportal_aread(int, void *, size_t);
	EXTERN int __kportal_wait(int);

/*============================================================================*
 * IKC Flow communication functions                                           *
 *============================================================================*/

	EXTERN int ikc_flow_config(int, word_t, word_t, word_t, word_t, word_t);
	EXTERN int ikc_flow_wait(int, word_t);
	EXTERN void ikc_flow_init(void);

#endif /* __NANVIX_USE_COMM_WITH_TASKS */
#endif /* NANVIX_SYS_IKC_FLOW_H_ */

