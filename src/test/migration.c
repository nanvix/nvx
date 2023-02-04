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


#define PORT 0
#define PORT2 10

unsigned int global = 0;

PRIVATE void sender2()
{
	int remote, out;
	int syncin, syncout;
	int nodes[] = {MASTER_NODENUM, SLAVE_NODENUM, SLAVE_NODENUM + 1};
	char dummy = 'd';

	// local  = SLAVE_NODENUM;
	remote = SLAVE_NODENUM + 1;

	unsigned *pg;
	const unsigned magic = 0xdeadbeef;

	pg = (unsigned  *)(UBASE_VIRT);

	/* Allocate.*/
	KASSERT(page_alloc(VADDR(pg)) == 0);

	/* Write. */
	for (size_t i = 0; i < PAGE_SIZE/sizeof(unsigned); i++)
		pg[i] = magic;


	kprintf("start end page: %d %d %d", 
		__USER_START,
		__USER_END,
		pg
	);
	kprintf("sender -> user length: %d", &__USER_END - &__USER_START);

	global = 0xc0ffee;
	
	test_assert((syncin = ksync_create(nodes, 3, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout = ksync_open(nodes, 3, SYNC_ALL_TO_ONE)) >= 0);

	test_assert((out = kmailbox_open(MASTER_NODENUM, PORT2)) >= 0);

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);
	
	test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));
	test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));

	kprintf("migrate to");

	test_assert(kmigrate_to(remote) == 0);

	// upage_inval(VADDR(pg));
	// dcache_invalidate();
	// tlb_flush();

	// KASSERT(tlb_inval(TLB_INSTRUCTION, VADDR(pg)) == 0);
	// KASSERT(tlb_inval(TLB_DATA, VADDR(pg)) == 0);

	for (size_t i = 0; i < PAGE_SIZE/sizeof(unsigned); i++)
	{
		KASSERT(pg[i] == magic);
	}

	// this line should be executed on both clusters: sender and receiver
	kprintf("global = %x", global);

	if (knode_get_num() == SLAVE_NODENUM)
		test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));
}

PRIVATE void receiver2()
{
	int syncin, syncout;
	// int remote = SLAVE_NODENUM;

	int nodes[] = {MASTER_NODENUM, SLAVE_NODENUM, SLAVE_NODENUM + 1};

	test_assert((syncin = ksync_create(nodes, 3, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout = ksync_open(nodes, 3, SYNC_ALL_TO_ONE)) >= 0);

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);

	kprintf("receiver -> user length: %d", &__USER_END - &__USER_START);

	kprintf("migrate from");

	while(1);
}

PRIVATE void master()
{
	int in, syncin, syncout;
	char dummy;
	uint64_t t1, error;
	int nodes[] = {MASTER_NODENUM, SLAVE_NODENUM, SLAVE_NODENUM + 1};

	test_assert((syncin = ksync_create(nodes, 3, SYNC_ALL_TO_ONE)) >= 0);
	test_assert((syncout = ksync_open(nodes, 3, SYNC_ONE_TO_ALL)) >= 0);

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

	if (knode_get_num() == MASTER_NODENUM)
		master();
	else
	{
		fn = (knode_get_num() == SLAVE_NODENUM) ? sender2 : receiver2;
		fn();
	}
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