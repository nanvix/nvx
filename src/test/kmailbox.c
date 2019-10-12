/*
 * MIT License
 *
 * Copyright(c) 2011-2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
 *              2015-2016 Davidson Francis     <davidsondfgl@gmail.com>
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

#include <nanvix/sys/mailbox.h>
#include <nanvix/sys/noc.h>
#include <posix/errno.h>

#include "test.h"

#if __TARGET_HAS_MAILBOX

/**
 * @brief Test's parameters
 */
#define NR_NODES       2
#define NR_NODES_MAX   PROCESSOR_NOC_NODES_NUM
#define MASTER_NODENUM 0
#ifdef __mppa256__
	#define SLAVE_NODENUM  8
#else
	#define SLAVE_NODENUM  1
#endif

/*============================================================================*
 * API Test: Create Unlink                                                    *
 *============================================================================*/

/**
 * @brief API Test: Mailbox Create Unlink
 */
static void test_api_mailbox_create_unlink(void)
{
	int mbxid;
	int local;

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local)) >= 0);
	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * API Test: Create Unlink                                                    *
 *============================================================================*/

/**
 * @brief API Test: Mailbox Create Unlink
 */
static void test_api_mailbox_open_close(void)
{
	int mbxid;
	int remote;

	remote = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_open(remote)) >= 0);
	test_assert(kmailbox_close(mbxid) == 0);
}

/*============================================================================*
 * API Test: Get volume                                                       *
 *============================================================================*/

/**
 * @brief API Test: Mailbox Get volume
 */
static void test_api_mailbox_get_volume(void)
{
	int local;
	int remote;
	int mbx_in;
	int mbx_out;
	size_t volume;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbx_in = kmailbox_create(local)) >= 0);
	test_assert((mbx_out = kmailbox_open(remote)) >= 0);

		test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == 0);

		test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == 0);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * API Test: Get latency                                                      *
 *============================================================================*/

/**
 * @brief API Test: Mailbox Get latency
 */
static void test_api_mailbox_get_latency(void)
{
	int local;
	int remote;
	int mbx_in;
	int mbx_out;
	uint64_t latency;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbx_in = kmailbox_create(local)) >= 0);
	test_assert((mbx_out = kmailbox_open(remote)) >= 0);

		test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);

		test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * API Test: Read Write 2 CC                                                  *
 *============================================================================*/

/**
 * @brief API Test: Read Write 2 CC
 */
static void test_api_mailbox_read_write(void)
{
	int local;
	int remote;
	int mbx_in;
	int mbx_out;
	size_t volume;
	uint64_t latency;
	char message[MAILBOX_MSG_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbx_in = kmailbox_create(local)) >= 0);
	test_assert((mbx_out = kmailbox_open(remote)) >= 0);

	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);

	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);

	if (local == MASTER_NODENUM)
	{
		for (unsigned i = 0; i < NITERATIONS; i++)
		{
			kmemset(message, 1, MAILBOX_MSG_SIZE);

			test_assert(kmailbox_awrite(mbx_out, message, MAILBOX_MSG_SIZE) == MAILBOX_MSG_SIZE);
			test_assert(kmailbox_wait(mbx_out) == 0);

			kmemset(message, 0, MAILBOX_MSG_SIZE);

			test_assert(kmailbox_aread(mbx_in, message, MAILBOX_MSG_SIZE) == MAILBOX_MSG_SIZE);
			test_assert(kmailbox_wait(mbx_in) == 0);

			for (unsigned j = 0; j < MAILBOX_MSG_SIZE; ++j)
				test_assert(message[j] == 2);
		}
	}
	else
	{
		for (unsigned i = 0; i < NITERATIONS; i++)
		{
			kmemset(message, 0, MAILBOX_MSG_SIZE);

			test_assert(kmailbox_aread(mbx_in, message, MAILBOX_MSG_SIZE) == MAILBOX_MSG_SIZE);
			test_assert(kmailbox_wait(mbx_in) == 0);

			for (unsigned j = 0; j < MAILBOX_MSG_SIZE; ++j)
				test_assert(message[j] == 1);

			kmemset(message, 2, MAILBOX_MSG_SIZE);

			test_assert(kmailbox_awrite(mbx_out, message, MAILBOX_MSG_SIZE) == MAILBOX_MSG_SIZE);
			test_assert(kmailbox_wait(mbx_out) == 0);
		}
	}

	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * MAILBOX_MSG_SIZE));
	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * MAILBOX_MSG_SIZE));
	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Create                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Create
 */
static void test_fault_mailbox_invalid_create(void)
{
	test_assert(kmailbox_create(-1) == -EINVAL);
	test_assert(kmailbox_create(PROCESSOR_NOC_NODES_NUM) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Create                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Create
 */
static void test_fault_mailbox_bad_create(void)
{
	int nodenum;

	nodenum = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert(kmailbox_create(nodenum) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Invalid Unlink                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Unlink
 */
static void test_fault_mailbox_invalid_unlink(void)
{
	test_assert(kmailbox_unlink(-1) == -EINVAL);
	test_assert(kmailbox_unlink(MAILBOX_CREATE_MAX) == -EBADF);
	test_assert(kmailbox_unlink(1000000) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Double Unlink                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Double Unlink
 */
static void test_fault_mailbox_double_unlink(void)
{
	int local;
	int mbxid;

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local)) >= 0);
	test_assert(kmailbox_unlink(mbxid) == 0);
	test_assert(kmailbox_unlink(mbxid) == -EBADF);
}

/*============================================================================*
 * Fault Test: Invalid Open                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Open
 */
static void test_fault_mailbox_invalid_open(void)
{
	test_assert(kmailbox_open(-1) == -EINVAL);
	test_assert(kmailbox_open(PROCESSOR_NOC_NODES_NUM) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Open                                                       *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Open
 */
static void test_fault_mailbox_bad_open(void)
{
	int nodenum;

	nodenum = knode_get_num();

#ifdef __mppa256__
	if (processor_noc_is_cnode(nodenum))
#endif
		test_assert(kmailbox_open(nodenum) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Invalid Close                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Close
 */
static void test_fault_mailbox_invalid_close(void)
{
	test_assert(kmailbox_close(-1) == -EINVAL);
	test_assert(kmailbox_close(MAILBOX_OPEN_MAX) == -EBADF);
	test_assert(kmailbox_close(1000000) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Double Close                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Double Close
 */
static void test_fault_mailbox_double_close(void)
{
	int mbxid;
	int nodenum;

	nodenum = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_open(nodenum)) >= 0);
	test_assert(kmailbox_close(mbxid) == 0);
	test_assert(kmailbox_close(mbxid) == -EBADF);
}

/*============================================================================*
 * Fault Test: Bad Close                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Close
 */
static void test_fault_mailbox_bad_close(void)
{
	int local;
	int mbxid;

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local)) >= 0);

		test_assert(kmailbox_close(mbxid) == -EBADF);

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Read                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Read
 */
static void test_fault_mailbox_invalid_read(void)
{
	char buffer[MAILBOX_MSG_SIZE];

	test_assert(kmailbox_aread(-1, buffer, MAILBOX_MSG_SIZE) == -EINVAL);
	test_assert(kmailbox_aread(MAILBOX_CREATE_MAX, buffer, MAILBOX_MSG_SIZE) == -EBADF);
	test_assert(kmailbox_aread(1000000, buffer, MAILBOX_MSG_SIZE) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Invalid Read Size                                              *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Read Size
 */
static void test_fault_mailbox_invalid_read_size(void)
{
	int mbxid;
	int local;
	char buffer[MAILBOX_MSG_SIZE];

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local)) >= 0);

		test_assert(kmailbox_aread(mbxid, buffer, -1) == -EINVAL);
		test_assert(kmailbox_aread(mbxid, buffer, 0) == -EINVAL);
		test_assert(kmailbox_aread(mbxid, buffer, MAILBOX_MSG_SIZE - 1) == -EINVAL);
		test_assert(kmailbox_aread(mbxid, buffer, MAILBOX_MSG_SIZE + 1) == -EINVAL);

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Null Read                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Null Read
 */
static void test_fault_mailbox_null_read(void)
{
	int mbxid;
	int local;

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local)) >= 0);

		test_assert(kmailbox_aread(mbxid, NULL, MAILBOX_MSG_SIZE) == -EINVAL);

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Write                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Write
 */
static void test_fault_mailbox_invalid_write(void)
{
	char buffer[MAILBOX_MSG_SIZE];

	test_assert(kmailbox_awrite(-1, buffer, MAILBOX_MSG_SIZE) == -EINVAL);
	test_assert(kmailbox_awrite(MAILBOX_OPEN_MAX, buffer, MAILBOX_MSG_SIZE) == -EBADF);
	test_assert(kmailbox_awrite(1000000, buffer, MAILBOX_MSG_SIZE) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Write                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Write
 */
static void test_fault_mailbox_bad_write(void)
{
	int mbxid;
	int local;
	char buffer[MAILBOX_MSG_SIZE];

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local)) >= 0);

		test_assert(kmailbox_awrite(mbxid, buffer, MAILBOX_MSG_SIZE) == -EBADF);

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Bad Wait                                                       *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Write
 */
static void test_fault_mailbox_bad_wait(void)
{
	test_assert(kmailbox_wait(-1) == -EINVAL);
#ifndef __unix64__
	test_assert(kmailbox_wait(MAILBOX_CREATE_MAX) == -EBADF);
	test_assert(kmailbox_wait(MAILBOX_OPEN_MAX) == -EBADF);
#endif
	test_assert(kmailbox_wait(1000000) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Invalid ioctl                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid ioctl
 */
static void test_fault_mailbox_invalid_ioctl(void)
{
	int local;
	int mbxid;
	size_t volume;
	uint64_t latency;

	test_assert(kmailbox_ioctl(-1, MAILBOX_IOCTL_GET_VOLUME, &volume) == -EINVAL);
	test_assert(kmailbox_ioctl(-1, MAILBOX_IOCTL_GET_LATENCY, &latency) == -EINVAL);
	test_assert(kmailbox_ioctl(1000000, MAILBOX_IOCTL_GET_VOLUME, &volume) == -EINVAL);
	test_assert(kmailbox_ioctl(1000000, MAILBOX_IOCTL_GET_LATENCY, &latency) == -EINVAL);

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local)) >=  0);

		test_assert(kmailbox_ioctl(mbxid, -1, &volume) == -ENOTSUP);

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Bad ioctl                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Bad ioctl
 */
static void test_fault_mailbox_bad_ioctl(void)
{
	size_t volume;

	test_assert(kmailbox_ioctl(0, MAILBOX_IOCTL_GET_VOLUME, &volume) == -EBADF);
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief Unit tests.
 */
static struct test mailbox_tests_api[] = {
	{ test_api_mailbox_create_unlink, "[test][mailbox][api] mailbox create unlink [passed]\n" },
	{ test_api_mailbox_open_close,    "[test][mailbox][api] mailbox open close    [passed]\n" },
	{ test_api_mailbox_get_volume,    "[test][mailbox][api] mailbox get volume    [passed]\n" },
	{ test_api_mailbox_get_latency,   "[test][mailbox][api] mailbox get latency   [passed]\n" },
	{ test_api_mailbox_read_write,    "[test][mailbox][api] mailbox read write    [passed]\n" },
	{ NULL,                            NULL                                                   },
};

/**
 * @brief Unit tests.
 */
static struct test mailbox_tests_fault[] = {
	{ test_fault_mailbox_invalid_create,    "[test][mailbox][fault] mailbox invalid create    [passed]\n" },
	{ test_fault_mailbox_bad_create,        "[test][mailbox][fault] mailbox bad create        [passed]\n" },
	{ test_fault_mailbox_invalid_unlink,    "[test][mailbox][fault] mailbox invalid unlink    [passed]\n" },
	{ test_fault_mailbox_double_unlink,     "[test][mailbox][fault] mailbox double unlink     [passed]\n" },
	{ test_fault_mailbox_invalid_open,      "[test][mailbox][fault] mailbox invalid open      [passed]\n" },
	{ test_fault_mailbox_bad_open,          "[test][mailbox][fault] mailbox bad open          [passed]\n" },
	{ test_fault_mailbox_invalid_close,     "[test][mailbox][fault] mailbox invalid close     [passed]\n" },
	{ test_fault_mailbox_double_close,      "[test][mailbox][fault] mailbox double close      [passed]\n" },
	{ test_fault_mailbox_bad_close,         "[test][mailbox][fault] mailbox bad close         [passed]\n" },
	{ test_fault_mailbox_invalid_read,      "[test][mailbox][fault] mailbox invalid read      [passed]\n" },
	{ test_fault_mailbox_invalid_read_size, "[test][mailbox][fault] mailbox invalid read size [passed]\n" },
	{ test_fault_mailbox_null_read,         "[test][mailbox][fault] mailbox null read         [passed]\n" },
	{ test_fault_mailbox_invalid_write,     "[test][mailbox][fault] mailbox invalid write     [passed]\n" },
	{ test_fault_mailbox_bad_write,         "[test][mailbox][fault] mailbox bad write         [passed]\n" },
	{ test_fault_mailbox_bad_wait,          "[test][mailbox][fault] mailbox bad wait          [passed]\n" },
	{ test_fault_mailbox_invalid_ioctl,     "[test][mailbox][fault] mailbox invalid ioctl     [passed]\n" },
	{ test_fault_mailbox_bad_ioctl,         "[test][mailbox][fault] mailbox bad ioctl         [passed]\n" },
	{ NULL,                                  NULL                                                         },
};

/**
 * The test_thread_mgmt() function launches testing units on thread manager.
 *
 * @author Pedro Henrique Penna
 */
void test_mailbox(void)
{
	int nodenum;

	nodenum = knode_get_num();

	if (nodenum == MASTER_NODENUM || nodenum == SLAVE_NODENUM)
	{
		/* API Tests */
		if (nodenum == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------\n");
		for (unsigned i = 0; mailbox_tests_api[i].test_fn != NULL; i++)
		{
			mailbox_tests_api[i].test_fn();

			if (nodenum == MASTER_NODENUM)
				nanvix_puts(mailbox_tests_api[i].name);
		}

		/* Fault Tests */
		if (nodenum == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------\n");
		for (unsigned i = 0; mailbox_tests_fault[i].test_fn != NULL; i++)
		{
			mailbox_tests_fault[i].test_fn();

			if (nodenum == MASTER_NODENUM)
				nanvix_puts(mailbox_tests_fault[i].name);
		}
	}
}

#endif /* __TARGET_HAS_MAILBOX */
