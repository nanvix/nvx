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

#include <nanvix/sys/portal.h>
#include <nanvix/sys/noc.h>
#include <posix/errno.h>

#include "test.h"

#if __TARGET_HAS_PORTAL

/**
 * @brief Test's parameters
 */
#define NR_NODES       2
#define NR_NODES_MAX   PROCESSOR_NOC_NODES_NUM
#define MESSAGE_SIZE   1024
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
 * @brief API Test: Portal Create Unlink
 */
static void test_api_portal_create_unlink(void)
{
	int local;
	int remote;
	int portalid;

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid = kportal_create(local, 0)) >= 0);
	test_assert(kportal_unlink(portalid) == 0);

	test_assert((portalid = kportal_create(local, 0)) >= 0);
	test_assert(kportal_allow(portalid, remote, 0) >= 0);
	test_assert(kportal_unlink(portalid) == 0);
}

/*============================================================================*
 * API Test: Open Close                                                       *
 *============================================================================*/

/**
 * @brief API Test: Portal Open Close
 */
static void test_api_portal_open_close(void)
{
	int local;
	int remote;
	int portalid;

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid = kportal_open(local, remote, 0)) >= 0);
	test_assert(kportal_close(portalid) == 0);
}

/*============================================================================*
 * API Test: Get volume                                                       *
 *============================================================================*/

/**
 * @brief API Test: Portal Get volume
 */
static void test_api_portal_get_volume(void)
{
	int local;
	int remote;
	int portal_in;
	int portal_out;
	size_t volume;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portal_in = kportal_create(local, 0)) >= 0);
	test_assert((portal_out = kportal_open(local, remote, 0)) >= 0);

		test_assert(kportal_ioctl(portal_in, KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == 0);

		test_assert(kportal_ioctl(portal_out, KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == 0);

	test_assert(kportal_close(portal_out) == 0);
	test_assert(kportal_unlink(portal_in) == 0);
}

/*============================================================================*
 * API Test: Get latency                                                      *
 *============================================================================*/

/**
 * @brief API Test: Portal Get latency
 */
static void test_api_portal_get_latency(void)
{
	int local;
	int remote;
	int portal_in;
	int portal_out;
	uint64_t latency;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portal_in = kportal_create(local, 0)) >= 0);
	test_assert((portal_out = kportal_open(local, remote, 0)) >= 0);

		test_assert(kportal_ioctl(portal_in, KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);

		test_assert(kportal_ioctl(portal_out, KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency == 0);

	test_assert(kportal_close(portal_out) == 0);
	test_assert(kportal_unlink(portal_in) == 0);
}

/*============================================================================*
 * API Test: Read Write 2 CC                                                  *
 *============================================================================*/

/**
 * @brief API Test: Read Write 2 CC
 *
 * @bug FIXME: Call kportal_wait() when the kernel properly supports it.
 */
static void test_api_portal_read_write(void)
{
	int local;
	int remote;
	int portal_in;
	int portal_out;
	size_t volume;
	uint64_t latency;
	char message[MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portal_in = kportal_create(local, 0)) >= 0);
	test_assert((portal_out = kportal_open(local, remote, 0)) >= 0);

	test_assert(kportal_ioctl(portal_in, KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kportal_ioctl(portal_in, KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);

	test_assert(kportal_ioctl(portal_out, KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == 0);
	test_assert(kportal_ioctl(portal_out, KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency == 0);

	if (local == MASTER_NODENUM)
	{
		for (unsigned i = 0; i < NITERATIONS; i++)
		{
			kmemset(message, 0, MESSAGE_SIZE);

			test_assert(kportal_allow(portal_in, remote, 0) == 0);
			test_assert(kportal_aread(portal_in, message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_in) == 0);
		#endif

			for (unsigned j = 0; j < MESSAGE_SIZE; ++j)
				test_assert(message[j] == 1);

			kmemset(message, 2, MESSAGE_SIZE);

			test_assert(kportal_awrite(portal_out, message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out) == 0);
		#endif
		}
	}
	else
	{
		for (unsigned i = 0; i < NITERATIONS; i++)
		{
			kmemset(message, 1, MESSAGE_SIZE);

			test_assert(kportal_awrite(portal_out, message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out) == 0);
		#endif

			kmemset(message, 0, MESSAGE_SIZE);

			test_assert(kportal_allow(portal_in, remote, 0) == 0);
			test_assert(kportal_aread(portal_in, message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_in) == 0);
		#endif

			for (unsigned j = 0; j < MESSAGE_SIZE; ++j)
				test_assert(message[j] == 2);
		}
	}

	test_assert(kportal_ioctl(portal_in, KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * MESSAGE_SIZE));
	test_assert(kportal_ioctl(portal_in, KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(kportal_ioctl(portal_out, KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
	test_assert(volume == (NITERATIONS * MESSAGE_SIZE));
	test_assert(kportal_ioctl(portal_out, KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
	test_assert(latency > 0);

	test_assert(kportal_close(portal_out) == 0);
	test_assert(kportal_unlink(portal_in) == 0);
}

/*============================================================================*
 * API Test: Virtualization                                                   *
 *============================================================================*/

#define TEST_VIRTUALIZATION_PORTALS_NR 10

/**
 * @brief API Test: Virtualization of HW portals.
 */
static void test_api_portal_virtualization(void)
{
	int local;
	int remote;
	int portal_in[TEST_VIRTUALIZATION_PORTALS_NR];
	int portal_out[TEST_VIRTUALIZATION_PORTALS_NR];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	/* Creates multiple virtual portals. */
	for (unsigned i = 0; i < TEST_VIRTUALIZATION_PORTALS_NR; ++i)
	{
		test_assert((portal_in[i] = kportal_create(local, i)) >= 0);
		test_assert((portal_out[i] = kportal_open(local, remote, i)) >= 0);
	}

	/* Deletion of the created virtual portals. */
	for (unsigned i = 0; i < TEST_VIRTUALIZATION_PORTALS_NR; ++i)
	{
		test_assert(kportal_unlink(portal_in[i]) == 0);
		test_assert(kportal_close(portal_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Multiplexation                                                   *
 *============================================================================*/

/**
 * @brief Multiplex test parameters.
 */
#define TEST_MULTIPLEXATION_PORTAL_PAIRS 5

/**
 * @brief API Test: Multiplexation of virtual to hardware portals.
 *
 * @bug FIXME: Call kportal_wait() when the kernel properly supports it.
 */
static void test_api_portal_multiplexation(void)
{
	int local;
	int remote;
	int portal_in[TEST_MULTIPLEXATION_PORTAL_PAIRS];
	int portal_out[TEST_MULTIPLEXATION_PORTAL_PAIRS];
	size_t volume;
	uint64_t latency;
	char message[MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	/* Creates multiple virtual portals. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION_PORTAL_PAIRS; ++i)
	{
		test_assert((portal_in[i] = kportal_create(local, i)) >= 0);
		test_assert((portal_out[i] = kportal_open(local, remote, i)) >= 0);
	}

	/* Multiple write/read operations to test multiplexation. */
	if (local == MASTER_NODENUM)
	{
		for (unsigned i = 0; i < TEST_MULTIPLEXATION_PORTAL_PAIRS; ++i)
		{
			kmemset(message, (i - 1), MESSAGE_SIZE);

			test_assert(kportal_allow(portal_in[i], remote, i) == 0);
			test_assert(kportal_aread(portal_in[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_in[i]) == 0);
		#endif

			for (unsigned j = 0; j < MESSAGE_SIZE; ++j)
				test_assert((message[j] - i) == 0);

			kmemset(message, (i + 1), MESSAGE_SIZE);

			test_assert(kportal_awrite(portal_out[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out[i]) == 0);
		#endif
		}
	}
	else
	{
		for (unsigned i = 0; i < TEST_MULTIPLEXATION_PORTAL_PAIRS; ++i)
		{
			kmemset(message, i, MESSAGE_SIZE);

			test_assert(kportal_awrite(portal_out[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out[i]) == 0);
		#endif

			kmemset(message, i, MESSAGE_SIZE);

			test_assert(kportal_allow(portal_in[i], remote, i) == 0);
			test_assert(kportal_aread(portal_in[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_in[i]) == 0);
		#endif

			for (unsigned j = 0; j < MESSAGE_SIZE; ++j)
				test_assert((message[j] - i - 1) == 0);
		}
	}

	/* Checks the data volume transferred by each vportal. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION_PORTAL_PAIRS; ++i)
	{
		test_assert(kportal_ioctl(portal_in[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == MESSAGE_SIZE);
		test_assert(kportal_ioctl(portal_in[i], KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency > 0);

		test_assert(kportal_ioctl(portal_out[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == MESSAGE_SIZE);
		test_assert(kportal_ioctl(portal_out[i], KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
		test_assert(latency > 0);

		/* Deletion of the created virtual portals. */
		test_assert(kportal_unlink(portal_in[i]) == 0);
		test_assert(kportal_close(portal_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Allow                                                            *
 *============================================================================*/

/**
 * @brief API Test: Virtual portals allowing.
 *
 * @bug FIXME: Call kportal_wait() when the kernel properly supports it.
 */
static void test_api_portal_allow(void)
{
	int local;
	int remote;
	int portal_in[2];
	int portal_out[2];
	char message[MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		for (unsigned i = 0; i < 2; ++i)
			test_assert((portal_out[i] = kportal_open(local, remote, i)) >= 0);

		for (unsigned i = 0; i < 2; ++i)
		{
			test_assert(kportal_awrite(portal_out[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out[i]) == 0);
		#endif

			test_assert(kportal_close(portal_out[i]) == 0);
		}
	}
	else
	{
		/* Creates the input vportals. */
		for (unsigned i = 0; i < 2; ++i)
			test_assert((portal_in[i] = kportal_create(local, i)) >= 0);

		/* Allowing tests. */
		test_assert(kportal_allow(portal_in[0], remote, 0) == 0);
		test_assert(kportal_aread(portal_in[1], message, MESSAGE_SIZE) == -EACCES);
		test_assert(kportal_aread(portal_in[0], message, MESSAGE_SIZE) == MESSAGE_SIZE);
	#if 0
		test_assert(kportal_wait(portal_in[0]) == 0);
	#endif
		test_assert(kportal_aread(portal_in[0], message, MESSAGE_SIZE) == -EACCES);
		test_assert(kportal_allow(portal_in[1], remote, 1) == 0);
		test_assert(kportal_aread(portal_in[0], message, MESSAGE_SIZE) == -EACCES);
		test_assert(kportal_aread(portal_in[1], message, MESSAGE_SIZE) == MESSAGE_SIZE);
	#if 0
		test_assert(kportal_wait(portal_in[1]) == 0);
	#endif

		/* Unlinks the created vportals. */
		for (unsigned i = 0; i < 2; ++i)
			test_assert(kportal_unlink(portal_in[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Multiplexation - Out of Order                                    *
 *============================================================================*/

#define TEST_MULTIPLEXATION2_PORTAL_PAIRS 3

/**
 * @brief API Test: Multiplexation test to assert the correct working when
 * messages flow occur in diferent order than the expected.
 *
 * @bug FIXME: Call kportal_wait() when the kernel properly supports it.
 */
static void test_api_portal_multiplexation_2(void)
{
	int local;
	int remote;
	int portal_in[TEST_MULTIPLEXATION2_PORTAL_PAIRS];
	int portal_out[TEST_MULTIPLEXATION2_PORTAL_PAIRS];
	size_t volume;
	uint64_t latency;
	char message[MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	/* Creates multiple virtual portals. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION2_PORTAL_PAIRS; ++i)
	{
		test_assert((portal_in[i] = kportal_create(local, i)) >= 0);
		test_assert((portal_out[i] = kportal_open(local, remote, i)) >= 0);
	}

	if (local == MASTER_NODENUM)
	{
		/* Read the messages in descendant order. */
		for (int i = (TEST_MULTIPLEXATION2_PORTAL_PAIRS - 1); i >= 0; --i)
		{
			kmemset(message, i, MESSAGE_SIZE);

			test_assert(kportal_allow(portal_in[i], remote, i) == 0);
			test_assert(kportal_aread(portal_in[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_in[i]) == 0);
		#endif

			for (unsigned j = 0; j < MESSAGE_SIZE; ++j)
				test_assert((message[j] - i) == 0);

			/* Checks the data transfered by each vportal. */
			test_assert(kportal_ioctl(portal_in[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == MESSAGE_SIZE);
			test_assert(kportal_ioctl(portal_in[i], KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
			test_assert(latency > 0);
		}
	}
	else
	{
		for (unsigned i = 0; i < TEST_MULTIPLEXATION2_PORTAL_PAIRS; ++i)
		{
			kmemset(message, i, MESSAGE_SIZE);

			test_assert(kportal_awrite(portal_out[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out[i]) == 0);
		#endif

			test_assert(kportal_ioctl(portal_out[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == MESSAGE_SIZE);
			test_assert(kportal_ioctl(portal_out[i], KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
			test_assert(latency > 0);
		}
	}

	/* Deletion of the created virtual portals. */
	for (unsigned i = 0; i < TEST_MULTIPLEXATION2_PORTAL_PAIRS; ++i)
	{
		test_assert(kportal_unlink(portal_in[i]) == 0);
		test_assert(kportal_close(portal_out[i]) == 0);
	}
}

/*============================================================================*
 * API Test: Multiplexation - Messages Selection                              *
 *============================================================================*/

/* @note Define an even number to this constant. */
#define TEST_MULTIPLEXATION3_PORTAL_MSGS_NR 6
#define TEST_MULTIPLEXATION3_PORTAL_SELECT_NR (TEST_MULTIPLEXATION3_PORTAL_MSGS_NR / 2)

/**
 * @brief API Test: Multiplexation test to assert the correct message selection
 * of multiple messages.
 *
 * @details In this test, the MASTER node sends 4 messages but only 2 for open ports
 * on the SLAVE which need to select them correctly.
 *
 * @bug FIXME: Call kportal_wait() when the kernel properly supports it.
 */
static void test_api_portal_multiplexation_3(void)
{
	int local;
	int remote;
	int portal_in[TEST_MULTIPLEXATION3_PORTAL_SELECT_NR];
	int portal_out[TEST_MULTIPLEXATION3_PORTAL_MSGS_NR];
	int port;
	size_t volume;
	uint64_t latency;
	char message[MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		/* Opens the output vportals. */
		for (unsigned int i = 0; i < TEST_MULTIPLEXATION3_PORTAL_MSGS_NR; ++i)
			test_assert((portal_out[i] = kportal_open(local, remote, i)) >= 0);

		/* Sends all the messages to the SLAVE node. */
		for (int i = (TEST_MULTIPLEXATION3_PORTAL_MSGS_NR - 1); i >= 0; --i)
		{
			kmemset(message, i, MESSAGE_SIZE);

			test_assert(kportal_awrite(portal_out[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out[i]) == 0);
		#endif
		}

		/* Closes the opened vportals. */
		for (unsigned int i = 0; i < TEST_MULTIPLEXATION3_PORTAL_MSGS_NR; ++i)
		{
			test_assert(kportal_ioctl(portal_out[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == MESSAGE_SIZE);

			test_assert(kportal_close(portal_out[i]) == 0);
		}
	}
	else
	{
		/* Creates multiple virtual portals (half of the MASTER senders). */
		for (int i = 0; i < TEST_MULTIPLEXATION3_PORTAL_SELECT_NR; ++i)
			test_assert((portal_in[i] = kportal_create(local, (i*2))) >= 0);

		for (unsigned i = 0; i < TEST_MULTIPLEXATION3_PORTAL_SELECT_NR; ++i)
		{
			port = portal_in[i] % KPORTAL_PORT_NR;

			kmemset(message, -1, MESSAGE_SIZE);

			test_assert(kportal_allow(portal_in[i], remote, port) == 0);
			test_assert(kportal_aread(portal_in[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_in[i]) == 0);
		#endif

			for (unsigned j = 0; j < MESSAGE_SIZE; ++j)
				test_assert((message[j] - port) == 0);
		}

		/* Checks the data volume transferred by each vportal. */
		for (unsigned i = 0; i < TEST_MULTIPLEXATION3_PORTAL_SELECT_NR; ++i)
		{
			test_assert(kportal_ioctl(portal_in[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == MESSAGE_SIZE);
			test_assert(kportal_ioctl(portal_in[i], KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
			test_assert(latency > 0);

			/* Unlinks each vportal. */
			test_assert(kportal_unlink(portal_in[i]) == 0);
		}
	}
}

/*============================================================================*
 * API Test: Multiplexation - Multiple Messages                               *
 *============================================================================*/

/* @note Define an even number to this constant. */
#define TEST_MLTPX4_PORTAL_SEND_NR 4
#define TEST_MLTPX4_PORTAL_SELECT_NR (TEST_MLTPX4_PORTAL_SEND_NR / 2)
#define TEST_MLTPX4_PORTAL_SELECT_MSG (TEST_MLTPX4_PORTAL_SEND_NR / TEST_MLTPX4_PORTAL_SELECT_NR)

/**
 * @brief API Test: Multiplexation test to assert the message buffers tab
 * solution to support multiple messages on the same port simultaneously.
 *
 * @details In this test, the SLAVE sends 4 messages to 2 ports of the SLAVE.
 * The reads in the master are also made out of order to assure that there are
 * 2 messages queued for each vportal on MASTER.
 *
 * @bug FIXME: Call kportal_wait() when the kernel properly supports it.
 */
static void test_api_portal_multiplexation_4(void)
{
	int local;
	int remote;
	int portal_in[TEST_MLTPX4_PORTAL_SELECT_NR];
	int portal_out[TEST_MLTPX4_PORTAL_SEND_NR];
	int port;
	size_t volume;
	uint64_t latency;
	char message[MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == MASTER_NODENUM)
	{
		/* Creates multiple virtual portals. */
		for (unsigned i = 0; i < TEST_MLTPX4_PORTAL_SELECT_NR; ++i)
			test_assert((portal_in[i] = kportal_create(local, i)) >= 0);

		/* For each input vportal. */
		for (int i = (TEST_MLTPX4_PORTAL_SELECT_NR - 1); i >= 0; --i)
		{
			/* For each message it should receive. */
			for (int j = (TEST_MLTPX4_PORTAL_SELECT_MSG - 1); j >= 0; --j)
			{
				port = (i*2 + j);

				kmemset(message, -1, MESSAGE_SIZE);

				test_assert(kportal_allow(portal_in[i], remote, port) == 0);
				test_assert(kportal_aread(portal_in[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
			#if 0
				test_assert(kportal_wait(portal_in[i]) == 0);
			#endif

				/* Asserts the message data. */
				for (unsigned k = 0; k < MESSAGE_SIZE; ++k)
					test_assert((message[k] - port) == 0);
			}
		}

		/* Checks the data volume transferred by each vportal. */
		for (unsigned i = 0; i < TEST_MLTPX4_PORTAL_SELECT_NR; ++i)
		{
			test_assert(kportal_ioctl(portal_in[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == (MESSAGE_SIZE * TEST_MLTPX4_PORTAL_SELECT_MSG));
			test_assert(kportal_ioctl(portal_in[i], KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
			test_assert(latency > 0);

			/* Unlinks the created vportals. */
			test_assert(kportal_unlink(portal_in[i]) == 0);
		}
	}
	else
	{
		/* Opens the output vportals. */
		for (unsigned i = 0; i < TEST_MLTPX4_PORTAL_SEND_NR; ++i)
			test_assert((portal_out[i] = kportal_open(local, remote, (int) (i/TEST_MLTPX4_PORTAL_SELECT_MSG))) >= 0);

		/* Sends all the messages to the MASTER node. */
		for (unsigned i = 0; i < TEST_MLTPX4_PORTAL_SEND_NR; ++i)
		{
			kmemset(message, i, MESSAGE_SIZE);

			test_assert(kportal_awrite(portal_out[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out[i]) == 0);
		#endif
		}

		/* Checks the data volume transferred by each vportal. */
		for (unsigned i = 0; i < TEST_MLTPX4_PORTAL_SEND_NR; ++i)
		{
			test_assert(kportal_ioctl(portal_out[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == MESSAGE_SIZE);

			/* Closes the output vportals. */
			test_assert(kportal_close(portal_out[i]) == 0);
		}
	}
}

/*============================================================================*
 * API Test: Pending Messages - Unlink                                        *
 *============================================================================*/

#define TEST_PENDING_UNLINK_PORTAL_PAIRS 2

/**
 * @brief API Test: Test to assert if the kernel correctly avoids an unlink
 * call when there still messages addressed to the target vportal on message
 * buffers tab.
 *
 * @todo Uncomment kportal_wait() call when microkernel properly supports it.
 */
static void test_api_portal_pending_msg_unlink(void)
{
	int local;
	int remote;
	size_t volume;
	uint64_t latency;
	int portal_in[TEST_PENDING_UNLINK_PORTAL_PAIRS];
	int portal_out[TEST_PENDING_UNLINK_PORTAL_PAIRS];
	char message[MESSAGE_SIZE];

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		/* Opens the output portals. */
		for (unsigned i = 0; i < TEST_PENDING_UNLINK_PORTAL_PAIRS; ++i)
			test_assert((portal_out[i] = kportal_open(local, remote, i)) >= 0);

		for (int i = 0; i < TEST_PENDING_UNLINK_PORTAL_PAIRS; ++i)
			test_assert(kportal_awrite(portal_out[i], message, MESSAGE_SIZE) == MESSAGE_SIZE);
		#if 0
			test_assert(kportal_wait(portal_out[i]) == 0);
		#endif

		/* Checks the data volume transfered by each vportal. */
		for (unsigned i = 0; i < TEST_PENDING_UNLINK_PORTAL_PAIRS; ++i)
		{
			test_assert(kportal_ioctl(portal_out[i], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
			test_assert(volume == MESSAGE_SIZE);
			test_assert(kportal_ioctl(portal_out[i], KPORTAL_IOCTL_GET_LATENCY, &latency) == 0);
			test_assert(latency > 0);

			/* Closes the opened vportals. */
			test_assert(kportal_close(portal_out[i]) == 0);
		}
	}
	else
	{
		/* Creates the input portals. */
		for (unsigned i = 0; i < TEST_PENDING_UNLINK_PORTAL_PAIRS; ++i)
			test_assert((portal_in[i] = kportal_create(local, i)) >= 0);

		/* Unlink tests. */
		test_assert(kportal_allow(portal_in[1], remote, 1) == 0);
		test_assert(kportal_aread(portal_in[1], message, MESSAGE_SIZE) == MESSAGE_SIZE);
	#if 0
		test_assert(kportal_wait(portal_in[1]) == 0);
	#endif

		test_assert(kportal_ioctl(portal_in[1], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == MESSAGE_SIZE);

		test_assert(kportal_unlink(portal_in[1]) == 0);


		test_assert(kportal_unlink(portal_in[0]) == -EBUSY);

		test_assert(kportal_allow(portal_in[0], remote, 0) == 0);
		test_assert(kportal_aread(portal_in[0], message, MESSAGE_SIZE) == MESSAGE_SIZE);
	#if 0
		test_assert(kportal_wait(portal_in[0]) == 0);
	#endif

		test_assert(kportal_ioctl(portal_in[0], KPORTAL_IOCTL_GET_VOLUME, &volume) == 0);
		test_assert(volume == MESSAGE_SIZE);

		test_assert(kportal_unlink(portal_in[0]) == 0);
	}
}

/*============================================================================*
 * Fault Test: Invalid Create                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Create
 */
static void test_fault_portal_invalid_create(void)
{
	int local;
	int remote;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert(kportal_create(-1, 0) == -EINVAL);
	test_assert(kportal_create(PROCESSOR_NOC_NODES_NUM, 0) == -EINVAL);
	test_assert(kportal_create(remote, 0) == -EINVAL);
	test_assert(kportal_create(local, -1) == -EINVAL);
	test_assert(kportal_create(local, KPORTAL_PORT_NR) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Double Create                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Double Create
 */
static void test_fault_portal_double_create(void)
{
	int local;
	int portal;

	local = knode_get_num();

	test_assert((portal = kportal_create(local, 0)) >= 0);

	test_assert(kportal_create(local, 0) == -EBUSY);

	test_assert(kportal_unlink(portal) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Unlink                                                 *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Unlink
 */
static void test_fault_portal_invalid_unlink(void)
{
	test_assert(kportal_unlink(-1) == -EINVAL);
	test_assert(kportal_unlink(KPORTAL_MAX) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Double Unlink                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Double Unlink
 */
static void test_fault_portal_double_unlink(void)
{
	int local;
	int portalid1;
	int portalid2;

	local = knode_get_num();

	test_assert((portalid1 = kportal_create(local, 0)) >= 0);
	test_assert((portalid2 = kportal_create(local, 1)) >= 0);

	test_assert(kportal_unlink(portalid1) == 0);
	test_assert(kportal_unlink(portalid1) == -EBADF);
	test_assert(kportal_unlink(portalid2) == 0);
}

/*============================================================================*
 * Fault Test: Bad Unlink                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Unlink
 */
static void test_fault_portal_bad_unlink(void)
{
	int local;
	int remote;
	int portalid;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid = kportal_open(local, remote, 0)) >= 0);
	test_assert(kportal_unlink(portalid) == -EBADF);
	test_assert(kportal_close(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Open                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Open
 */
static void test_fault_portal_invalid_open(void)
{
	int local;
	int remote;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert(kportal_open(local, -1, 0) == -EINVAL);
	test_assert(kportal_open(local, PROCESSOR_NOC_NODES_NUM, 0) == -EINVAL);
	test_assert(kportal_open(local, local, 0) == -EINVAL);
	test_assert(kportal_open(-1, remote, 0) == -EINVAL);
	test_assert(kportal_open(PROCESSOR_NOC_NODES_NUM, remote, 0) == -EINVAL);
	test_assert(kportal_open(remote, remote, 0) == -EINVAL);
	test_assert(kportal_open(-1, -1, 0) == -EINVAL);
	test_assert(kportal_open(local, remote, -1) == -EINVAL);
	test_assert(kportal_open(local, remote, KPORTAL_PORT_NR) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Invalid Close                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Close
 */
static void test_fault_portal_invalid_close(void)
{
	test_assert(kportal_close(-1) == -EINVAL);
	test_assert(kportal_close(KPORTAL_MAX) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Double Close                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Double Close
 */
static void test_fault_portal_double_close(void)
{
	int local;
	int remote;
	int portalid1;
	int portalid2;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid1 = kportal_open(local, remote, 0)) >= 0);
	test_assert((portalid2 = kportal_open(local, remote, 1)) >= 0);

	test_assert(kportal_close(portalid1) == 0);
	test_assert(kportal_close(portalid1) == -EBADF);
	test_assert(kportal_close(portalid2) == 0);
}

/*============================================================================*
 * Fault Test: Bad Close                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Close
 */
static void test_fault_portal_bad_close(void)
{
	int local;
	int portalid;

	local = knode_get_num();

	test_assert((portalid = kportal_create(local, 0)) >= 0);
	test_assert(kportal_close(portalid) == -EBADF);
	test_assert(kportal_unlink(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid allow                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid allow
 */
static void test_fault_portal_invalid_allow(void)
{
	int local;
	int remote;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert(kportal_allow(-1, remote, 0) == -EINVAL);
	test_assert(kportal_allow(KPORTAL_MAX, remote, 0) == -EINVAL);
	test_assert(kportal_allow(0, -1, 0) == -EINVAL);
	test_assert(kportal_allow(0, local, 0) == -EINVAL);
	test_assert(kportal_allow(0, PROCESSOR_NOC_NODES_NUM, 0) == -EINVAL);
	test_assert(kportal_allow(0, remote, -1) == -EINVAL);
	test_assert(kportal_allow(0, remote, KPORTAL_PORT_NR) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Double allow                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Double allow
 */
static void test_fault_portal_double_allow(void)
{
	int local;
	int remote;
	int portalid;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid = kportal_create(local, 0)) >= 0);
	test_assert(kportal_allow(portalid, remote, 0) == 0);
	test_assert(kportal_allow(portalid, remote, 0) == -EBUSY);
	test_assert(kportal_unlink(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Bad allow                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Bad allow
 */
static void test_fault_portal_bad_allow(void)
{
	int local;
	int remote;
	int portalid;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid = kportal_open(local, remote, 0)) >= 0);
	test_assert(kportal_allow(portalid, remote, 0) == -EBADF);
	test_assert(kportal_close(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Read                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Read
 */
static void test_fault_portal_invalid_read(void)
{
	char buffer[MESSAGE_SIZE];

	test_assert(kportal_aread(-1, buffer, MESSAGE_SIZE) == -EINVAL);
	test_assert(kportal_aread(KPORTAL_MAX, buffer, MESSAGE_SIZE) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Read                                                       *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Read
 */
static void test_fault_portal_bad_read(void)
{
	int local;
	int remote;
	int portalid;
	char buffer[MESSAGE_SIZE];

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid = kportal_open(local, remote, 0)) >= 0);

	test_assert(kportal_aread(portalid, buffer, MESSAGE_SIZE) == -EBADF);

	test_assert(kportal_close(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Read Size                                              *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Read Size
 */
static void test_fault_portal_invalid_read_size(void)
{
	int portalid;
	int local;
	char buffer[MESSAGE_SIZE];

	local = knode_get_num();

	test_assert((portalid = kportal_create(local, 0)) >= 0);

		test_assert(kportal_aread(portalid, buffer, -1) == -EINVAL);
		test_assert(kportal_aread(portalid, buffer, 0) == -EINVAL);
		test_assert(kportal_aread(portalid, buffer, HAL_PORTAL_MAX_SIZE + 1) == -EINVAL);

	test_assert(kportal_unlink(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Null Read                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Null Read
 */
static void test_fault_portal_null_read(void)
{
	int portalid;
	int local;

	local = knode_get_num();

	test_assert((portalid = kportal_create(local, 0)) >= 0);

		test_assert(kportal_aread(portalid, NULL, MESSAGE_SIZE) == -EINVAL);

	test_assert(kportal_unlink(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Write                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Write
 */
static void test_fault_portal_invalid_write(void)
{
	char buffer[MESSAGE_SIZE];

	test_assert(kportal_awrite(-1, buffer, MESSAGE_SIZE) == -EINVAL);
	test_assert(kportal_awrite(KPORTAL_MAX, buffer, MESSAGE_SIZE) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Bad Write                                                      *
 *============================================================================*/

/**
 * @brief Fault Test: Bad Write
 */
static void test_fault_portal_bad_write(void)
{
	int local;
	int portalid;
	char buffer[MESSAGE_SIZE];

	local = knode_get_num();

	test_assert((portalid = kportal_create(local, 0)) >= 0);

		test_assert(kportal_awrite(portalid, buffer, MESSAGE_SIZE) == -EBADF);

	test_assert(kportal_unlink(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Write Size                                             *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Write Size
 */
static void test_fault_portal_invalid_write_size(void)
{
	int local;
	int remote;
	int portalid;
	char buffer[MESSAGE_SIZE];

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid = kportal_open(local, remote, 0)) >= 0);

		test_assert(kportal_awrite(portalid, buffer, -1) == -EINVAL);
		test_assert(kportal_awrite(portalid, buffer, 0) == -EINVAL);
		test_assert(kportal_awrite(portalid, buffer, HAL_PORTAL_MAX_SIZE + 1) == -EINVAL);

	test_assert(kportal_close(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Null Write                                                     *
 *============================================================================*/

/**
 * @brief Fault Test: Null Write
 */
static void test_fault_portal_null_write(void)
{
	int local;
	int remote;
	int portalid;

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portalid = kportal_open(local, remote, 0)) >= 0);

		test_assert(kportal_awrite(portalid, NULL, MESSAGE_SIZE) == -EINVAL);

	test_assert(kportal_close(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Invalid Wait                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid Wait
 */
static void test_fault_portal_invalid_wait(void)
{
	test_assert(kportal_wait(-1) == -EINVAL);
	test_assert(kportal_wait(KPORTAL_MAX) == -EINVAL);
}

/*============================================================================*
 * Fault Test: Invalid ioctl                                                  *
 *============================================================================*/

/**
 * @brief Fault Test: Invalid ioctl
 */
static void test_fault_portal_invalid_ioctl(void)
{
	int local;
	int portalid;
	size_t volume;
	uint64_t latency;

	test_assert(kportal_ioctl(-1, KPORTAL_IOCTL_GET_VOLUME, &volume) == -EINVAL);
	test_assert(kportal_ioctl(-1, KPORTAL_IOCTL_GET_LATENCY, &latency) == -EINVAL);
	test_assert(kportal_ioctl(KPORTAL_MAX, KPORTAL_IOCTL_GET_VOLUME, &volume) == -EINVAL);
	test_assert(kportal_ioctl(KPORTAL_MAX, KPORTAL_IOCTL_GET_LATENCY, &latency) == -EINVAL);

	local = knode_get_num();

	test_assert((portalid = kportal_create(local, 0)) >=  0);

		test_assert(kportal_ioctl(portalid, -1, &volume) == -ENOTSUP);

	test_assert(kportal_unlink(portalid) == 0);
}

/*============================================================================*
 * Fault Test: Bad portalid                                                   *
 *============================================================================*/

/**
 * @brief Fault Test: Bad portalid
 */
static void test_fault_portal_bad_portalid(void)
{
	int portal_in;
	int portal_out;
	int remote;
	int local;
	size_t volume;
	char buffer[MESSAGE_SIZE];

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	test_assert((portal_in = kportal_create(local, 0)) >= 0);
	test_assert((portal_out = kportal_open(local, remote, 0)) >= 0);

	test_assert(kportal_close(portal_out + 1) == -EBADF);
	test_assert(kportal_unlink(portal_in + 1) == -EBADF);
	test_assert(kportal_allow(portal_in + 1, remote, 0) == -EBADF)
	test_assert(kportal_aread(portal_in + 1, buffer, MESSAGE_SIZE) == -EBADF);
	test_assert(kportal_awrite(portal_in + 1, buffer, MESSAGE_SIZE) == -EBADF);
	test_assert(kportal_wait(portal_in + 1) == -EBADF);
	test_assert(kportal_ioctl(portal_in + 1, KPORTAL_IOCTL_GET_VOLUME, &volume) == -EBADF);

	test_assert(kportal_close(portal_out) == 0);
	test_assert(kportal_unlink(portal_in) == 0);
}

/*============================================================================*
 * Test Driver                                                                *
 *============================================================================*/

/**
 * @brief Unit tests.
 */
static struct test portal_tests_api[] = {
	{ test_api_portal_create_unlink,      "[test][portal][api] portal create unlink      [passed]" },
	{ test_api_portal_open_close,         "[test][portal][api] portal open close         [passed]" },
	{ test_api_portal_get_volume,         "[test][portal][api] portal get volume         [passed]" },
	{ test_api_portal_get_latency,        "[test][portal][api] portal get latency        [passed]" },
	{ test_api_portal_read_write,         "[test][portal][api] portal read write         [passed]" },
	{ test_api_portal_virtualization,     "[test][portal][api] portal virtualization     [passed]" },
	{ test_api_portal_multiplexation,     "[test][portal][api] portal multiplexation     [passed]" },
	{ test_api_portal_allow,              "[test][portal][api] portal allow              [passed]" },
	{ test_api_portal_multiplexation_2,   "[test][portal][api] portal multiplexation 2   [passed]" },
	{ test_api_portal_multiplexation_3,   "[test][portal][api] portal multiplexation 3   [passed]" },
	{ test_api_portal_multiplexation_4,   "[test][portal][api] portal multiplexation 4   [passed]" },
	{ test_api_portal_pending_msg_unlink, "[test][portal][api] portal pending msg unlink [passed]" },
	{ NULL,                                NULL                                                    },
};

/**
 * @brief Unit tests.
 */
static struct test portal_tests_fault[] = {
	{ test_fault_portal_invalid_create,     "[test][portal][fault] portal invalid create     [passed]" },
	{ test_fault_portal_double_create,      "[test][portal][fault] portal double create      [passed]" },
	{ test_fault_portal_invalid_unlink,     "[test][portal][fault] portal invalid unlink     [passed]" },
	{ test_fault_portal_double_unlink,      "[test][portal][fault] portal double unlink      [passed]" },
	{ test_fault_portal_bad_unlink,         "[test][portal][fault] portal bad unlink         [passed]" },
	{ test_fault_portal_invalid_open,       "[test][portal][fault] portal invalid open       [passed]" },
	{ test_fault_portal_invalid_close,      "[test][portal][fault] portal invalid close      [passed]" },
	{ test_fault_portal_double_close,       "[test][portal][fault] portal double close       [passed]" },
	{ test_fault_portal_bad_close,          "[test][portal][fault] portal bad close          [passed]" },
	{ test_fault_portal_invalid_allow,      "[test][portal][fault] portal invalid allow      [passed]" },
	{ test_fault_portal_double_allow,       "[test][portal][fault] portal double allow       [passed]" },
	{ test_fault_portal_bad_allow,          "[test][portal][fault] portal bad allow          [passed]" },
	{ test_fault_portal_invalid_read,       "[test][portal][fault] portal invalid read       [passed]" },
	{ test_fault_portal_bad_read,           "[test][portal][fault] portal bad read           [passed]" },
	{ test_fault_portal_invalid_read_size,  "[test][portal][fault] portal invalid read size  [passed]" },
	{ test_fault_portal_null_read,          "[test][portal][fault] portal null read          [passed]" },
	{ test_fault_portal_invalid_write,      "[test][portal][fault] portal invalid write      [passed]" },
	{ test_fault_portal_bad_write,          "[test][portal][fault] portal bad write          [passed]" },
	{ test_fault_portal_invalid_write_size, "[test][portal][fault] portal invalid write size [passed]" },
	{ test_fault_portal_null_write,         "[test][portal][fault] portal null write         [passed]" },
	{ test_fault_portal_invalid_wait,       "[test][portal][fault] portal invalid wait       [passed]" },
	{ test_fault_portal_invalid_ioctl,      "[test][portal][fault] portal invalid ioctl      [passed]" },
	{ test_fault_portal_bad_portalid,       "[test][portal][fault] portal bad portalid       [passed]" },
	{ NULL,                                  NULL                                                      },
};

/**
 * The test_thread_mgmt() function launches testing units on thread manager.
 *
 * @author Pedro Henrique Penna
 */
void test_portal(void)
{
	int local;
	int remote;
	int portal_in;
	int portal_out;

	local = knode_get_num();

	if (local == MASTER_NODENUM || local == SLAVE_NODENUM)
	{
		/* API Tests */
		if (local == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------");
		for (unsigned i = 0; portal_tests_api[i].test_fn != NULL; i++)
		{
			portal_tests_api[i].test_fn();

			if (local == MASTER_NODENUM)
				nanvix_puts(portal_tests_api[i].name);
		}

		remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

		/* Create dummy vportals to help on fault assertions. */
		test_assert((portal_in = kportal_create(local, KPORTAL_PORT_NR - 1)) >= 0);
		test_assert((portal_out = kportal_open(local, remote, KPORTAL_PORT_NR - 1)) >= 0);

		/* Fault Tests */
		if (local == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------");
		for (unsigned i = 0; portal_tests_fault[i].test_fn != NULL; i++)
		{
			portal_tests_fault[i].test_fn();

			if (local == MASTER_NODENUM)
				nanvix_puts(portal_tests_fault[i].name);
		}

		/* Destroy the dummy vportals. */
		test_assert(kportal_close(portal_out) == 0);
		test_assert(kportal_unlink(portal_in) == 0);
	}
}

#endif /* __TARGET_HAS_PORTAL */
