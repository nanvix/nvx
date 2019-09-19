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
#include <errno.h>

/*============================================================================*
 * knode_get_num()                                                            *
 *============================================================================*/

/*
 * @see kernel_node_get_num()
 */
int knode_get_num(void)
{
	int ret;
    int coreid;

    coreid = core_get_id();

	ret = kcall1(
		NR_node_get_num,
		(word_t) coreid
	);

	return (ret);
}

/*============================================================================*
 * knode_set_num()                                                            *
 *============================================================================*/

/*
 * @see kernel_node_set_num()
 */
int knode_set_num(int nodenum)
{
	int ret;
    int coreid;

    if (!WITHIN(nodenum, 0, PROCESSOR_NOC_NODES_NUM))
        return (-EINVAL);

    coreid = core_get_id();

	ret = kcall2(
		NR_node_set_num,
		(word_t) coreid,
		(word_t) nodenum
	);

	return (ret);
}
