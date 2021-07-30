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
#include <nanvix/runtime/fence.h>
#include <posix/errno.h>

#include "test.h"

#if __TARGET_HAS_MAILBOX

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

/**
 * @name Multiplexation
 */
/**{*/
#if __NANVIX_IKC_USES_ONLY_MAILBOX
#define TEST_MULTIPLEXATION_MBX_PAIRS  KMAILBOX_PORT_NR
#else
#define TEST_MULTIPLEXATION_MBX_PAIRS  MAILBOX_PORT_NR
#endif
#define TEST_MULTIPLEXATION2_MBX_PAIRS 3
#define TEST_MULTIPLEXATION3_MBX_PAIRS 4
/**}*/

/**
 * @brief Virtualization
 */
#if __NANVIX_IKC_USES_ONLY_MAILBOX
#define TEST_VIRTUALIZATION_MBX_NR KMAILBOX_PORT_NR
#else
#define TEST_VIRTUALIZATION_MBX_NR MAILBOX_PORT_NR
#endif

/**
 * @brief Pending Messages - Unlink
 */
#define TEST_PENDING_UNLINK_MBX_PAIRS 2

/**
 * @name Threads 
 */
/**{*/
#define TEST_THREAD_MAX       (CORES_NUM - 1)
#define TEST_THREAD_SPECIFIED 3
#define TEST_THREAD_MBXID_NUM ((TEST_THREAD_NPORTS / TEST_THREAD_MAX) + 1)
#define TEST_CORE_AFFINITY    (1)
#define TEST_THREAD_AFFINITY  (1 << TEST_CORE_AFFINITY)
/**}*/

/*============================================================================*
 * Global variables                                                           *
 *============================================================================*/

/**
 * @brief Simple fence used for thread synchronization.
 */
PRIVATE struct nanvix_fence _fence;

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
 * API Test: Open Close                                                       *
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

		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == 0);

		test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
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

		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

		test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * API Test: Get counters                                                     *
 *============================================================================*/

/**
 * @brief API Test: Mailbox Get latency
 */
static void test_api_mailbox_get_counters(void)
{
	int local;
	int remote;
	int mbx_in;
	int mbx_out;
	uint64_t c0;
	uint64_t c1;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbx_in = kmailbox_create(local, 0)) >= 0);

		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NCREATES, &c0) == 0);
		test_assert(c0 == 4);
		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NOPENS, &c1) == 0);
		test_assert(c1 == 3);
		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NCLOSES, &c1) == 0);
		test_assert(c1 == 3);

	test_assert((mbx_out = kmailbox_open(remote, 0)) >= 0);

		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NCREATES, &c0) == 0);
		test_assert(c0 == 4);
		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NOPENS, &c0) == 0);
		test_assert(c0 == 4);
		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NUNLINKS, &c1) == 0);
		test_assert(c1 == 3);
		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NCLOSES, &c1) == 0);
		test_assert(c1 == 3);

		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NREADS, &c1) == 0);
		test_assert(c1 == 0);
		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NWRITES, &c1) == 0);
		test_assert(c1 == 0);

	test_assert(kmailbox_close(mbx_out) == 0);

		test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NCLOSES, &c1) == 0);
		test_assert(c1 == 4);

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
	uint64_t counter;
	char message[KMAILBOX_MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbx_in = kmailbox_create(local, 0)) >= 0);
	test_assert((mbx_out = kmailbox_open(remote, 0)) >= 0);

	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

	test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NREADS, &counter) == 0);
	test_assert(counter  == 0);
	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NWRITES, &counter) == 0);
	test_assert(counter == 0);

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

	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * KMAILBOX_MESSAGE_SIZE));
	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

	test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * KMAILBOX_MESSAGE_SIZE));
	test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NREADS, &counter) == 0);
	test_assert(counter  == NITERATIONS);
	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_NWRITES, &counter) == 0);
	test_assert(counter == NITERATIONS);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * API Test: Virtualization                                                   *
 *============================================================================*/

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
		test_assert(kmailbox_ioctl(mbx_in[i], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == KMAILBOX_MESSAGE_SIZE);
		test_assert(kmailbox_ioctl(mbx_in[i], KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

		test_assert(kmailbox_ioctl(mbx_out[i], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == KMAILBOX_MESSAGE_SIZE);
		test_assert(kmailbox_ioctl(mbx_out[i], KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
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
			test_assert(kmailbox_ioctl(mbx_out[i], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == KMAILBOX_MESSAGE_SIZE);
			test_assert(kmailbox_ioctl(mbx_out[i], KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
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
			test_assert(kmailbox_ioctl(mbx_in[i], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
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
			test_assert(kmailbox_ioctl(mbx_out[i], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == KMAILBOX_MESSAGE_SIZE);
			test_assert(kmailbox_ioctl(mbx_out[i], KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);
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
			test_assert(kmailbox_ioctl(mbx_in[i], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
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
			test_assert(kmailbox_ioctl(mbx_out[i], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == KMAILBOX_MESSAGE_SIZE);
			test_assert(kmailbox_ioctl(mbx_out[i], KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

			test_assert(kmailbox_close(mbx_out[i]) == 0);
		}
	}
	else if (local == SLAVE_NODENUM)
	{
		test_assert(kmailbox_read(mbx_in[1], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_ioctl(mbx_in[1], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_unlink(mbx_in[1]) == 0);


		test_assert(kmailbox_unlink(mbx_in[0]) == -EBUSY);

		test_assert(kmailbox_read(mbx_in[0], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_ioctl(mbx_in[0], KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
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

	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

	test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

	for (unsigned i = 0; i < NITERATIONS; i++)
	{
		kmemset(message, local, KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_write(mbx_out, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		kmemset(message, 0, KMAILBOX_MESSAGE_SIZE);

		test_assert(kmailbox_read(mbx_in, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

		for (unsigned j = 0; j < KMAILBOX_MESSAGE_SIZE; ++j)
			test_assert(message[j] == local);
	}

	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * KMAILBOX_MESSAGE_SIZE));
	test_assert(kmailbox_ioctl(mbx_in, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

	test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * KMAILBOX_MESSAGE_SIZE));
	test_assert(kmailbox_ioctl(mbx_out, KMAILBOX_IOCTL_GET_LATENCY, &latency) == 0);

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

#if __NANVIX_IKC_USES_ONLY_MAILBOX
	test_assert(kmailbox_create(nodenum, KMAILBOX_PORT_NR) == -EINVAL);
#else
	test_assert(kmailbox_create(nodenum, MAILBOX_PORT_NR) == -EINVAL);
#endif
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
#if __NANVIX_IKC_USES_ONLY_MAILBOX
	test_assert(kmailbox_open(nodenum, KMAILBOX_PORT_NR) == -EINVAL);
#else
	test_assert(kmailbox_open(nodenum, MAILBOX_PORT_NR) == -EINVAL);
#endif
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

	test_assert(kmailbox_ioctl(-1, KMAILBOX_IOCTL_GET_VOLUME, &volume) == -EINVAL);
	test_assert(kmailbox_ioctl(-1, KMAILBOX_IOCTL_GET_LATENCY, &latency) == -EINVAL);
	test_assert(kmailbox_ioctl(KMAILBOX_MAX, KMAILBOX_IOCTL_GET_VOLUME, &volume) == -EINVAL);
	test_assert(kmailbox_ioctl(KMAILBOX_MAX, KMAILBOX_IOCTL_GET_LATENCY, &latency) == -EINVAL);

	local = knode_get_num();

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

		test_assert(kmailbox_ioctl(mbxid, -1, &volume) == -ENOTSUP);
		test_assert(kmailbox_ioctl(mbxid, KMAILBOX_IOCTL_GET_VOLUME, NULL) == -EFAULT);
		test_assert(kmailbox_ioctl(mbxid, KMAILBOX_IOCTL_GET_LATENCY, NULL) == -EFAULT);

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Set Remote                                             *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Set Remote
 */
static void test_fault_mailbox_invalid_set_remote(void)
{
	int mbxid;
	int local;
	int remote;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	/* Invalid mbxid. */
	test_assert(kmailbox_set_remote(-1, remote, MAILBOX_ANY_PORT) == (-EINVAL));
	test_assert(kmailbox_set_remote(KMAILBOX_MAX, remote, MAILBOX_ANY_PORT) == (-EINVAL));

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

	/* Invalid remote. */
	test_assert(kmailbox_set_remote(mbxid, -1, MAILBOX_ANY_PORT) == (-EINVAL));
	test_assert(kmailbox_set_remote(mbxid, (MAILBOX_ANY_SOURCE + 1), MAILBOX_ANY_PORT) == (-EINVAL));

	/* Invalid port number. */
	test_assert(kmailbox_set_remote(mbxid, remote, -1) == (-EINVAL));
	test_assert(kmailbox_set_remote(mbxid, remote, (MAILBOX_ANY_PORT + 1)) == (-EINVAL));

	test_assert(kmailbox_unlink(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Bad Set Remote                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Set Remote
 */
static void test_fault_mailbox_bad_set_remote(void)
{
	int mbxid;
	int local;
	int remote;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);

	/* Bad mailbox. */
	test_assert(kmailbox_set_remote(mbxid, local, MAILBOX_ANY_PORT) == (-EFAULT));

	test_assert(kmailbox_close(mbxid) == 0);
}

/*============================================================================*
 * Fault Test: Double Set Remote                                              *
 *============================================================================*/

/**
 * @brief Fault Test: Double Set Remote
 */
static void test_fault_mailbox_double_set_remote(void)
{
	int mbxid;
	int local;
	int remote;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

	/* Double set remote. */
	test_assert(kmailbox_set_remote(mbxid, remote, MAILBOX_ANY_PORT) == 0);
	test_assert(kmailbox_set_remote(mbxid, remote, MAILBOX_ANY_PORT) == (-EFAULT));

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

	test_assert(kmailbox_read(mbx_out, buffer, KMAILBOX_MESSAGE_SIZE) == -EBADF);
	test_assert(kmailbox_write(mbx_in, buffer, KMAILBOX_MESSAGE_SIZE) == -EBADF);

	test_assert(kmailbox_read(mbx_in + 1, buffer, KMAILBOX_MESSAGE_SIZE) == -EBADF);
	test_assert(kmailbox_write(mbx_out + 1, buffer, KMAILBOX_MESSAGE_SIZE) == -EBADF);

	test_assert(kmailbox_wait(mbx_out + 1) == -EBADF);
	test_assert(kmailbox_ioctl(mbx_out + 1, KMAILBOX_IOCTL_GET_VOLUME, &volume) == -EBADF);

	test_assert(kmailbox_unlink(mbx_in + 1) == -EBADF);
	test_assert(kmailbox_close(mbx_out + 1) == -EBADF);

	test_assert(kmailbox_close(mbx_out) == 0);
	test_assert(kmailbox_unlink(mbx_in) == 0);
}

/*============================================================================*
 * Stress Tests                                                               *
 *============================================================================*/

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
	int mbxids[TEST_MULTIPLEXATION_MBX_PAIRS];
	char message[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == MASTER_NODENUM)
	{
		message[0] = local;

		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
				test_assert((mbxids[j] = kmailbox_open(remote, j)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
				test_assert((mbxids[j] = kmailbox_create(local, j)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					message[0] = local;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == remote);
				}
			}

			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
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
	int mbxids[TEST_MULTIPLEXATION_MBX_PAIRS];
	char message[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		message[0] = local;

		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
				test_assert((mbxids[j] = kmailbox_open(remote, j)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
				test_assert((mbxids[j] = kmailbox_create(local, j)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					message[0] = local;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == remote);
				}
			}

			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
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
	int inboxes[TEST_MULTIPLEXATION_MBX_PAIRS];
	int outboxes[TEST_MULTIPLEXATION_MBX_PAIRS];
	char msg_in[KMAILBOX_MESSAGE_SIZE];
	char msg_out[KMAILBOX_MESSAGE_SIZE];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	msg_out[0] = local;

	for (int i = 0; i < NSETUPS; ++i)
	{
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			test_assert((inboxes[j] = kmailbox_create(local, j)) >= 0);
			test_assert((outboxes[j] = kmailbox_open(remote, j)) >= 0);
		}

		if (local == SLAVE_NODENUM)
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
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
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					test_assert(kmailbox_write(outboxes[k], msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

					msg_in[0] = local;
					test_assert(kmailbox_read(inboxes[k], msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(msg_in[0] == remote);
				}
			}
		}

		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Thread synchronization                                        *
 *============================================================================*/

/*============================================================================*
 * Stress Test: Thread Specified Source                                       *
 *============================================================================*/

PRIVATE void * do_thread_specified_source(void * arg)
{
	int tid;
	int local;
	int remote;
	int mbxid;
	int nreads;
	int nmessages;
	char message[KMAILBOX_MESSAGE_SIZE];

	tid = ((int)(intptr_t) arg);

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		/* Opens all mailboxes in order. */
		for (int i = 0; i < TEST_THREAD_SPECIFIED; ++i)
		{
			if (tid == i)
				test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);

			nanvix_fence(&_fence);
		}

		/* Send some messages to the master node. */
		for (int i = 0; i < TEST_THREAD_SPECIFIED; ++i)
		{
			message[0] = tid;
			message[1] = i;
			test_assert(kmailbox_write(mbxid, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
		}

		test_assert(kmailbox_close(mbxid) == 0);
	}
	else
	{
		nmessages = TEST_THREAD_SPECIFIED * TEST_THREAD_SPECIFIED;
		nreads = 0;

		test_assert((mbxid = kmailbox_create(local, 0)) >= 0);

		/* Reads only messages from pair ports. */
		for (int i = 0; i < TEST_THREAD_SPECIFIED; i += 2)
		{
			/* Reads all the messages of a single port first. */
			for (int k = 0; k < TEST_THREAD_SPECIFIED; ++k)
			{
				message[0] = (-1);

				/* Specifies the port from where to read. */
				test_assert(kmailbox_set_remote(mbxid, MAILBOX_ANY_SOURCE, i) == 0);

				test_assert(kmailbox_read(mbxid, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				test_assert((message[0] == i) && (message[1] == k));

				nreads++;
			}
		}

		/* Reads the restant messages. */
		while (nreads < nmessages)
		{
			message[0] = 0;

			test_assert(kmailbox_read(mbxid, message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
			test_assert((message[0] % 2) == 1);

			nreads++;
		}

		test_assert(kmailbox_unlink(mbxid) == 0);
	}

	return (NULL);
}

/**
 * @brief Stress Test: Specified Thread Source
 */
static void test_stress_mailbox_thread_specified_source(void)
{
	int local;
	kthread_t tid[TEST_THREAD_SPECIFIED - 1];

	local = knode_get_num();

	if (local == SLAVE_NODENUM)
	{
		nanvix_fence_init(&_fence, TEST_THREAD_SPECIFIED);

		/* Create threads. */
		for (int i = 1; i < TEST_THREAD_SPECIFIED; ++i)
			test_assert(kthread_create(&tid[i - 1], do_thread_specified_source, ((void *)(intptr_t) i)) == 0);

		do_thread_specified_source(0);

		/* Join threads. */
		for (int i = 1; i < TEST_THREAD_SPECIFIED; ++i)
			test_assert(kthread_join(tid[i - 1], NULL) == 0);
	}
	else if (local == MASTER_NODENUM)
		do_thread_specified_source(0);
}

/*============================================================================*
 * Stress Test: Thread Specified Affinity                                     *
 *============================================================================*/

PRIVATE void * do_thread_specified_affinity(void * arg)
{
	test_assert(kthread_set_affinity(TEST_THREAD_AFFINITY) > 0);

	test_assert(core_get_id() == TEST_CORE_AFFINITY);

	return (do_thread_specified_source(arg));
}

/**
 * @brief Stress Test: Specified Thread Affinity
 */
static void test_stress_mailbox_thread_specified_affinity(void)
{
	int local;
	kthread_t tid[TEST_THREAD_SPECIFIED - 1];

	local = knode_get_num();

	if (local == SLAVE_NODENUM)
	{
		nanvix_fence_init(&_fence, TEST_THREAD_SPECIFIED);

		/* Create threads. */
		for (int i = 1; i < TEST_THREAD_SPECIFIED; ++i)
			test_assert(kthread_create(&tid[i - 1], do_thread_specified_affinity, ((void *)(intptr_t) i)) == 0);

		do_thread_specified_affinity(0);

		/* Join threads. */
		for (int i = 1; i < TEST_THREAD_SPECIFIED; ++i)
			test_assert(kthread_join(tid[i - 1], NULL) == 0);
	}
	else if (local == MASTER_NODENUM)
		do_thread_specified_affinity(0);

	test_assert(kthread_set_affinity(KTHREAD_AFFINITY_DEFAULT) == TEST_THREAD_AFFINITY);
}

/*============================================================================*
 * Stress Test: Mailbox Thread Multiplexing Broadcast                         *
 *============================================================================*/

PRIVATE void * do_thread_multiplexing_broadcast(void * arg)
{
	int tid;
	int nports;
	int local;
	int remote;
	int mbxids[TEST_THREAD_MBXID_NUM];
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
				if (j == (tid + nports * TEST_THREAD_MAX))
					test_assert((mbxids[nports++] = kmailbox_open(remote, j)) >= 0);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				
				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);

			nanvix_fence(&_fence);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				if (j == (tid + nports * TEST_THREAD_MAX))
					test_assert((mbxids[nports++] = kmailbox_create(local, j)) >= 0);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					message[0] = local;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == remote);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_unlink(mbxids[j]) == 0);

			nanvix_fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Broadcast
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_broadcast(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	nanvix_fence_init(&_fence, TEST_THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_broadcast, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_broadcast(0);

	/* Join threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
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
	int mbxids[TEST_THREAD_MBXID_NUM];
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
				if (j == (tid + nports * TEST_THREAD_MAX))
					test_assert((mbxids[nports++] = kmailbox_open(remote, j)) >= 0);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);

			nanvix_fence(&_fence);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				if (j == (tid + nports * TEST_THREAD_MAX))
					test_assert((mbxids[nports++] = kmailbox_create(local, j)) >= 0);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					message[0] = local;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == remote);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_unlink(mbxids[j]) == 0);

			nanvix_fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Gather
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_gather(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	nanvix_fence_init(&_fence, TEST_THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_gather, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_gather(0);

	/* Join threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
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
	int inboxes[TEST_THREAD_MBXID_NUM];
	int outboxes[TEST_THREAD_MBXID_NUM];
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
			if (j == (tid + nports * TEST_THREAD_MAX))
			{
				test_assert((inboxes[nports] = kmailbox_create(local, j)) >= 0);
				test_assert((outboxes[nports] = kmailbox_open(remote, j)) >= 0);
				nports++;
			}

			nanvix_fence(&_fence);
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

				nanvix_fence(&_fence);
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

				nanvix_fence(&_fence);
			}
		}

		for (int j = 0; j < nports; ++j)
		{
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}

		nanvix_fence(&_fence);
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Ping-Pong
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_pingpong(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	nanvix_fence_init(&_fence, TEST_THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_pingpong, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_pingpong(0);

	/* Join threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);
}

/*============================================================================*
 * Stress Test: Mailbox Thread Multiplexing Affinity                          *
 *============================================================================*/

#define PPORT(x) (tid + x * TEST_THREAD_MAX)

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Ping-Pong
 */
PRIVATE void * do_thread_multiplexing_affinity(void * arg)
{
	int tid;
	int local;
	int remote;
	int nports;
	int inboxes[TEST_THREAD_MBXID_NUM];
	int outboxes[TEST_THREAD_MBXID_NUM];
	char msg_in[KMAILBOX_MESSAGE_SIZE];
	char msg_out[KMAILBOX_MESSAGE_SIZE];

	/* Change affinity. */
	test_assert(kthread_set_affinity(TEST_THREAD_AFFINITY) > 0);
	test_assert(core_get_id() == TEST_CORE_AFFINITY);

	tid = ((int)(intptr_t) arg);

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	msg_out[0] = local;

	for (int i = 0; i < NSETUPS_AFFINITY; ++i)
	{
		nports = 0;
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			if (j == (tid + nports * TEST_THREAD_MAX))
			{
				test_assert((inboxes[nports] = kmailbox_create(local, j)) >= 0);
				test_assert((outboxes[nports] = kmailbox_open(remote, j)) >= 0);
				nports++;
			}

			nanvix_fence(&_fence);
		}

		if (local == MASTER_NODENUM)
		{
			for (int j = 0; j < NCOMMS_AFFINITY; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					msg_in[0] = local;
					test_assert(kmailbox_read(inboxes[k], msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(msg_in[0] == remote);

					test_assert(kmailbox_write(outboxes[k], msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				}

				nanvix_fence(&_fence);
			}
		}
		else
		{
			for (int j = 0; j < NCOMMS_AFFINITY; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					test_assert(kmailbox_write(outboxes[k], msg_out, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

					msg_in[0] = local;
					test_assert(kmailbox_read(inboxes[k], msg_in, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(msg_in[0] == remote);
				}

				nanvix_fence(&_fence);
			}
		}

		for (int j = 0; j < nports; ++j)
		{
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}

		nanvix_fence(&_fence);
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Ping-Pong
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_affinity(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	nanvix_fence_init(&_fence, TEST_THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_affinity, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_affinity(0);

	/* Join threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);

	test_assert(kthread_set_affinity(KTHREAD_AFFINITY_DEFAULT) == TEST_THREAD_AFFINITY);
}

/*============================================================================*
 * Stress Test: Mailbox Thread Multiplexing Broadcast Local                   *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Broadcast Local
 */
PRIVATE void * do_thread_multiplexing_broadcast_local(void * arg)
{
	int tid;
	int local;
	int nports;
	int mbxids[TEST_THREAD_NPORTS];
	char message[KMAILBOX_MESSAGE_SIZE];

	tid = ((int)(intptr_t) arg);

	local = knode_get_num();

	if (tid == 0)
	{
		message[0] = 1;

		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < (TEST_THREAD_NPORTS - 1); ++j)
			{
				test_assert((mbxids[nports++] = kmailbox_open(local, j)) >= 0);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
				
				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);

			nanvix_fence(&_fence);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < (TEST_THREAD_NPORTS - 1); ++j)
			{
				if (j == ((tid - 1) + nports * (TEST_THREAD_MAX - 1)))
					test_assert((mbxids[nports++] = kmailbox_create(local, j)) >= 0);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					message[0] = 0;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == 1);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_unlink(mbxids[j]) == 0);

			nanvix_fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Broadcast Local
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_broadcast_local(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	nanvix_fence_init(&_fence, TEST_THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_broadcast_local, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_broadcast_local(0);

	/* Join threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);
}


/*============================================================================*
 * Stress Test: Mailbox Thread Multiplexing Gather Local                      *
 *============================================================================*/

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Gather Local
 */
PRIVATE void * do_thread_multiplexing_gather_local(void * arg)
{
	int tid;
	int local;
	int nports;
	int mbxids[TEST_THREAD_NPORTS];
	char message[KMAILBOX_MESSAGE_SIZE];

	tid = ((int)(intptr_t) arg);

	local = knode_get_num();

	if (tid != 0)
	{
		message[0] = 1;

		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < (TEST_THREAD_NPORTS - 1); ++j)
			{
				if (j == ((tid - 1) + nports * (TEST_THREAD_MAX - 1)))
					test_assert((mbxids[nports++] = kmailbox_open(local, j)) >= 0);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
					test_assert(kmailbox_write(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_close(mbxids[j]) == 0);

			nanvix_fence(&_fence);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < (TEST_THREAD_NPORTS - 1); ++j)
			{
				test_assert((mbxids[nports++] = kmailbox_create(local, j)) >= 0);

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					message[0] = 0;
					test_assert(kmailbox_read(mbxids[k], message, KMAILBOX_MESSAGE_SIZE) == KMAILBOX_MESSAGE_SIZE);
					test_assert(message[0] == 1);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
				test_assert(kmailbox_unlink(mbxids[j]) == 0);

			nanvix_fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Mailbox Thread Multiplexing Gather
 */
PRIVATE void test_stress_mailbox_thread_multiplexing_gather_local(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	nanvix_fence_init(&_fence, TEST_THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_gather_local, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_gather_local(0);

	/* Join threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief API tests.
 */
static struct test mailbox_tests_api[] = {
	{ test_api_mailbox_create_unlink,         "[test][mailbox][api] mailbox create unlink         [passed]" },
	{ test_api_mailbox_open_close,            "[test][mailbox][api] mailbox open close            [passed]" },
	{ test_api_mailbox_get_volume,            "[test][mailbox][api] mailbox get volume            [passed]" },
	{ test_api_mailbox_get_latency,           "[test][mailbox][api] mailbox get latency           [passed]" },
	{ test_api_mailbox_get_counters,          "[test][mailbox][api] mailbox get counters          [passed]" },
	{ test_api_mailbox_read_write,            "[test][mailbox][api] mailbox read write            [passed]" },
	{ test_api_mailbox_virtualization,        "[test][mailbox][api] mailbox virtualization        [passed]" },
	{ test_api_mailbox_multiplexation,        "[test][mailbox][api] mailbox multiplexation        [passed]" },
	{ test_api_mailbox_multiplexation_2,      "[test][mailbox][api] mailbox multiplexation 2      [passed]" },
	{ test_api_mailbox_multiplexation_3,      "[test][mailbox][api] mailbox multiplexation 3      [passed]" },
	{ test_api_mailbox_pending_msg_unlink,    "[test][mailbox][api] mailbox pending msg unlink    [passed]" },
	{ test_api_mailbox_msg_forwarding,        "[test][mailbox][api] mailbox message forwarding    [passed]" },
	{ NULL,                                    NULL                                                         },
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
	{ test_fault_mailbox_invalid_set_remote, "[test][mailbox][fault] mailbox invalid set remote [passed]" },
	{ test_fault_mailbox_bad_set_remote,     "[test][mailbox][fault] mailbox bad set remote     [passed]" },
	{ test_fault_mailbox_double_set_remote,  "[test][mailbox][fault] mailbox double set remote  [passed]" },
	{ test_fault_mailbox_bad_mbxid,          "[test][mailbox][fault] mailbox bad mbxid          [passed]" },
	{ NULL,                                   NULL                                                        },
};

/**
 * @brief Stress tests.
 */
PRIVATE struct test mailbox_tests_stress[] = {
	/* Intra-Cluster API Tests */
	{ test_stress_mailbox_create_unlink,                       "[test][mailbox][stress] mailbox create unlink                       [passed]" },
	{ test_stress_mailbox_open_close,                          "[test][mailbox][stress] mailbox open close                          [passed]" },
	{ test_stress_mailbox_broadcast,                           "[test][mailbox][stress] mailbox broadcast                           [passed]" },
	{ test_stress_mailbox_gather,                              "[test][mailbox][stress] mailbox gather                              [passed]" },
	{ test_stress_mailbox_pingpong,                            "[test][mailbox][stress] mailbox ping-pong                           [passed]" },
	{ test_stress_mailbox_multiplexing_broadcast,              "[test][mailbox][stress] mailbox multiplexing broadcast              [passed]" },
	{ test_stress_mailbox_multiplexing_gather,                 "[test][mailbox][stress] mailbox multiplexing gather                 [passed]" },
	{ test_stress_mailbox_multiplexing_pingpong,               "[test][mailbox][stress] mailbox multiplexing ping-pong              [passed]" },
	{ test_stress_mailbox_thread_specified_source,             "[test][mailbox][stress] mailbox thread specified source             [passed]" },
#if (CORE_SUPPORTS_MULTITHREADING && !__NANVIX_MICROKERNEL_STATIC_SCHED)
	{ test_stress_mailbox_thread_specified_affinity,           "[test][mailbox][stress] mailbox thread specified affinity           [passed]" },
#endif
	{ test_stress_mailbox_thread_multiplexing_broadcast,       "[test][mailbox][stress] mailbox thread multiplexing broadcast       [passed]" },
	{ test_stress_mailbox_thread_multiplexing_gather,          "[test][mailbox][stress] mailbox thread multiplexing gather          [passed]" },
	{ test_stress_mailbox_thread_multiplexing_pingpong,        "[test][mailbox][stress] mailbox thread multiplexing ping-pong       [passed]" },
#if (CORE_SUPPORTS_MULTITHREADING && !__NANVIX_MICROKERNEL_STATIC_SCHED)
	{ test_stress_mailbox_thread_multiplexing_affinity,        "[test][mailbox][stress] mailbox thread multiplexing affinity        [passed]" },
#endif
	{ test_stress_mailbox_thread_multiplexing_broadcast_local, "[test][mailbox][stress] mailbox thread multiplexing broadcast local [passed]" },
	{ test_stress_mailbox_thread_multiplexing_gather_local,    "[test][mailbox][stress] mailbox thread multiplexing gather local    [passed]" },
	{ NULL,                                                     NULL                                                                          },
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

			test_barrier_nodes();

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

			test_barrier_nodes();

			if (nodenum == MASTER_NODENUM)
				nanvix_puts(mailbox_tests_stress[i].name);
		}
	}
}

#endif /* __TARGET_HAS_MAILBOX */
