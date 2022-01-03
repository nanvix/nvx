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

#include <nanvix/sys/dev.h>
#include <nanvix/sys/noc.h>
#include "test.h"

/**
 * @name Configuration of the routines (Involved clusters)
 */
/**@{*/
const int _nodenums[NR_NODES] ALIGN(sizeof(uint64_t)) = {
	MASTER_NODENUM, SLAVE_NODENUM, SLAVE_NODENUM + 1
};
/**@}*/

#ifdef __mppa256__

/**
 * @brief Stub main().
 */
int main(int argc, const char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	return (0);
}

#endif /* __mppa256__ */

/*============================================================================*
 * strlen()                                                                   *
 *============================================================================*/

/**
 * @brief Returns the length of a string.
 *
 * @param str String to be evaluated.
 *
 * @returns The length of the string.
 */
PRIVATE size_t nanvix_strlen(const char *str)
{
	const char *p;

	/* Count the number of characters. */
	for (p = str; *p != '\0'; p++)
		/* noop */;

	return (p - str);
}

/*============================================================================*
 * nanvix_puts()                                                              *
 *============================================================================*/

/**
 * The nanvix_puts() function writes to the standard output device the string
 * pointed to by @p str.
 */
void nanvix_puts(const char *str)
{
	if (knode_get_num() != MASTER_NODENUM)
		return;

	nanvix_write(0, str, nanvix_strlen(str));
}

/*============================================================================*
 * main()                                                                     *
 *============================================================================*/

/**
 * @brief Lunches user-land testing units.
 *
 * @param argc Argument counter.
 * @param argv Argument variables.
 */
void ___start(int argc, const char *argv[])
{
	int index;
	int nodenum;

	/* Required. */
	knoc_init();

	((void) argc);
	((void) argv);

	index   = -1;
	nodenum = knode_get_num();

	/* Finds the index of local node on nodenums vector. */
	for (int i = 0; i < NR_NODES; ++i)
	{
		if (nodenum == _nodenums[i])
		{
			index = i;
			break;
		}
	}

	/* Only involved nodes. */
	if (index >= 0)
		test_migration();

	/* Halt. */
	kshutdown();
	UNREACHABLE();
}

