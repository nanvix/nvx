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

#ifndef NANVIX_SYS_EXCP_H_
#define NANVIX_SYS_EXCP_H_

	#include <nanvix/kernel/kernel.h>

	/**
	 * @brief Sets a user-space exception handler.
	 *
	 * @param excpnum Number of the target exception.
	 * @param action  Action upon target exception.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure a negative error code is returned instead.
	 */
	extern int excp_ctrl(int excpnum, int action);

#if __NANVIX_USE_EXCEPTION_WITH_TASKS

	/**
	 * @brief Sets a user-space exception handler.
	 *
	 * @param excpnum Number of the target exception.
	 * @param handler Function handler.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure a negative error code is returned instead.
	 */
	extern int excp_set_handler(int excpnum, exception_handler_fn handler);

#endif /* __NANVIX_USE_EXCEPTION_WITH_TASKS */

	/**
	 * @brief Pauses the user-space exception handler.
	 *
	 * @param excp Information about the exception
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure a negative error code is returned instead.
	 */
	extern int excp_pause(struct exception *excp);

	/**
	 * @brief Resumes a kernel-space exception handler.
	 *
	 * @return Upon successful completion, zero is returned. Upon
	 * failure a negative error code is returned instead.
	 */
	extern int excp_resume(void);

#endif /* NANVIX_SYS_EXCP_H_ */
