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

#ifndef NANVIX_SYS_PAGE_H_
#define NANVIX_SYS_PAGE_H_

	#include <nanvix/kernel/syscall.h>

	/**
	 * @brief Allocates a user page.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int page_alloc(vaddr_t vaddr);

	/**
	 * @brief Releases a user page.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 *
	 * @see upage_alloc().
	 */
	extern int page_free(vaddr_t vaddr);

	/**
	 * @brief Releases a user page.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 *
	 * @see upage_alloc().
	 */
	EXTERN int kernel_upage_free(vaddr_t vaddr);

	/**
	 * @brief Maps a page frame into a page.
	 *
	 * @param vaddr Target virtual address.
	 * @param frame Target page frame.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int page_map(vaddr_t vaddr, frame_t frame);

	/**
	 * @brief Unmaps a page frame.
	 *
	 * @param vaddr Target virtual address.
	 *
	 * @returns Upon successful completion, zero is returned. Upon
	 * failure, a negative error code is returned instead.
	 */
	extern int page_unmap(vaddr_t vaddr);

#endif /* NANVIX_SYS_PAGE_H_ */
