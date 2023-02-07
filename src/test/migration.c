#include <nanvix/sys/task.h>
#include <nanvix/sys/semaphore.h>
#include <nanvix/sys/portal.h>
#include <nanvix/sys/mailbox.h>
#include <nanvix/kernel/mm.h>
#include <nanvix/kernel/uarea.h>
#include <nanvix/sys/sync.h>
#include <nanvix/sys/page.h>
#include <nanvix/sys/migration.h>
#include <nanvix/sys/noc.h>

#include "test.h"


#define DEBUG 0

#define PORT 0
#define PORT2 10


int nodes[NR_NODES];

#define SLAVE_FIRST_SENDER (SLAVE_NODENUM)
#define SLAVE_LAST_SENDER (SLAVE_NODENUM + 15)
#define SLAVE_LAST_RECEIVER (SLAVE_NODENUM)
#define SLAVE_MAILBOX_CONTROLLER SLAVE_LAST_RECEIVER

int done = 0;
unsigned int global = 0;
int out __attribute__((section(".libnanvix.data")));

PRIVATE inline void slave_wait()
{
	while(1);
}

PRIVATE void slave_fn()
{
	int syncin, syncout;
	char dummy = 'd';
	int local = knode_get_num();

	unsigned *pg;
	const unsigned magic = 0xdeadbeef;

	if (local == SLAVE_FIRST_SENDER)
	{
		pg = (unsigned  *)(UBASE_VIRT);

		/* Allocate.*/
		KASSERT(page_alloc(VADDR(pg)) == 0);

		/* Write. */
		for (size_t i = 0; i < PAGE_SIZE/sizeof(unsigned); i++)
			pg[i] = magic;

		global = 0xc0ffee;
	}

	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

	if (local == SLAVE_MAILBOX_CONTROLLER)
		test_assert((out = kmailbox_open(MASTER_NODENUM, PORT2)) >= 0);

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);
	
	if (local == SLAVE_MAILBOX_CONTROLLER)
	{
		test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));
		test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));
	}

	if (local != SLAVE_FIRST_SENDER)
		slave_wait();

	for (int remote = SLAVE_NODENUM + 1; remote <= SLAVE_LAST_SENDER; remote++)
	{
		if (knode_get_num() == remote - 1)
		{
			kprintf("migrating to %d", remote);
			#if DEBUG
			#endif

			kmigrate_to(remote);

			#if DEBUG
				for (size_t i = 0; i < PAGE_SIZE/sizeof(unsigned); i++)
				{
					KASSERT(pg[i] == magic);
				}
				kprintf("global = %x", global);
			#endif
		}
	}

	if (knode_get_num() == SLAVE_MAILBOX_CONTROLLER)
		/* wait until last sender revive this cluster */
		slave_wait();

	if (knode_get_num() == SLAVE_LAST_SENDER)
		kmigrate_to(SLAVE_LAST_RECEIVER);


	if (knode_get_num() == SLAVE_MAILBOX_CONTROLLER)
		test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));
}

PRIVATE void master_fn()
{
	int in, syncin, syncout;
	char dummy;
	uint64_t t1, error;
	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

	test_assert((in = kmailbox_create(MASTER_NODENUM, PORT2)) >= 0);

	test_assert(ksync_signal(syncout) == 0);
	test_assert(ksync_wait(syncin) == 0);

	test_assert(kmailbox_read(in, &dummy, sizeof(char)) == sizeof(char));
	perf_start(0, PERF_CYCLES);

	test_assert(kmailbox_read(in, &dummy, sizeof(char)) == sizeof(char));
	t1 = perf_read(0);

	error = t1;

	test_assert(kmailbox_read(in, &dummy, sizeof(char)) == sizeof(char));
	perf_stop(0);
	t1 = perf_read(0);

	kprintf("time: %l", t1 - error);
}



PRIVATE void test_api_migration()
{
	void (*fn)(void);

	nodes[0] = MASTER_NODENUM;
	for (int i = 1; i < NR_NODES; i++)
		nodes[i] = SLAVE_NODENUM + i - 1;

	fn = knode_get_num() == MASTER_NODENUM ? master_fn : slave_fn;
	fn();
}

PRIVATE struct test migration_tests_api[] = {
	{ test_api_migration,             "[test][migration][api] migration          [passed]" },
	{ NULL,                            NULL                                             },
};


void test_migration(void)
{
	/* API Tests */
	nanvix_puts("--------------------------------------------------------------------------------");
	for (int i = 0; migration_tests_api[i].test_fn != NULL; i++)
	{
		migration_tests_api[i].test_fn();
		nanvix_puts(migration_tests_api[i].name);
	}
}