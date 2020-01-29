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
int excp_ctrl(int excpnum, int action)
{
	int ret;

	/* TODO: sanity check parameters. */

	ret = kcall2(
		NR_excp_ctrl,
		(word_t) excpnum,
		(word_t) action
	);

	return (ret);
}


/**
 * TODO: provide a detailed description for this function.
 */
int excp_pause(struct exception *excp)
{
	int ret;

	/* TODO: sanity check parameters. */

	ret = kcall1(
		NR_excp_pause,
		(word_t) excp
	);

	return (ret);
}

/**
 * TODO: provide a detailed description for this function.
 */
int excp_resume(void)
{
	int ret;

	/* TODO: sanity check parameters. */

	ret = kcall0(
		NR_excp_resume
	);

	return (ret);
}
