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

#include <nanvix/sys/thread.h>
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

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);
	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * API Test: Open Close                                                    *
 *============================================================================*/

/**
 * @brief API Test: Mailbox Open Close
 */
static void test_api_mailbox_open_close(void)
{
	int mbxid;
	int remote;

	remote = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);
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

	test_assert((mbx_in = kmailbox_create(local, 0)) >= 0);
	test_assert((mbx_out = kmailbox_open(remote, 0)) >= 0);

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

	test_assert((mbx_in = kmailbox_create(local, 0)) >= 0);
	test_assert((mbx_out = kmailbox_open(remote, 0)) >= 0);

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
	char message[KMAILBOX_MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbx_in = kmailbox_create(local, 0)) >= 0);
	test_assert((mbx_out = kmailbox_open(remote, 0)) >= 0);

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
			kmemset(message, 1, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_write(mbx_out, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			kmemset(message, 0, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_read(mbx_in, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (unsigned j = 0; j < KMAILBOX_MESSAGE_SIZE; ++j)
				test_assert(message[j] == 2);
		}
	}
	else
	{
		for (unsigned i = 0; i < NITERATIONS; i++)
		{
			kmemset(message, 0, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_read(mbx_in, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (unsigned j = 0; j < KMAILBOX_MESSAGE_SIZE; ++j)
				test_assert(message[j] == 1);

			kmemset(message, 2, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_write(mbx_out, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
		}
	}

	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * KMAILBOX_MESSAGE_SIZE));
	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * KMAILBOX_MESSAGE_SIZE));
	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * API Test: Virtualization                                                   *
 *============================================================================*/

#define TEST_VIRTUALIZATION_MBX_NR MAILBOX_PORT_NR

/**
 * @brief API Test: Virtualization.
 */
static void test_api_mailbox_virtualization(void)
{
	int local;
	int remote;
	int mbx_in[TEST_VIRTUALIZATION_MBX_NR];
	int mbx_out[TEST_VIRTUALIZATION_MBX_NR];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	/* Creates multiple virtual mailboxes. */
	for (int i = 0; i < TEST_VIRTUALIZATION_MBX_NR; ++i)
	{
		test_assert((mbx_in[i] = kmailbox_create(local, i)) >= 0);
		test_assert((mbx_out[i] = kmailbox_open(remote, i)) >= 0);
	}

	/* Deletion of the created virtual mailboxes. */
	for (int i = 0; i < TEST_VIRTUALIZATION_MBX_NR; ++i)
	{
		test_assert(kmailbox_unlink(mbx_in[i]) == 0);
		test_assert(kmailbox_close(mbx_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Multiplexation                                                   *
 *============================================================================*/

#define TEST_MULTIPLEXATION_MBX_PAIRS MAILBOX_PORT_NR

/**
 * @brief API Test: Multiplex of virtual to hardware mailboxes.
 */
static void test_api_mailbox_multiplexation(void)
{
	int local;
	int remote;
	size_t volume;
	uint64_t latency;
	int mbx_in[TEST_MULTIPLEXATION_MBX_PAIRS];
	int mbx_out[TEST_MULTIPLEXATION_MBX_PAIRS];
	char message[KMAILBOX_MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	/* Creates multiple virtual mailboxes. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION_MBX_PAIRS; ++i)
	{
		test_assert((mbx_in[i] = kmailbox_create(local, i)) >= 0);
		test_assert((mbx_out[i] = kmailbox_open(remote, i)) >= 0);
	}

	/* Multiple write/read operations to test multiplex. */
	if (local == MASTER_NODENUM)
	{
		for (unsigned i = 0; i < TEST_MULTIPLEXATION_MBX_PAIRS; ++i)
		{
			kmemset(message, i, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_write(mbx_out[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			kmemset(message, 0, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_read(mbx_in[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (unsigned j = 0; j < KMAILBOX_MESSAGE_SIZE; ++j)
				test_assert(message[j] - (i + 1) == 0);
		}
	}
	else if (local == SLAVE_NODENUM)
	{
		for (unsigned i = 0; i < TEST_MULTIPLEXATION_MBX_PAIRS; ++i)
		{
			kmemset(message, 0, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_read(mbx_in[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (unsigned j = 0; j < KMAILBOX_MESSAGE_SIZE; ++j)
				test_assert(message[j] - i == 0);

			kmemset(message, i + 1, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_write(mbx_out[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
		}
	}

	/* Checks the data volume transferred by each vmailbox. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION_MBX_PAIRS; ++i)
	{
		test_assert(kmailbox_ioctl(mbx_in[i], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == KMAILBOX_MESSAGE_SIZE);
		test_assert(kmailbox_ioctl(mbx_in[i], MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency > 0);

		test_assert(kmailbox_ioctl(mbx_out[i], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == KMAILBOX_MESSAGE_SIZE);
		test_assert(kmailbox_ioctl(mbx_out[i], MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency > 0);
	}

	/* Closes the used vmailboxes. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION_MBX_PAIRS; ++i)
	{
		test_assert(kmailbox_unlink(mbx_in[i]) == 0);
		test_assert(kmailbox_close(mbx_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Multiplexation - Out of Order                                    *
 *============================================================================*/

#define TEST_MULTIPLEXATION2_MBX_PAIRS 3

/**
 * @brief API Test: Multiplexation test to assert the correct working when
 * messages flow occur in diferent order than the expected.
 */
static void test_api_mailbox_multiplexation_2(void)
{
	int local;
	int remote;
	size_t volume;
	uint64_t latency;
	int mbx_in[TEST_MULTIPLEXATION2_MBX_PAIRS];
	int mbx_out[TEST_MULTIPLEXATION2_MBX_PAIRS];
	char message[KMAILBOX_MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	for (unsigned i = 0; i < TEST_MULTIPLEXATION2_MBX_PAIRS; ++i)
	{
		test_assert((mbx_in[i] = kmailbox_create(local, i)) >= 0);
		test_assert((mbx_out[i] = kmailbox_open(remote, i)) >= 0);
	}

	if (local == MASTER_NODENUM)
	{
		/* Writes data in descendant order. */
		for (int i = (TEST_MULTIPLEXATION2_MBX_PAIRS - 1); i >= 0; --i)
		{
			kmemset(message, i, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_write(mbx_out[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
		}

		for (unsigned i = 0; i < TEST_MULTIPLEXATION2_MBX_PAIRS; ++i)
		{
			test_assert(kmailbox_ioctl(mbx_out[i], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == KMAILBOX_MESSAGE_SIZE);
			test_assert(kmailbox_ioctl(mbx_out[i], MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
			test_assert(latency > 0);
		}
	}
	else if (local == SLAVE_NODENUM)
	{
		for (unsigned i = 0; i < TEST_MULTIPLEXATION2_MBX_PAIRS; ++i)
		{
			kmemset(message, -1, KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_read(mbx_in[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (unsigned j = 0; j < KMAILBOX_MESSAGE_SIZE; ++j)
				test_assert((message[j] - i) == 0);
		}

		for (unsigned i = 0; i < TEST_MULTIPLEXATION2_MBX_PAIRS; ++i)
		{
			test_assert(kmailbox_ioctl(mbx_in[i], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == KMAILBOX_MESSAGE_SIZE);
		}
	}

	/* Synchronization message. */
	if (local == MASTER_NODENUM)
	{
		test_assert(kmailbox_read(mbx_in[0], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
	}
	else
		test_assert(kmailbox_write(mbx_out[0], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

	/* Closes the used vmailboxes. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION2_MBX_PAIRS; ++i)
	{
		test_assert(kmailbox_unlink(mbx_in[i]) == 0);
		test_assert(kmailbox_close(mbx_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Multiplexation - Multiple Messages                               *
 *============================================================================*/

#define TEST_MULTIPLEXATION3_MBX_PAIRS 4

/**
 * @brief API Test: Multiplexation test to assert the message buffers tab
 * solution to support multiple messages on the same port simultaneously.
 *
 * @details In this test, the MASTER sends 4 messages to 2 ports of the SLAVE.
 * The reads in the master are also made out of order to assure that there are
 * 2 messages queued for each vmailbox on SLAVE.
 */
static void test_api_mailbox_multiplexation_3(void)
{
	int local;
	int remote;
	size_t volume;
	uint64_t latency;
	int mbx_in[TEST_MULTIPLEXATION3_MBX_PAIRS];
	int mbx_out[TEST_MULTIPLEXATION3_MBX_PAIRS];
	char message[KMAILBOX_MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	for (unsigned i = 0; i < TEST_MULTIPLEXATION3_MBX_PAIRS; ++i)
	{
		test_assert((mbx_in[i] = kmailbox_create(local, i)) >= 0);
		test_assert((mbx_out[i] = kmailbox_open(remote, (int) (i/2))) >= 0);
	}

	/* Multiple write/read operations to test multiplex. */
	if (local == MASTER_NODENUM)
	{
		for (int i = 0; i < TEST_MULTIPLEXATION3_MBX_PAIRS; ++i)
		{
			kmemset(message, (int) (i/2), KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_write(mbx_out[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
		}

		/* Checks the data volume transfered by each mailbox. */
		for (unsigned i = 0; i < TEST_MULTIPLEXATION3_MBX_PAIRS; ++i)
		{
			test_assert(kmailbox_ioctl(mbx_out[i], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == KMAILBOX_MESSAGE_SIZE);
			test_assert(kmailbox_ioctl(mbx_out[i], MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
			test_assert(latency > 0);
		}
	}
	else if (local == SLAVE_NODENUM)
	{
		/* For each vmailbox. */
		for (int i = 1; i >= 0; --i)
		{
			/* For each message (2 for each vmailbox). */
			for (int j = 0; j < 2; ++j)
			{
				kmemset(message, -1, KMAILBOX_MESSAGE_SIZE);

				test_assert(kmailbox_read(mbx_in[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

				for (unsigned k = 0; k < KMAILBOX_MESSAGE_SIZE; ++k)
					test_assert((message[k] - i) == 0);
			}
		}

		/* Checks the data volume transfered by each mailbox. */
		for (unsigned i = 0; i < 2; ++i)
		{
			test_assert(kmailbox_ioctl(mbx_in[i], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == (KMAILBOX_MESSAGE_SIZE * 2));
		}
	}

	/* Synchronization message. */
	if (local == MASTER_NODENUM)
	{
		test_assert(kmailbox_read(mbx_in[0], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
	}
	else
		test_assert(kmailbox_write(mbx_out[0], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

	/* Closes the used vmailboxes. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION3_MBX_PAIRS; ++i)
	{
		test_assert(kmailbox_unlink(mbx_in[i]) == 0);
		test_assert(kmailbox_close(mbx_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Pending Messages - Unlink                                        *
 *============================================================================*/

#define TEST_PENDING_UNLINK_MBX_PAIRS 2

/**
 * @brief API Test: Test to assert if the kernel correctly avoids an unlink
 * call when there still messages addressed to the target vportal on message
 * buffers tab.
 */
static void test_api_mailbox_pending_msg_unlink(void)
{
	int local;
	int remote;
	size_t volume;
	uint64_t latency;
	int mbx_in[TEST_PENDING_UNLINK_MBX_PAIRS];
	int mbx_out[TEST_PENDING_UNLINK_MBX_PAIRS];
	char message[KMAILBOX_MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	for (unsigned i = 0; i < TEST_PENDING_UNLINK_MBX_PAIRS; ++i)
	{
		test_assert((mbx_in[i] = kmailbox_create(local, i)) >= 0);
		test_assert((mbx_out[i] = kmailbox_open(remote, i)) >= 0);
	}

	/* Multiple write/read operations to test multiplex. */
	if (local == MASTER_NODENUM)
	{
		for (int i = 0; i < TEST_PENDING_UNLINK_MBX_PAIRS; ++i)
			test_assert(kmailbox_write(mbx_out[i], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		/* Checks the data volume transfered by each mailbox. */
		for (unsigned i = 0; i < TEST_PENDING_UNLINK_MBX_PAIRS; ++i)
		{
			test_assert(kmailbox_ioctl(mbx_out[i], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == KMAILBOX_MESSAGE_SIZE);
			test_assert(kmailbox_ioctl(mbx_out[i], MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
			test_assert(latency > 0);

			test_assert(kmailbox_close(mbx_out[i]) == 0);
		}
	}
	else if (local == SLAVE_NODENUM)
	{
		test_assert(kmailbox_read(mbx_in[1], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_ioctl(mbx_in[1], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_unlink(mbx_in[1]) == 0);


		test_assert(kmailbox_unlink(mbx_in[0]) == -EBUSY);

		test_assert(kmailbox_read(mbx_in[0], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_ioctl(mbx_in[0], MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_unlink(mbx_in[0]) == 0);
	}

	/* Synchronization message + closes the used vmailboxes. */
	if (local == MASTER_NODENUM)
	{
		test_assert(kmailbox_read(mbx_in[0], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
		for (unsigned i = 0; i < TEST_PENDING_UNLINK_MBX_PAIRS; ++i)
			test_assert(kmailbox_unlink(mbx_in[i]) == 0);
	}
	else
	{
		test_assert(kmailbox_write(mbx_out[0], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
		for (unsigned i = 0; i < TEST_PENDING_UNLINK_MBX_PAIRS; ++i)
			test_assert(kmailbox_close(mbx_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Message Forwarding                                               *
 *============================================================================*/

/**
 * @brief API Test: Message forwarding
 */
static void test_api_mailbox_msg_forwarding(void)
{
	int local;
	int mbx_in;
	int mbx_out;
	size_t volume;
	uint64_t latency;
	char message[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();

	test_assert((mbx_in = kmailbox_create(local, 0)) >= 0);
	test_assert((mbx_out = kmailbox_open(local, 0)) >= 0);

	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);

	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);

	for (unsigned i = 0; i < NITERATIONS; i++)
	{
		kmemset(message, local, KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_write(mbx_out, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		kmemset(message, 0, KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_read(mbx_in, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		for (unsigned j = 0; j < KMAILBOX_MESSAGE_SIZE; ++j)
			test_assert(message[j] == local);
	}

	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * KMAILBOX_MESSAGE_SIZE));
	test_assert(kmailbox_ioctl(mbx_in, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * KMAILBOX_MESSAGE_SIZE));
	test_assert(kmailbox_ioctl(mbx_out, MAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * Fault Tests                                                                *
 *============================================================================*/

/*============================================================================*
 * Fault Test: Invalid Create                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Create
 */
static void test_fault_mailbox_invalid_create(void)
{
	int nodenum = knode_get_num();

	test_assert(kmailbox_create(-1, 0) == -EINVAL);
	test_assert(kmailbox_create(PROCESSOR_NOC_NODES_NUM, 0) == -EINVAL);
	test_assert(kmailbox_create(nodenum, -1) == -EINVAL);
	test_assert(kmailbox_create(nodenum, MAILBOX_PORT_NR) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Double Create                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Double Create
 */
static void test_fault_mailbox_double_create(void)
{
	int mbx;
	int nodenum = knode_get_num();

	test_assert((mbx = kmailbox_create(nodenum, 0)) >= 0);

	test_assert(kmailbox_create(nodenum, 0) == -EBUSY);

	test_assert(kmailbox_unlink(mbx) == 0);
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

	test_assert(kmailbox_create(nodenum, 0) == -EINVAL);
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
	test_assert(kmailbox_unlink(KMAILBOX_MAX) == -EINVAL);
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
	int mbxid1;
	int mbxid2;

	local = knode_get_num();

	test_assert((mbxid1 = kmailbox_create(local, 0)) >= 0);
	test_assert((mbxid2 = kmailbox_create(local, 1)) >= 0);

	test_assert(kmailbox_unlink(mbxid1) == 0);
	test_assert(kmailbox_unlink(mbxid1) == -EBADF);
	test_assert(kmailbox_unlink(mbxid2) == 0);
}

/*============================================================================*
 * Fault Test: Bad Unlink                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Unlink
 */
static void test_fault_mailbox_bad_unlink(void)
{
	int remote;
	int mbxid;

	remote = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);

	test_assert(kmailbox_unlink(mbxid) == -EBADF);

	test_assert(kmailbox_close(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Open                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Open
 */
static void test_fault_mailbox_invalid_open(void)
{
	int nodenum;

	nodenum = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert(kmailbox_open(-1, 0) == -EINVAL);
	test_assert(kmailbox_open(PROCESSOR_NOC_NODES_NUM, 0) == -EINVAL);
	test_assert(kmailbox_open(nodenum, -1) == -EINVAL);
	test_assert(kmailbox_open(nodenum, MAILBOX_PORT_NR) == -EINVAL);
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
	test_assert(kmailbox_close(KMAILBOX_MAX) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Double Close                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Double Close
 */
static void test_fault_mailbox_double_close(void)
{
	int mbxid1;
	int mbxid2;
	int nodenum;

	nodenum = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid1 = kmailbox_open(nodenum, 0)) >= 0);
	test_assert((mbxid2 = kmailbox_open(nodenum, 1)) >= 0);

	test_assert(kmailbox_close(mbxid1) == 0);
	test_assert(kmailbox_close(mbxid1) == -EBADF);
	test_assert(kmailbox_close(mbxid2) == 0);
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

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

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
	char buffer[KMAILBOX_MESSAGE_SIZE];

	test_assert(kmailbox_read(-1, buffer, KMAILBOX_MESSAGE_SIZE) == -EINVAL);
	test_assert(kmailbox_read(KMAILBOX_MAX, buffer, KMAILBOX_MESSAGE_SIZE) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Read                                                       *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Read
 */
static void test_fault_mailbox_bad_read(void)
{
	int mbxid;
	int nodenum;
	char buffer[KMAILBOX_MESSAGE_SIZE];

	nodenum = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_open(nodenum, 0)) >= 0);

	test_assert(kmailbox_read(mbxid, buffer, KMAILBOX_MESSAGE_SIZE) == -EBADF);

	test_assert(kmailbox_close(mbxid) == 0);
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
	char buffer[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

		test_assert(kmailbox_read(mbxid, buffer, -1) == -EINVAL);
		test_assert(kmailbox_read(mbxid, buffer, 0) == -EINVAL);
		test_assert(kmailbox_read(mbxid, buffer, KMAILBOX_MESSAGE_SIZE + 1) == -EINVAL);

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

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

		test_assert(kmailbox_read(mbxid, NULL, KMAILBOX_MESSAGE_SIZE) == -EINVAL);

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
	char buffer[KMAILBOX_MESSAGE_SIZE];

	test_assert(kmailbox_write(-1, buffer, KMAILBOX_MESSAGE_SIZE) == -EINVAL);
	test_assert(kmailbox_write(KMAILBOX_MAX, buffer, KMAILBOX_MESSAGE_SIZE) == -EINVAL);
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
	char buffer[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

		test_assert(kmailbox_write(mbxid, buffer, KMAILBOX_MESSAGE_SIZE) == -EBADF);

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Write Size                                             *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Write Size
 */
static void test_fault_mailbox_invalid_write_size(void)
{
	int mbxid;
	int remote;
	char buffer[KMAILBOX_MESSAGE_SIZE];

	remote = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);

		test_assert(kmailbox_write(mbxid, buffer, -1) == -EINVAL);
		test_assert(kmailbox_write(mbxid, buffer, 0) == -EINVAL);
		test_assert(kmailbox_write(mbxid, buffer, KMAILBOX_MESSAGE_SIZE + 1) == -EINVAL);

	test_assert(kmailbox_close(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Null Write                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Null Write
 */
static void test_fault_mailbox_null_write(void)
{
	int mbxid;
	int remote;

	remote = (knode_get_num() == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);

		test_assert(kmailbox_write(mbxid, NULL, KMAILBOX_MESSAGE_SIZE) == -EINVAL);

	test_assert(kmailbox_close(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Wait                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Wait
 */
static void test_fault_mailbox_invalid_wait(void)
{
	test_assert(kmailbox_wait(-1) == -EINVAL);
	test_assert(kmailbox_wait(KMAILBOX_MAX) == -EINVAL);
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
	test_assert(kmailbox_ioctl(KMAILBOX_MAX, MAILBOX_IOCTL_GET_VOLUME, &volume) == -EINVAL);
	test_assert(kmailbox_ioctl(KMAILBOX_MAX, MAILBOX_IOCTL_GET_LATENCY, &latency) == -EINVAL);

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

		test_assert(kmailbox_ioctl(mbxid, -1, &volume) == -ENOTSUP);

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Bad mbxid                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Bad mbxid
 */
static void test_fault_mailbox_bad_mbxid(void)
{
	int mbx_in;
	int mbx_out;
	int remote;
	int local;
	size_t volume;
	char buffer[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbx_in = kmailbox_create(local, 0)) >= 0);
	test_assert((mbx_out = kmailbox_open(remote, 0)) >= 0);

	test_assert(kmailbox_close(mbx_out + 1) == -EBADF);
	test_assert(kmailbox_unlink(mbx_in + 1) == -EBADF);
	test_assert(kmailbox_read(mbx_in + 1, buffer, KMAILBOX_MESSAGE_SIZE) == -EBADF);
	test_assert(kmailbox_write(mbx_in + 1, buffer, KMAILBOX_MESSAGE_SIZE) == -EBADF);
	test_assert(kmailbox_wait(mbx_in + 1) == -EBADF);
	test_assert(kmailbox_ioctl(mbx_in + 1, MAILBOX_IOCTL_GET_VOLUME, &volume) == -EBADF);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * Stress Tests                                                               *
 *============================================================================*/

/**
 * @name Number of setups and communications.
 */
/**@{*/
#define NSETUPS 1
#define NCOMMUNICATIONS 1
#define TEST_MAILBOX_NPORTS TEST_MULTIPLEXATION_MBX_PAIRS
/**@}*/

/*============================================================================*
 * Stress Test: Mailbox Create Unlink                                         *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Create Unlink
 */
PRIVATE void test_stress_mailbox_create_unlink(void)
{
	int mbxid;
	int local;

	local = knode_get_num();

	for (int i = 0; i < NSETUPS; ++i)
	{
		test_assert((mbxid = kmailbox_create(local, 0)) >= 0);
		test_assert(kmailbox_unlink(mbxid) == 0);
	}
}

/*============================================================================*
 * Stress Test: Mailbox Open Close                                            *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Open Close
 */
PRIVATE void test_stress_mailbox_open_close(void)
{
	int mbxid;
	int remote;

	remote = knode_get_num() == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	for (int i = 0; i < NSETUPS; ++i)
	{
		test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);
		test_assert(kmailbox_close(mbxid) == 0);
	}
}

/*============================================================================*
 * Stress Test: Mailbox Broadcast                                             *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Broadcast
 */
PRIVATE void test_stress_mailbox_broadcast(void)
{
	int local;
	int remote;
	int mbxid;
	char message[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == MASTER_NODENUM)
	{
		message[0] = local;

		for (int i = 0; i < NSETUPS; ++i)
		{
			test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
				test_assert(kmailbox_write(mbxid, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_close(mbxid) == 0);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				message[0] = local;
				test_assert(kmailbox_read(mbxid, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				test_assert(message[0] == remote);
			}

			test_assert(kmailbox_unlink(mbxid) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Mailbox Gather                                                *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Gather
 */
PRIVATE void test_stress_mailbox_gather(void)
{
	int local;
	int remote;
	int mbxid;
	char message[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		message[0] = local;

		for (int i = 0; i < NSETUPS; ++i)
		{
			test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
				test_assert(kmailbox_write(mbxid, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			test_assert(kmailbox_close(mbxid) == 0);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				message[0] = local;
				test_assert(kmailbox_read(mbxid, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				test_assert(message[0] == remote);
			}

			test_assert(kmailbox_unlink(mbxid) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Mailbox Ping-Pong                                             *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Ping-Pong
 */
PRIVATE void test_stress_mailbox_pingpong(void)
{
	int local;
	int remote;
	int inbox;
	int outbox;
	char msg_in[KMAILBOX_MESSAGE_SIZE];
	char msg_out[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	msg_out[0] = local;

	for (int i = 0; i < NSETUPS; ++i)
	{
		test_assert((inbox = kmailbox_create(local, 0)) >= 0);
		test_assert((outbox = kmailbox_open(remote, 0)) >= 0);

		if (local == SLAVE_NODENUM)
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				msg_in[0] = local;
				test_assert(kmailbox_read(inbox, msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				test_assert(msg_in[0] == remote);

				test_assert(kmailbox_write(outbox, msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
			}
		}
		else
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				test_assert(kmailbox_write(outbox, msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

				msg_in[0] = local;
				test_assert(kmailbox_read(inbox, msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				test_assert(msg_in[0] == remote);
			}
		}

		test_assert(kmailbox_close(outbox) == 0);
		test_assert(kmailbox_unlink(inbox) == 0);
	}
}

/*============================================================================*
 * Stress Test: Mailbox Multiplexing Broadcast                                *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Multiplexing Broadcast
 */
PRIVATE void test_stress_mailbox_multiplexing_broadcast(void)
{
	int local;
	int remote;
	int mbxids[TEST_MAILBOX_NPORTS];
	char message[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == MASTER_NODENUM)
	{
		message[0] = local;

		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
				test_assert((mbxids[j] = kmailbox_open(remote, j)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
				for (int k = 0; k < TEST_MAILBOX_NPORTS; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
				test_assert((mbxids[j] = kmailbox_create(local, j)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_MAILBOX_NPORTS; ++k)
				{
					message[0] = local;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == remote);
				}
			}

			for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
				test_assert(kmailbox_unlink(mbxids[j]) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Mailbox Multiplexing Gather                                   *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Multiplexing Gather
 */
PRIVATE void test_stress_mailbox_multiplexing_gather(void)
{
	int local;
	int remote;
	int mbxids[TEST_MAILBOX_NPORTS];
	char message[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		message[0] = local;

		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
				test_assert((mbxids[j] = kmailbox_open(remote, j)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
				for (int k = 0; k < TEST_MAILBOX_NPORTS; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
				test_assert((mbxids[j] = kmailbox_create(local, j)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_MAILBOX_NPORTS; ++k)
				{
					message[0] = local;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == remote);
				}
			}

			for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
				test_assert(kmailbox_unlink(mbxids[j]) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Mailbox Multiplexing Ping-Pong                                *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Multiplexing Ping-Pong
 */
PRIVATE void test_stress_mailbox_multiplexing_pingpong(void)
{
	int local;
	int remote;
	int inboxes[TEST_MAILBOX_NPORTS];
	int outboxes[TEST_MAILBOX_NPORTS];
	char msg_in[KMAILBOX_MESSAGE_SIZE];
	char msg_out[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	msg_out[0] = local;

	for (int i = 0; i < NSETUPS; ++i)
	{
		for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
		{
			test_assert((inboxes[j] = kmailbox_create(local, j)) >= 0);
			test_assert((outboxes[j] = kmailbox_open(remote, j)) >= 0);
		}

		if (local == SLAVE_NODENUM)
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_MAILBOX_NPORTS; ++k)
				{
					msg_in[0] = local;
					test_assert(kmailbox_read(inboxes[k], msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(msg_in[0] == remote);

					test_assert(kmailbox_write(outboxes[k], msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				}
			}
		}
		else
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_MAILBOX_NPORTS; ++k)
				{
					test_assert(kmailbox_write(outboxes[k], msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

					msg_in[0] = local;
					test_assert(kmailbox_read(inboxes[k], msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(msg_in[0] == remote);
				}
			}
		}

		for (int j = 0; j < TEST_MAILBOX_NPORTS; ++j)
		{
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Thread synchronization                                        *
 *============================================================================*/

/**
 * @brief A simple fence.
 */
static struct fence
{
	int ncores;      /**< Number of cores in the fence.            */
	int nreached;    /**< Number of cores that reached the fence.  */
	int release;     /**< Wait condition.                          */
	spinlock_t lock; /**< Lock.                                    */
} _fence = {
	0, 0, 0, SPINLOCK_UNLOCKED
};

/**
 * @brief Initializes a fence.
 *
 * @param b      Target fence.
 * @param ncores Number of cores in the fence.
 */
PRIVATE void fence_init(struct fence *b, int ncores)
{
	b->ncores   = ncores;
	b->nreached = 0;
}

/**
 * @brief Waits in a fence.
 */
PRIVATE void fence(struct fence *b)
{
	int exit;
	int local_release;

	/* Notifies thread reach. */
	spinlock_lock(&b->lock);

		local_release = !b->release;

		b->nreached++;

		if (b->nreached == b->ncores)
		{
			b->nreached = 0;
			b->release  = local_release;
		}

	spinlock_unlock(&b->lock);

	exit = 0;
	while (!exit)
	{
		exit = 100;
		while (exit--);

		spinlock_lock(&b->lock);
			exit = (local_release == b->release);
		spinlock_unlock(&b->lock);
	}
}

#ifdef __mppa256__
	#define TEST_THREAD_NPORTS (K1BDP_CORES_NUM - 1)
#else
	#define TEST_THREAD_NPORTS (THREAD_MAX)
#endif

/*============================================================================*
 * Stress Test: Mailbox Thread Multiplexing Broadcast                         *
 *============================================================================*/

PRIVATE void * do_thread_multiplexing_broadcast(void * arg)
{
	int tid;
	int nports;
	int local;
	int remote;
	int mbxids[(TEST_THREAD_NPORTS / THREAD_MAX) + 1];
	char message[KMAILBOX_MESSAGE_SIZE];

	tid = ((int)(intptr_t) arg);

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == MASTER_NODENUM)
	{
		message[0] = local;

		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				if (j == (tid + nports * THREAD_MAX))
					test_assert((mbxids[nports++] = kmailbox_open(remote, j)) >= 0);

				fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				
				fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);

			fence(&_fence);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				if (j == (tid + nports * THREAD_MAX))
					test_assert((mbxids[nports++] = kmailbox_create(local, j)) >= 0);

				fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					message[0] = local;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == remote);
				}

				fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_unlink(mbxids[j]) == 0);

			fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Broadcast
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_broadcast(void)
{
	kthread_t tid[THREAD_MAX - 1];

	fence_init(&_fence, THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_broadcast, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_broadcast(0);

	/* Join threads. */
	for (int i = 1; i < THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);
}

/*============================================================================*
 * Stress Test: Mailbox Thread Multiplexing Gather                            *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Gather
 */
PRIVATE void * do_thread_multiplexing_gather(void * arg)
{
	int tid;
	int nports;
	int local;
	int remote;
	int mbxids[(TEST_THREAD_NPORTS / THREAD_MAX) + 1];
	char message[KMAILBOX_MESSAGE_SIZE];

	tid = ((int)(intptr_t) arg);

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		message[0] = local;

		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				if (j == (tid + nports * THREAD_MAX))
					test_assert((mbxids[nports++] = kmailbox_open(remote, j)) >= 0);

				fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

				fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);

			fence(&_fence);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				if (j == (tid + nports * THREAD_MAX))
					test_assert((mbxids[nports++] = kmailbox_create(local, j)) >= 0);

				fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					message[0] = local;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == remote);
				}

				fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_unlink(mbxids[j]) == 0);

			fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Gather
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_gather(void)
{
	kthread_t tid[THREAD_MAX - 1];

	fence_init(&_fence, THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_gather, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_gather(0);

	/* Join threads. */
	for (int i = 1; i < THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);
}

/*============================================================================*
 * Stress Test: Mailbox Thread Multiplexing Ping-Pong                         *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Ping-Pong
 */
PRIVATE void * do_thread_multiplexing_pingpong(void * arg)
{
	int tid;
	int local;
	int remote;
	int nports;
	int inboxes[(TEST_THREAD_NPORTS / THREAD_MAX) + 1];
	int outboxes[(TEST_THREAD_NPORTS / THREAD_MAX) + 1];
	char msg_in[KMAILBOX_MESSAGE_SIZE];
	char msg_out[KMAILBOX_MESSAGE_SIZE];

	tid = ((int)(intptr_t) arg);

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	msg_out[0] = local;

	for (int i = 0; i < NSETUPS; ++i)
	{
		nports = 0;
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			if (j == (tid + nports * THREAD_MAX))
			{
				test_assert((inboxes[nports] = kmailbox_create(local, j)) >= 0);
				test_assert((outboxes[nports] = kmailbox_open(remote, j)) >= 0);
				nports++;
			}

			fence(&_fence);
		}

		if (local == MASTER_NODENUM)
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					msg_in[0] = local;
					test_assert(kmailbox_read(inboxes[k], msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(msg_in[0] == remote);

					test_assert(kmailbox_write(outboxes[k], msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				}

				fence(&_fence);
			}
		}
		else
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					test_assert(kmailbox_write(outboxes[k], msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

					msg_in[0] = local;
					test_assert(kmailbox_read(inboxes[k], msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(msg_in[0] == remote);
				}

				fence(&_fence);
			}
		}

		for (int j = 0; j < nports; ++j)
		{
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}

		fence(&_fence);
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Ping-Pong
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_pingpong(void)
{
	kthread_t tid[THREAD_MAX - 1];

	fence_init(&_fence, THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_pingpong, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_pingpong(0);

	/* Join threads. */
	for (int i = 1; i < THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
static struct test mailbox_tests_api[] = {
	{ test_api_mailbox_create_unlink,      "[test][mailbox][api] mailbox create unlink      [passed]" },
	{ test_api_mailbox_open_close,         "[test][mailbox][api] mailbox open close         [passed]" },
	{ test_api_mailbox_get_volume,         "[test][mailbox][api] mailbox get volume         [passed]" },
	{ test_api_mailbox_get_latency,        "[test][mailbox][api] mailbox get latency        [passed]" },
	{ test_api_mailbox_read_write,         "[test][mailbox][api] mailbox read write         [passed]" },
	{ test_api_mailbox_virtualization,     "[test][mailbox][api] mailbox virtualization     [passed]" },
	{ test_api_mailbox_multiplexation,     "[test][mailbox][api] mailbox multiplexation     [passed]" },
	{ test_api_mailbox_multiplexation_2,   "[test][mailbox][api] mailbox multiplexation 2   [passed]" },
	{ test_api_mailbox_multiplexation_3,   "[test][mailbox][api] mailbox multiplexation 3   [passed]" },
	{ test_api_mailbox_pending_msg_unlink, "[test][mailbox][api] mailbox pending msg unlink [passed]" },
	{ test_api_mailbox_msg_forwarding,     "[test][mailbox][api] mailbox message forwarding [passed]" },
	{ NULL,                                 NULL                                                      },
};

/**
 * @brief Fault tests.
 */
static struct test mailbox_tests_fault[] = {
	{ test_fault_mailbox_invalid_create,     "[test][mailbox][fault] mailbox invalid create     [passed]" },
	{ test_fault_mailbox_bad_create,         "[test][mailbox][fault] mailbox bad create         [passed]" },
	{ test_fault_mailbox_double_create,      "[test][mailbox][fault] mailbox double create      [passed]" },
	{ test_fault_mailbox_invalid_unlink,     "[test][mailbox][fault] mailbox invalid unlink     [passed]" },
	{ test_fault_mailbox_double_unlink,      "[test][mailbox][fault] mailbox double unlink      [passed]" },
	{ test_fault_mailbox_bad_unlink,         "[test][mailbox][fault] mailbox bad unlink         [passed]" },
	{ test_fault_mailbox_invalid_open,       "[test][mailbox][fault] mailbox invalid open       [passed]" },
	{ test_fault_mailbox_invalid_close,      "[test][mailbox][fault] mailbox invalid close      [passed]" },
	{ test_fault_mailbox_double_close,       "[test][mailbox][fault] mailbox double close       [passed]" },
	{ test_fault_mailbox_bad_close,          "[test][mailbox][fault] mailbox bad close          [passed]" },
	{ test_fault_mailbox_invalid_read,       "[test][mailbox][fault] mailbox invalid read       [passed]" },
	{ test_fault_mailbox_bad_read,           "[test][mailbox][fault] mailbox bad read           [passed]" },
	{ test_fault_mailbox_invalid_read_size,  "[test][mailbox][fault] mailbox invalid read size  [passed]" },
	{ test_fault_mailbox_null_read,          "[test][mailbox][fault] mailbox null read          [passed]" },
	{ test_fault_mailbox_invalid_write,      "[test][mailbox][fault] mailbox invalid write      [passed]" },
	{ test_fault_mailbox_bad_write,          "[test][mailbox][fault] mailbox bad write          [passed]" },
	{ test_fault_mailbox_invalid_write_size, "[test][mailbox][fault] mailbox invalid write size [passed]" },
	{ test_fault_mailbox_null_write,         "[test][mailbox][fault] mailbox null write         [passed]" },
	{ test_fault_mailbox_invalid_wait,       "[test][mailbox][fault] mailbox invalid wait       [passed]" },
	{ test_fault_mailbox_invalid_ioctl,      "[test][mailbox][fault] mailbox invalid ioctl      [passed]" },
	{ test_fault_mailbox_bad_mbxid,          "[test][mailbox][fault] mailbox bad mbxid          [passed]" },
	{ NULL,                                   NULL                                                        },
};

/**
 * @brief Stress tests.
 */
PRIVATE struct test mailbox_tests_stress[] = {
	/* Intra-Cluster API Tests */
	{ test_stress_mailbox_create_unlink,                 "[test][mailbox][stress] mailbox create unlink                 [passed]" },
	{ test_stress_mailbox_open_close,                    "[test][mailbox][stress] mailbox open close                    [passed]" },
	{ test_stress_mailbox_thread_multiplexing_broadcast, "[test][mailbox][stress] mailbox thread multiplexing broadcast [passed]" },
	{ test_stress_mailbox_thread_multiplexing_gather,    "[test][mailbox][stress] mailbox thread multiplexing gather    [passed]" },
	{ test_stress_mailbox_thread_multiplexing_pingpong,  "[test][mailbox][stress] mailbox thread multiplexing ping-pong [passed]" },
	{ test_stress_mailbox_broadcast,                     "[test][mailbox][stress] mailbox broadcast                     [passed]" },
	{ test_stress_mailbox_gather,                        "[test][mailbox][stress] mailbox gather                        [passed]" },
	{ test_stress_mailbox_pingpong,                      "[test][mailbox][stress] mailbox ping-pong                     [passed]" },
	{ test_stress_mailbox_multiplexing_broadcast,        "[test][mailbox][stress] mailbox multiplexing broadcast        [passed]" },
	{ test_stress_mailbox_multiplexing_gather,           "[test][mailbox][stress] mailbox multiplexing gather           [passed]" },
	{ test_stress_mailbox_multiplexing_pingpong,         "[test][mailbox][stress] mailbox multiplexing ping-pong        [passed]" },
	{ NULL,                                               NULL                                                                    },
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
			nanvix_puts("--------------------------------------------------------------------------------");
		for (unsigned i = 0; mailbox_tests_api[i].test_fn != NULL; i++)
		{
			mailbox_tests_api[i].test_fn();

			if (nodenum == MASTER_NODENUM)
				nanvix_puts(mailbox_tests_api[i].name);
		}

		/* Fault Tests */
		if (nodenum == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------");
		for (unsigned i = 0; mailbox_tests_fault[i].test_fn != NULL; i++)
		{
			mailbox_tests_fault[i].test_fn();

			if (nodenum == MASTER_NODENUM)
				nanvix_puts(mailbox_tests_fault[i].name);
		}

		/* Stress Tests */
		if (nodenum == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------");
		for (unsigned i = 0; mailbox_tests_stress[i].test_fn != NULL; i++)
		{
			mailbox_tests_stress[i].test_fn();

			if (nodenum == MASTER_NODENUM)
				nanvix_puts(mailbox_tests_stress[i].name);
		}
	}
}

#endif /* __TARGET_HAS_MAILBOX */
