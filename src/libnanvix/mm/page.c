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

/**
 * TODO: provide a detailed description for this function.
 */
int page_alloc(vaddr_t vaddr)
{
	int ret;

	/* TODO: sanity check parameters. */

	ret = kcall1(
		NR_upage_alloc,
		(word_t) vaddr
	);

	return (ret);
}

/**
 * TODO: provide a detailed description for this function.
 */
int page_free(vaddr_t vaddr)
{
	int ret;

	/* TODO: sanity check parameters. */

	ret = kcall1(
		NR_upage_free,
		(word_t) vaddr
	);

	return (ret);
}

/**
 * TODO: provide a detailed description for this function.
 */
int page_map(vaddr_t vaddr, frame_t frame)
{
	int ret;

	ret = kcall2(
		NR_upage_map,
		(word_t) vaddr,
		(word_t) frame
	);

	return (ret);
}

/**
 * TODO: provide a detailed description for this function.
 */
int page_unmap(vaddr_t vaddr)
{
	int ret;

	ret = kcall1(
		NR_upage_unmap,
		(word_t) vaddr
	);

	return (ret);
}

/**
 * TODO: provide a detailed description for this function.
 */
int page_link(vaddr_t vaddr1, vaddr_t vaddr2)
{
	int ret;

	ret = kcall2(
		NR_upage_link,
		(word_t) vaddr1,
		(word_t) vaddr2
	);

	return (ret);
}
