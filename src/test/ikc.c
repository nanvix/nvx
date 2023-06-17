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
#include <nanvix/sys/portal.h>
#include <nanvix/sys/noc.h>
#include <nanvix/runtime/fence.h>
#include <posix/errno.h>

#include "test.h"

#if __TARGET_HAS_MAILBOX && __TARGET_HAS_PORTAL

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

/**
 * @name Threads 
 */
/**{*/
#define TEST_THREAD_MAX       (CORES_NUM - 1)
#define TEST_THREAD_IKCID_NUM ((TEST_THREAD_NPORTS / TEST_THREAD_MAX) + 1)
#define TEST_CORE_AFFINITY    (1)
#define TEST_THREAD_AFFINITY  (1 << TEST_CORE_AFFINITY)
/**}*/

/*============================================================================*
 * Global variables                                                           *
 *============================================================================*/

PRIVATE char message[PORTAL_SIZE_LARGE];
PRIVATE char message_in[TEST_THREAD_MAX][PORTAL_SIZE];
PRIVATE char message_out[PORTAL_SIZE];

/**
 * @brief Simple fence used for thread synchronization.
 */
PRIVATE struct nanvix_fence _fence;

/*============================================================================*
 * Stress Tests                                                               *
 *============================================================================*/

/*============================================================================*
 * Stress Test: Portal Create Unlink                                          *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Create Unlink
 */
PRIVATE void test_stress_ikc_create_unlink(void)
{
	int local;
	int mbxid;
	int portalid;

	local = knode_get_num();

	for (int i = 0; i < NSETUPS; ++i)
	{
		test_assert((mbxid = kmailbox_create(local, 0)) >= 0);
		test_assert(kmailbox_unlink(mbxid) == 0);

		test_assert((portalid = kportal_create(local, 0)) >= 0);
		test_assert(kportal_unlink(portalid) == 0);
	}
}

/*============================================================================*
 * Stress Test: Portal Open Close                                             *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Open Close
 */
PRIVATE void test_stress_ikc_open_close(void)
{
	int local;
	int remote;
	int mbxid;
	int portalid;

	local  = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	for (int i = 0; i < NSETUPS; ++i)
	{
		test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);
		test_assert(kmailbox_close(mbxid) == 0);

		test_assert((portalid = kportal_open(local, remote, 0)) >= 0);
		test_assert(kportal_close(portalid) == 0);
	}
}

/*============================================================================*
 * Stress Test: Portal Broadcast                                              *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Broadcast
 */
PRIVATE void test_stress_ikc_broadcast(void)
{
	int local;
	int remote;
	int mbxid;
	int portalid;
	
	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == MASTER_NODENUM)
	{
		kmemset(message, (char) local, PORTAL_SIZE);

		for (int i = 0; i < NSETUPS; ++i)
		{
			test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);
			test_assert((portalid = kportal_open(local, remote, 0)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				test_assert(kmailbox_write(mbxid, message, MAILBOX_SIZE) == MAILBOX_SIZE);
				test_assert(kportal_write(portalid, message, PORTAL_SIZE) == PORTAL_SIZE);
			}

			test_assert(kportal_close(portalid) == 0);
			test_assert(kmailbox_close(mbxid) == 0);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			test_assert((mbxid = kmailbox_create(local, 0)) >= 0);
			test_assert((portalid = kportal_create(local, 0)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				kmemset(message, (char) local, MAILBOX_SIZE);
				test_assert(kmailbox_read(mbxid, message, MAILBOX_SIZE) == MAILBOX_SIZE);
				for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
					test_assert(message[l] == remote);

				kmemset(message, (char) local, PORTAL_SIZE);
				test_assert(kportal_allow(portalid, remote, 0) >= 0);
				test_assert(kportal_read(portalid, message, PORTAL_SIZE) == PORTAL_SIZE);
				for (unsigned l = 0; l < PORTAL_SIZE; ++l)
					test_assert(message[l] == remote);
			}

			test_assert(kportal_unlink(portalid) == 0);
			test_assert(kmailbox_unlink(mbxid) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Portal Gather                                                 *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Gather
 */
PRIVATE void test_stress_ikc_gather(void)
{
	int local;
	int remote;
	int mbxid;
	int portalid;

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		kmemset(message, (char) local, PORTAL_SIZE);

		for (int i = 0; i < NSETUPS; ++i)
		{
			test_assert((mbxid = kmailbox_open(remote, 0)) >= 0);
			test_assert((portalid = kportal_open(local, remote, 0)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				test_assert(kmailbox_write(mbxid, message, MAILBOX_SIZE) == MAILBOX_SIZE);
				test_assert(kportal_write(portalid, message, PORTAL_SIZE) == PORTAL_SIZE);
			}

			test_assert(kportal_close(portalid) == 0);
			test_assert(kmailbox_close(mbxid) == 0);
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			test_assert((mbxid = kmailbox_create(local, 0)) >= 0);
			test_assert((portalid = kportal_create(local, 0)) >= 0);

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				kmemset(message, (char) local, MAILBOX_SIZE);
				test_assert(kmailbox_read(mbxid, message, MAILBOX_SIZE) == MAILBOX_SIZE);
				for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
					test_assert(message[l] == remote);

				kmemset(message, (char) local, PORTAL_SIZE);
				test_assert(kportal_allow(portalid, remote, 0) >= 0);
				test_assert(kportal_read(portalid, message, PORTAL_SIZE) == PORTAL_SIZE);
				for (unsigned l = 0; l < PORTAL_SIZE; ++l)
					test_assert(message[l] == remote);
			}

			test_assert(kportal_unlink(portalid) == 0);
			test_assert(kmailbox_unlink(mbxid) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Portal Ping-Pong                                              *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Ping-Pong
 */
PRIVATE void test_stress_ikc_pingpong(void)
{
	int local;
	int remote;
	int inbox;
	int outbox;
	int inportal;
	int outportal;

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	kmemset(message_out, (char) local, PORTAL_SIZE);

	for (int i = 0; i < NSETUPS; ++i)
	{
		test_assert((inbox = kmailbox_create(local, 0)) >= 0);
		test_assert((outbox = kmailbox_open(remote, 0)) >= 0);
		test_assert((inportal = kportal_create(local, 0)) >= 0);
		test_assert((outportal = kportal_open(local, remote, 0)) >= 0);

		if (local == SLAVE_NODENUM)
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				kmemset(message, (char) local, MAILBOX_SIZE);
				test_assert(kmailbox_read(inbox, message, MAILBOX_SIZE) == MAILBOX_SIZE);
				for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
					test_assert(message[l] == remote);

				kmemset(message, (char) local, PORTAL_SIZE);
				test_assert(kportal_allow(inportal, remote, 0) >= 0);
				test_assert(kportal_read(inportal, message, PORTAL_SIZE) == PORTAL_SIZE);
				for (unsigned l = 0; l < PORTAL_SIZE; ++l)
					test_assert(message[l] == remote);

				test_assert(kmailbox_write(outbox, message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

				test_assert(kportal_write(outportal, message_out, PORTAL_SIZE) == PORTAL_SIZE);
			}
		}
		else
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				test_assert(kmailbox_write(outbox, message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

				test_assert(kportal_write(outportal, message_out, PORTAL_SIZE) == PORTAL_SIZE);

				kmemset(message, (char) local, MAILBOX_SIZE);
				test_assert(kmailbox_read(inbox, message, MAILBOX_SIZE) == MAILBOX_SIZE);
				for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
					test_assert(message[l] == remote);

				kmemset(message, (char) local, PORTAL_SIZE);
				test_assert(kportal_allow(inportal, remote, 0) >= 0);
				test_assert(kportal_read(inportal, message, PORTAL_SIZE) == PORTAL_SIZE);
				for (unsigned l = 0; l < PORTAL_SIZE; ++l)
					test_assert(message[l] == remote);
			}
		}

		test_assert(kportal_close(outportal) == 0);
		test_assert(kportal_unlink(inportal) == 0);
		test_assert(kmailbox_close(outbox) == 0);
		test_assert(kmailbox_unlink(inbox) == 0);
	}
}


/*============================================================================*
 * Stress Test: Portal Ping-Pong Reverse                                      *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Ping-Pong Reverse
 */
PRIVATE void test_stress_ikc_pingpong_reverse(void)
{
	int local;
	int remote;
	int inbox;
	int outbox;
	int inportal;
	int outportal;

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	kmemset(message_out, (char) local, PORTAL_SIZE);

	for (int i = 0; i < NSETUPS; ++i)
	{
		test_assert((inbox = kmailbox_create(local, 0)) >= 0);
		test_assert((outbox = kmailbox_open(remote, 0)) >= 0);
		test_assert((inportal = kportal_create(local, 0)) >= 0);
		test_assert((outportal = kportal_open(local, remote, 0)) >= 0);

		if (local == SLAVE_NODENUM)
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				kmemset(message, (char) local, MAILBOX_SIZE);
				test_assert(kmailbox_read(inbox, message, MAILBOX_SIZE) == MAILBOX_SIZE);
				for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
					test_assert(message[l] == remote);

				test_assert(kportal_write(outportal, message_out, PORTAL_SIZE) == PORTAL_SIZE);

				test_assert(kmailbox_write(outbox, message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

				kmemset(message, (char) local, PORTAL_SIZE);
				test_assert(kportal_allow(inportal, remote, 0) >= 0);
				test_assert(kportal_read(inportal, message, PORTAL_SIZE) == PORTAL_SIZE);
				for (unsigned l = 0; l < PORTAL_SIZE; ++l)
					test_assert(message[l] == remote);
			}
		}
		else
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				test_assert(kmailbox_write(outbox, message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

				kmemset(message, (char) local, PORTAL_SIZE);
				test_assert(kportal_allow(inportal, remote, 0) >= 0);
				test_assert(kportal_read(inportal, message, PORTAL_SIZE) == PORTAL_SIZE);
				for (unsigned l = 0; l < PORTAL_SIZE; ++l)
					test_assert(message[l] == remote);

				kmemset(message, (char) local, MAILBOX_SIZE);
				test_assert(kmailbox_read(inbox, message, MAILBOX_SIZE) == MAILBOX_SIZE);
				for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
					test_assert(message[l] == remote);

				test_assert(kportal_write(outportal, message_out, PORTAL_SIZE) == PORTAL_SIZE);
			}
		}

		test_assert(kportal_close(outportal) == 0);
		test_assert(kportal_unlink(inportal) == 0);
		test_assert(kmailbox_close(outbox) == 0);
		test_assert(kmailbox_unlink(inbox) == 0);
	}
}

/*============================================================================*
 * Stress Test: Portal Multiplexing Broadcast                                 *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Multiplexing Broadcast
 */
PRIVATE void test_stress_ikc_multiplexing_broadcast(void)
{
	int local;
	int remote;
	int mbxids[TEST_THREAD_NPORTS];
	int portalids[TEST_THREAD_NPORTS];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == MASTER_NODENUM)
	{
		kmemset(message, (char) local, PORTAL_SIZE);

		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				test_assert((mbxids[j] = kmailbox_open(remote, j)) >= 0);
				test_assert((portalids[j] = kportal_open(local, remote, j)) >= 0);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					test_assert(kmailbox_write(mbxids[k], message, MAILBOX_SIZE) == MAILBOX_SIZE);
					test_assert(kportal_write(portalids[k], message, PORTAL_SIZE) == PORTAL_SIZE);
				}

			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				test_assert(kportal_close(portalids[j]) == 0);
				test_assert(kmailbox_close(mbxids[j]) == 0);
			}
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				test_assert((mbxids[j] = kmailbox_create(local, j)) >= 0);
				test_assert((portalids[j] = kportal_create(local, j)) >= 0);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					kmemset(message, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(mbxids[k], message, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(message[l] == remote);

					kmemset(message, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(portalids[k], remote, k) >= 0);
					test_assert(kportal_read(portalids[k], message, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(message[l] == remote);
				}
			}

			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				test_assert(kportal_unlink(portalids[j]) == 0);
				test_assert(kmailbox_unlink(mbxids[j]) == 0);
			}
		}
	}
}

/*============================================================================*
 * Stress Test: Portal Multiplexing Gather                                    *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Multiplexing Gather
 */
PRIVATE void test_stress_ikc_multiplexing_gather(void)
{
	int local;
	int remote;
	int mbxids[TEST_THREAD_NPORTS];
	int portalids[TEST_THREAD_NPORTS];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		kmemset(message, (char) local, PORTAL_SIZE);

		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				test_assert((mbxids[j] = kmailbox_open(remote, j)) >= 0);
				test_assert((portalids[j] = kportal_open(local, remote, j)) >= 0);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					test_assert(kmailbox_write(mbxids[k], message, MAILBOX_SIZE) == MAILBOX_SIZE);
					test_assert(kportal_write(portalids[k], message, PORTAL_SIZE) == PORTAL_SIZE);
				}

			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				test_assert(kportal_close(portalids[j]) == 0);
				test_assert(kmailbox_close(mbxids[j]) == 0);
			}
		}
	}
	else
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				test_assert((mbxids[j] = kmailbox_create(local, j)) >= 0);
				test_assert((portalids[j] = kportal_create(local, j)) >= 0);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					kmemset(message, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(mbxids[k], message, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(message[l] == remote);

					kmemset(message, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(portalids[k], remote, k) >= 0);
					test_assert(kportal_read(portalids[k], message, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(message[l] == remote);
				}
			}

			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				test_assert(kportal_unlink(portalids[j]) == 0);
				test_assert(kmailbox_unlink(mbxids[j]) == 0);
			}
		}
	}
}

/*============================================================================*
 * Stress Test: Portal Multiplexing Ping-Pong                                 *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Multiplexing Ping-Pong
 */
PRIVATE void test_stress_ikc_multiplexing_pingpong(void)
{
	int local;
	int remote;
	int inboxes[TEST_THREAD_NPORTS];
	int outboxes[TEST_THREAD_NPORTS];
	int inportals[TEST_THREAD_NPORTS];
	int outportals[TEST_THREAD_NPORTS];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	kmemset(message_out, (char) local, PORTAL_SIZE);

	for (int i = 0; i < NSETUPS; ++i)
	{
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			test_assert((inboxes[j] = kmailbox_create(local, j)) >= 0);
			test_assert((outboxes[j] = kmailbox_open(remote, j)) >= 0);
			test_assert((inportals[j] = kportal_create(local, j)) >= 0);
			test_assert((outportals[j] = kportal_open(local, remote, j)) >= 0);
		}

		if (local == SLAVE_NODENUM)
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					kmemset(message, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], message, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(message[l] == remote);

					kmemset(message, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, k) >= 0);
					test_assert(kportal_read(inportals[k], message, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(message[l] == remote);

					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
				}
			}
		}
		else
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);

					kmemset(message, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], message, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(message[l] == remote);

					kmemset(message, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, k) >= 0);
					test_assert(kportal_read(inportals[k], message, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(message[l] == remote);
				}
			}
		}

		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			test_assert(kportal_close(outportals[j]) == 0);
			test_assert(kportal_unlink(inportals[j]) == 0);
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Portal Multiplexing Ping-Pong Reverse                         *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Multiplexing Ping-Pong Reverse
 */
PRIVATE void test_stress_ikc_multiplexing_pingpong_reverse(void)
{
	int local;
	int remote;
	int inboxes[TEST_THREAD_NPORTS];
	int outboxes[TEST_THREAD_NPORTS];
	int inportals[TEST_THREAD_NPORTS];
	int outportals[TEST_THREAD_NPORTS];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	kmemset(message_out, (char) local, PORTAL_SIZE);

	for (int i = 0; i < NSETUPS; ++i)
	{
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			test_assert((inboxes[j] = kmailbox_create(local, j)) >= 0);
			test_assert((outboxes[j] = kmailbox_open(remote, j)) >= 0);
			test_assert((inportals[j] = kportal_create(local, j)) >= 0);
			test_assert((outportals[j] = kportal_open(local, remote, j)) >= 0);
		}

		if (local == SLAVE_NODENUM)
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					kmemset(message, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], message, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(message[l] == remote);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);

					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					kmemset(message, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, k) >= 0);
					test_assert(kportal_read(inportals[k], message, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(message[l] == remote);
				}
			}
		}
		else
		{
			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < TEST_THREAD_NPORTS; ++k)
				{
					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					kmemset(message, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, k) >= 0);
					test_assert(kportal_read(inportals[k], message, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(message[l] == remote);

					kmemset(message, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], message, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(message[l] == remote);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
				}
			}
		}

		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			test_assert(kportal_close(outportals[j]) == 0);
			test_assert(kportal_unlink(inportals[j]) == 0);
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}
	}
}

/*============================================================================*
 * Stress Test: Thread synchronization                                        *
 *============================================================================*/

/*============================================================================*
 * Stress Test: Portal Thread Multiplexing Broadcast                          *
 *============================================================================*/

PRIVATE void * do_thread_multiplexing_broadcast(void * arg)
{
	int tid;
	int local;
	int remote;
	int nports;
	int mbxids[TEST_THREAD_IKCID_NUM];
	int portalids[TEST_THREAD_IKCID_NUM];
	char * msg;

	tid = ((int)(intptr_t) arg);
	msg = message_in[tid];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == MASTER_NODENUM)
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				if (j == (tid + nports * TEST_THREAD_MAX))
				{
					test_assert((mbxids[nports] = kmailbox_open(remote, j)) >= 0);
					test_assert((portalids[nports] = kportal_open(local, remote, j)) >= 0);
					nports++;
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					test_assert(kmailbox_write(mbxids[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);
					test_assert(kportal_write(portalids[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
			{
				test_assert(kportal_close(portalids[j]) == 0);
				test_assert(kmailbox_close(mbxids[j]) == 0);
			}

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
				{
					test_assert((mbxids[nports] = kmailbox_create(local, j)) >= 0);
					test_assert((portalids[nports] = kportal_create(local, j)) >= 0);
					nports++;
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(mbxids[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(portalids[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(portalids[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
			{
				test_assert(kportal_unlink(portalids[j]) == 0);
				test_assert(kmailbox_unlink(mbxids[j]) == 0);
			}

			nanvix_fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Portal Thread Multiplexing Broadcast
 */
PRIVATE void test_stress_ikc_thread_multiplexing_broadcast(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	kmemset(message_out, (char) knode_get_num(), PORTAL_SIZE);
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
 * Stress Test: Portal Thread Multiplexing Gather                             *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Thread Multiplexing Gather
 */
PRIVATE void * do_thread_multiplexing_gather(void * arg)
{
	int tid;
	int nports;
	int local;
	int remote;
	int mbxids[TEST_THREAD_IKCID_NUM];
	int portalids[TEST_THREAD_IKCID_NUM];
	char * msg;

	tid = ((int)(intptr_t) arg);
	msg = message_in[tid];

	local = knode_get_num();
	remote = (local == MASTER_NODENUM) ? SLAVE_NODENUM : MASTER_NODENUM;

	if (local == SLAVE_NODENUM)
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
			{
				if (j == (tid + nports * TEST_THREAD_MAX))
				{
					test_assert((mbxids[nports] = kmailbox_open(remote, j)) >= 0);
					test_assert((portalids[nports] = kportal_open(local, remote, j)) >= 0);
					nports++;
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					test_assert(kmailbox_write(mbxids[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);
					test_assert(kportal_write(portalids[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
			{
				test_assert(kportal_close(portalids[j]) == 0);
				test_assert(kmailbox_close(mbxids[j]) == 0);
			}

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
				{
					test_assert((mbxids[nports] = kmailbox_create(local, j)) >= 0);
					test_assert((portalids[nports] = kportal_create(local, j)) >= 0);
					nports++;
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(mbxids[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(portalids[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(portalids[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
			{
				test_assert(kmailbox_unlink(mbxids[j]) == 0);
				test_assert(kportal_unlink(portalids[j]) == 0);
			}

			nanvix_fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Portal Thread Multiplexing Gather
 */
PRIVATE void test_stress_ikc_thread_multiplexing_gather(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	kmemset(message_out, (char) knode_get_num(), PORTAL_SIZE);
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
 * Stress Test: Portal Thread Multiplexing Ping-Pong                          *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Thread Multiplexing Ping-Pong
 */
PRIVATE void * do_thread_multiplexing_pingpong(void * arg)
{
	int tid;
	int local;
	int remote;
	int nports;
	int inboxes[TEST_THREAD_IKCID_NUM];
	int outboxes[TEST_THREAD_IKCID_NUM];
	int inportals[TEST_THREAD_IKCID_NUM];
	int outportals[TEST_THREAD_IKCID_NUM];
	char * msg;

	tid = ((int)(intptr_t) arg);
	msg = message_in[tid];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	for (int i = 0; i < NSETUPS; ++i)
	{
		nports = 0;
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			if (j == (tid + nports * TEST_THREAD_MAX))
			{
				test_assert((inboxes[nports] = kmailbox_create(local, j)) >= 0);
				test_assert((outboxes[nports] = kmailbox_open(remote, j)) >= 0);
				test_assert((inportals[nports] = kportal_create(local, j)) >= 0);
				test_assert((outportals[nports] = kportal_open(local, remote, j)) >= 0);
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
					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(inportals[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);

					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
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
					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);

					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(inportals[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);
				}

				nanvix_fence(&_fence);
			}
		}

		for (int j = 0; j < nports; ++j)
		{
			test_assert(kportal_close(outportals[j]) == 0);
			test_assert(kportal_unlink(inportals[j]) == 0);
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}

		nanvix_fence(&_fence);
	}

	return (NULL);
}

/**
 * @brief Stress Test: Portal Thread Multiplexing Ping-Pong
 */
PRIVATE void test_stress_ikc_thread_multiplexing_pingpong(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	kmemset(message_out, (char) knode_get_num(), PORTAL_SIZE);
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
 * Stress Test: IKC Thread Multiplexing Affinity                              *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Thread Multiplexing Affinity
 */
PRIVATE void * do_thread_multiplexing_affinity(void * arg)
{
	int tid;
	int local;
	int remote;
	int nports;
	int inboxes[TEST_THREAD_IKCID_NUM];
	int outboxes[TEST_THREAD_IKCID_NUM];
	int inportals[TEST_THREAD_IKCID_NUM];
	int outportals[TEST_THREAD_IKCID_NUM];
	char * msg;

	/* Change affinity. */
	test_assert(kthread_set_affinity(TEST_THREAD_AFFINITY) > 0);
	test_assert(core_get_id() == TEST_CORE_AFFINITY);

	tid = ((int)(intptr_t) arg);
	msg = message_in[tid];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	for (int i = 0; i < NSETUPS_AFFINITY; ++i)
	{
		nports = 0;
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			if (j == (tid + nports * TEST_THREAD_MAX))
			{
				test_assert((inboxes[nports] = kmailbox_create(local, j)) >= 0);
				test_assert((outboxes[nports] = kmailbox_open(remote, j)) >= 0);
				test_assert((inportals[nports] = kportal_create(local, j)) >= 0);
				test_assert((outportals[nports] = kportal_open(local, remote, j)) >= 0);
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
					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(inportals[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);

					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
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
					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);

					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(inportals[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);
				}

				nanvix_fence(&_fence);
			}
		}

		for (int j = 0; j < nports; ++j)
		{
			test_assert(kportal_close(outportals[j]) == 0);
			test_assert(kportal_unlink(inportals[j]) == 0);
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}

		nanvix_fence(&_fence);
	}

	return (NULL);
}

/**
 * @brief Stress Test: IKC Thread Multiplexing Affinity
 */
PRIVATE void test_stress_ikc_thread_multiplexing_affinity(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	kmemset(message_out, (char) knode_get_num(), PORTAL_SIZE);
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
 * Stress Test: Portal Thread Multiplexing Ping-Pong Reverse                  *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Thread Multiplexing Ping-Pong Reverse
 */
PRIVATE void * do_thread_multiplexing_pingpong_reverse(void * arg)
{
	int tid;
	int local;
	int remote;
	int nports;
	int inboxes[TEST_THREAD_IKCID_NUM];
	int outboxes[TEST_THREAD_IKCID_NUM];
	int inportals[TEST_THREAD_IKCID_NUM];
	int outportals[TEST_THREAD_IKCID_NUM];
	char * msg;

	tid = ((int)(intptr_t) arg);
	msg = message_in[tid];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	for (int i = 0; i < NSETUPS; ++i)
	{
		nports = 0;
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			if (j == (tid + nports * TEST_THREAD_MAX))
			{
				test_assert((inboxes[nports] = kmailbox_create(local, j)) >= 0);
				test_assert((outboxes[nports] = kmailbox_open(remote, j)) >= 0);
				test_assert((inportals[nports] = kportal_create(local, j)) >= 0);
				test_assert((outportals[nports] = kportal_open(local, remote, j)) >= 0);
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
					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);

					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(inportals[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);
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
					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(inportals[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);

					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
				}

				nanvix_fence(&_fence);
			}
		}

		for (int j = 0; j < nports; ++j)
		{
			test_assert(kportal_close(outportals[j]) == 0);
			test_assert(kportal_unlink(inportals[j]) == 0);
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}

		nanvix_fence(&_fence);
	}

	return (NULL);
}

/**
 * @brief Stress Test: Portal Thread Multiplexing Ping-Pong
 */
PRIVATE void test_stress_ikc_thread_multiplexing_pingpong_reverse(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	kmemset(message_out, (char) knode_get_num(), PORTAL_SIZE);
	nanvix_fence_init(&_fence, TEST_THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_pingpong_reverse, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_pingpong_reverse(0);

	/* Join threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);
}

/*============================================================================*
 * Stress Test: IKC Thread Multiplexing Affinity Reverse                      *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Thread Multiplexing Affinity Reverse
 */
PRIVATE void * do_thread_multiplexing_affinity_reverse(void * arg)
{
	int tid;
	int local;
	int remote;
	int nports;
	int inboxes[TEST_THREAD_IKCID_NUM];
	int outboxes[TEST_THREAD_IKCID_NUM];
	int inportals[TEST_THREAD_IKCID_NUM];
	int outportals[TEST_THREAD_IKCID_NUM];
	char * msg;

	/* Change affinity. */
	test_assert(kthread_set_affinity(TEST_THREAD_AFFINITY) > 0);
	test_assert(core_get_id() == TEST_CORE_AFFINITY);

	tid = ((int)(intptr_t) arg);
	msg = message_in[tid];

	local = knode_get_num();
	remote = local == MASTER_NODENUM ? SLAVE_NODENUM : MASTER_NODENUM;

	for (int i = 0; i < NSETUPS_AFFINITY; ++i)
	{
		nports = 0;
		for (int j = 0; j < TEST_THREAD_NPORTS; ++j)
		{
			if (j == (tid + nports * TEST_THREAD_MAX))
			{
				test_assert((inboxes[nports] = kmailbox_create(local, j)) >= 0);
				test_assert((outboxes[nports] = kmailbox_open(remote, j)) >= 0);
				test_assert((inportals[nports] = kportal_create(local, j)) >= 0);
				test_assert((outportals[nports] = kportal_open(local, remote, j)) >= 0);
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
					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);

					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(inportals[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);
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
					test_assert(kmailbox_write(outboxes[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);

					kmemset(msg, (char) local, PORTAL_SIZE);
					test_assert(kportal_allow(inportals[k], remote, (tid + k * TEST_THREAD_MAX)) >= 0);
					test_assert(kportal_read(inportals[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == remote);

					kmemset(msg, (char) local, MAILBOX_SIZE);
					test_assert(kmailbox_read(inboxes[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == remote);

					test_assert(kportal_write(outportals[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
				}

				nanvix_fence(&_fence);
			}
		}

		for (int j = 0; j < nports; ++j)
		{
			test_assert(kportal_close(outportals[j]) == 0);
			test_assert(kportal_unlink(inportals[j]) == 0);
			test_assert(kmailbox_close(outboxes[j]) == 0);
			test_assert(kmailbox_unlink(inboxes[j]) == 0);
		}

		nanvix_fence(&_fence);
	}

	return (NULL);
}

/**
 * @brief Stress Test: IKC Thread Multiplexing Affinity Reverse
 */
PRIVATE void test_stress_ikc_thread_multiplexing_affinity_reverse(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	kmemset(message_out, (char) knode_get_num(), PORTAL_SIZE);
	nanvix_fence_init(&_fence, TEST_THREAD_MAX);

	/* Create threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_create(&tid[i - 1], do_thread_multiplexing_affinity_reverse, ((void *)(intptr_t) i)) == 0);

	do_thread_multiplexing_affinity_reverse(0);

	/* Join threads. */
	for (int i = 1; i < TEST_THREAD_MAX; ++i)
		test_assert(kthread_join(tid[i - 1], NULL) == 0);

	test_assert(kthread_set_affinity(KTHREAD_AFFINITY_DEFAULT) == TEST_THREAD_AFFINITY);
}

/*============================================================================*
 * Stress Test: Portal Thread Multiplexing Broadcast Local                    *
 *============================================================================*/

PRIVATE void * do_thread_multiplexing_broadcast_local(void * arg)
{
	int tid;
	int local;
	int nports;
	int mbxids[TEST_THREAD_NPORTS];
	int portalids[TEST_THREAD_NPORTS];
	char * msg;

	tid = ((int)(intptr_t) arg);
	msg = message_in[tid];

	local = knode_get_num();

	if (tid == 0)
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < (TEST_THREAD_NPORTS - 1); ++j)
			{
				test_assert((mbxids[nports] = kmailbox_open(local, j)) >= 0);
				test_assert((portalids[nports] = kportal_open(local, local, j)) >= 0);
				nports++;

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					test_assert(kmailbox_write(mbxids[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);
					test_assert(kportal_write(portalids[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
			{
				test_assert(kportal_close(portalids[j]) == 0);
				test_assert(kmailbox_close(mbxids[j]) == 0);
			}

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
				{
					test_assert((mbxids[nports] = kmailbox_create(local, j)) >= 0);
					test_assert((portalids[nports] = kportal_create(local, j)) >= 0);
					nports++;
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					kmemset(msg, 0, MAILBOX_SIZE);
					test_assert(kmailbox_read(mbxids[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == 1);

					kmemset(msg, 0, PORTAL_SIZE);
					test_assert(kportal_allow(portalids[k], local, ((tid - 1) + k * (TEST_THREAD_MAX - 1))) >= 0);
					test_assert(kportal_read(portalids[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == 1);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
			{
				test_assert(kportal_unlink(portalids[j]) == 0);
				test_assert(kmailbox_unlink(mbxids[j]) == 0);
			}

			nanvix_fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Portal Thread Multiplexing Broadcast Local
 */
PRIVATE void test_stress_ikc_thread_multiplexing_broadcast_local(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	kmemset(message_out, 1, PORTAL_SIZE);
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
 * Stress Test: Portal Thread Multiplexing Gather Local                       *
 *============================================================================*/

/**
 * @brief Stress Test: Portal Thread Multiplexing Gather Local
 */
PRIVATE void * do_thread_multiplexing_gather_local(void * arg)
{
	int tid;
	int local;
	int nports;
	int mbxids[TEST_THREAD_NPORTS];
	int portalids[TEST_THREAD_NPORTS];
	char * msg;

	tid = ((int)(intptr_t) arg);
	msg = message_in[tid];

	local = knode_get_num();

	if (tid != 0)
	{
		for (int i = 0; i < NSETUPS; ++i)
		{
			nports = 0;
			for (int j = 0; j < (TEST_THREAD_NPORTS - 1); ++j)
			{
				if (j == ((tid - 1) + nports * (TEST_THREAD_MAX - 1)))
				{
					test_assert((mbxids[nports] = kmailbox_open(local, j)) >= 0);
					test_assert((portalids[nports] = kportal_open(local, local, j)) >= 0);
					nports++;
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					test_assert(kmailbox_write(mbxids[k], message_out, MAILBOX_SIZE) == MAILBOX_SIZE);
					test_assert(kportal_write(portalids[k], message_out, PORTAL_SIZE) == PORTAL_SIZE);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
			{
				test_assert(kportal_close(portalids[j]) == 0);
				test_assert(kmailbox_close(mbxids[j]) == 0);
			}

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
				test_assert((mbxids[nports] = kmailbox_create(local, j)) >= 0);
				test_assert((portalids[nports] = kportal_create(local, j)) >= 0);
				nports++;

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < NCOMMUNICATIONS; ++j)
			{
				for (int k = 0; k < nports; ++k)
				{
					kmemset(msg, 0, MAILBOX_SIZE);
					test_assert(kmailbox_read(mbxids[k], msg, MAILBOX_SIZE) == MAILBOX_SIZE);
					for (unsigned l = 0; l < MAILBOX_SIZE; ++l)
						test_assert(msg[l] == 1);

					kmemset(msg, 0, PORTAL_SIZE);
					test_assert(kportal_allow(portalids[k], local, k) >= 0);
					test_assert(kportal_read(portalids[k], msg, PORTAL_SIZE) == PORTAL_SIZE);
					for (unsigned l = 0; l < PORTAL_SIZE; ++l)
						test_assert(msg[l] == 1);
				}

				nanvix_fence(&_fence);
			}

			for (int j = 0; j < nports; ++j)
			{
				test_assert(kmailbox_unlink(mbxids[j]) == 0);
				test_assert(kportal_unlink(portalids[j]) == 0);
			}

			nanvix_fence(&_fence);
		}
	}

	return (NULL);
}

/**
 * @brief Stress Test: Portal Thread Multiplexing Gather Local
 */
PRIVATE void test_stress_ikc_thread_multiplexing_gather_local(void)
{
	kthread_t tid[TEST_THREAD_MAX - 1];

	kmemset(message_out, 1, PORTAL_SIZE);
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
 * @brief Stress tests.
 */
PRIVATE struct test ikc_tests_stress[] = {
	/* Intra-Cluster API Tests */
	{ test_stress_ikc_create_unlink,                        "[test][ikc][stress] IKC create unlink                         [passed]" },
	{ test_stress_ikc_open_close,                           "[test][ikc][stress] IKC open close                            [passed]" },
	{ test_stress_ikc_broadcast,                            "[test][ikc][stress] IKC broadcast                             [passed]" },
	{ test_stress_ikc_gather,                               "[test][ikc][stress] IKC gather                                [passed]" },
	{ test_stress_ikc_pingpong,                             "[test][ikc][stress] IKC ping-pong                             [passed]" },
	{ test_stress_ikc_pingpong_reverse,                     "[test][ikc][stress] IKC ping-pong reverse                     [passed]" },
	{ test_stress_ikc_multiplexing_broadcast,               "[test][ikc][stress] IKC multiplexing broadcast                [passed]" },
	{ test_stress_ikc_multiplexing_gather,                  "[test][ikc][stress] IKC multiplexing gather                   [passed]" },
	{ test_stress_ikc_multiplexing_pingpong,                "[test][ikc][stress] IKC multiplexing ping-pong                [passed]" },
	{ test_stress_ikc_multiplexing_pingpong_reverse,        "[test][ikc][stress] IKC multiplexing ping-pong reverse        [passed]" },
	{ test_stress_ikc_thread_multiplexing_broadcast,        "[test][ikc][stress] IKC thread multiplexing broadcast         [passed]" },
	{ test_stress_ikc_thread_multiplexing_gather,           "[test][ikc][stress] IKC thread multiplexing gather            [passed]" },
	{ test_stress_ikc_thread_multiplexing_pingpong,         "[test][ikc][stress] IKC thread multiplexing ping-pong         [passed]" },
	{ test_stress_ikc_thread_multiplexing_pingpong_reverse, "[test][ikc][stress] IKC thread multiplexing ping-pong reverse [passed]" },
#if (CORE_SUPPORTS_MULTITHREADING && __NANVIX_MICROKERNEL_DYNAMIC_SCHED)
	{ test_stress_ikc_thread_multiplexing_affinity,         "[test][ikc][stress] IKC thread multiplexing affinity          [passed]" },
	{ test_stress_ikc_thread_multiplexing_affinity_reverse, "[test][ikc][stress] IKC thread multiplexing affinity reverse  [passed]" },
#endif
	{ test_stress_ikc_thread_multiplexing_broadcast_local,  "[test][ikc][stress] IKC thread multiplexing broadcast local   [passed]" },
	{ test_stress_ikc_thread_multiplexing_gather_local,     "[test][ikc][stress] IKC thread multiplexing gather local      [passed]" },
	{ NULL,                                                  NULL                                                                    },
};

/**
 * The test_thread_mgmt() function launches testing units on thread manager.
 *
 * @author Pedro Henrique Penna
 */
void test_ikc(void)
{
	int local;

	local = knode_get_num();

	if (local == MASTER_NODENUM || local == SLAVE_NODENUM)
	{
		/* Stress Tests */
		if (local == MASTER_NODENUM)
			nanvix_puts("--------------------------------------------------------------------------------");
		for (unsigned i = 0; ikc_tests_stress[i].test_fn != NULL; i++)
		{
			ikc_tests_stress[i].test_fn();

			test_barrier_nodes();

			if (local == MASTER_NODENUM)
				nanvix_puts(ikc_tests_stress[i].name);
		}
	}
}

#endif /* __TARGET_HAS_MAILBOX && __TARGET_HAS_PORTAL */
