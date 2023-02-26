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

#define CIRCULAR_SLAVE_FIRST_SENDER (SLAVE_NODENUM)
#define CIRCULAR_SLAVE_LAST_SENDER (SLAVE_NODENUM + 15)
#define CIRCULAR_SLAVE_LAST_RECEIVER (SLAVE_NODENUM)

#define THREADS_SLAVE_SENDER (SLAVE_NODENUM)
#define THREADS_SLAVE_RECEIVER (SLAVE_NODENUM + 1)

#if TEST_CIRCULAR_MIGRATION
	#define SLAVE_MAILBOX_CONTROLLER CIRCULAR_SLAVE_LAST_RECEIVER
	#define NEED_SETUP_TIME_SYNC    0
#elif TEST_MULTIPLE_THREADS
	#define SLAVE_MAILBOX_CONTROLLER THREADS_SLAVE_RECEIVER
	#define NEED_SETUP_TIME_SYNC    1
#elif TEST_PARALLEL_MIGRATION
	#define SLAVE_MAILBOX_CONTROLLER (SLAVE_NODENUM + 1)
	#define NEED_SETUP_TIME_SYNC    1
#endif

int nodes[NR_NODES];
int * cnodes = &nodes[1];
int done;
unsigned int global = 0;
int out __attribute__((section(".libnanvix.data")));
int syncin __attribute__((section(".libnanvix.data")));
int syncout __attribute__((section(".libnanvix.data")));

PRIVATE inline void slave_wait()
{
	while(1);
}

PRIVATE void *thread_work(void *arg)
{
	UNUSED(arg);

	int *i = &done;
	while(!(*i))
	{
		dcache_invalidate();
	};

	return NULL;
}

PRIVATE void master_fn()
{
	uint64_t t1, setup;

	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);
	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);

	test_assert(ksync_signal(syncout) == 0);
	test_assert(ksync_wait(syncin) == 0);

	perf_start(0, PERF_CYCLES);

	#if NEED_SETUP_TIME_SYNC
		test_assert(ksync_wait(syncin) == 0);
		test_assert(ksync_signal(syncout) == 0);
		setup = perf_read(0);
	#endif

	test_assert(ksync_signal(syncout) == 0);
	test_assert(ksync_wait(syncin) == 0);

	perf_stop(0);
	#if NEED_SETUP_TIME_SYNC
		t1 = perf_read(0) - setup;
	#else
		t1 = perf_read(0);
	#endif

	kprintf("time: %l", t1);
}

PRIVATE inline int parallel_node_index(void)
{
	return knode_get_num() - SLAVE_NODENUM;
}

PRIVATE inline int parallel_receveiver_node_index()
{
	return ((NR_NODES - 1) / 2) + parallel_node_index();
}

PRIVATE int parallel_is_sender()
{
	if (parallel_node_index() < (NR_NODES - 1) / 2)
		return (1);
	return (0);

}

PRIVATE void slave_fn_parallel()
{
	kthread_t tids[TESTS_NTHREADS];
	unsigned * pg;
	const unsigned magic = 0xdeadbeef;

	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);

	done = 0;
	if (parallel_is_sender())
	{
		for (int i = 0; i < TESTS_NTHREADS; i++)
			KASSERT(kthread_create(&tids[i], thread_work, NULL) == 0);

		for (int i = 0; i < NPAGES; i++)
		{
			pg = (unsigned  *)(UBASE_VIRT) + i * PAGE_SIZE;

			KASSERT(page_alloc(VADDR(pg)) == 0);

			/* Write. */
			for (size_t j = 0; j < PAGE_SIZE/sizeof(unsigned); j++)
				pg[j] = magic;

		}
		test_assert(ksync_signal(syncout) == 0);
		test_assert(ksync_wait(syncin) == 0);
		kmigrate_to(cnodes[parallel_receveiver_node_index()]);
	}
	else
	{
		test_assert(ksync_signal(syncout) == 0);
		test_assert(ksync_wait(syncin) == 0);
		slave_wait();
	}

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);
	done = 1;
	for (int i = 0; i < TESTS_NTHREADS; i++)
		KASSERT(kthread_join(tids[i], NULL) == 0);
}

PRIVATE void slave_fn_circular()
{
	char dummy = 'd';
	int local = knode_get_num();

	unsigned *pg;
	const unsigned magic = 0xdeadbeef;

	if (local == CIRCULAR_SLAVE_FIRST_SENDER)
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

	if (local != CIRCULAR_SLAVE_FIRST_SENDER)
		slave_wait();

	for (int remote = SLAVE_NODENUM + 1; remote <= CIRCULAR_SLAVE_LAST_SENDER; remote++)
	{
		if (knode_get_num() == remote - 1)
		{
			#if DEBUG
			kprintf("migrating to %d", remote);
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

	if (knode_get_num() == CIRCULAR_SLAVE_LAST_SENDER)
		kmigrate_to(CIRCULAR_SLAVE_LAST_RECEIVER);


	if (knode_get_num() == SLAVE_MAILBOX_CONTROLLER)
		test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));
}

PRIVATE void slave_fn_threads()
{
	unsigned * pg;
	kthread_t tids[TESTS_NTHREADS];
	const unsigned magic = 0xdeadbeef;

	test_assert((syncin = ksync_create(nodes, NR_NODES, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout = ksync_open(nodes, NR_NODES, SYNC_ALL_TO_ONE)) >= 0);

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);

	done = 0;

	if (knode_get_num() == THREADS_SLAVE_RECEIVER)
	{
		test_assert(ksync_signal(syncout) == 0);
		test_assert(ksync_wait(syncin) == 0);
		slave_wait();
	}

	if (knode_get_num() == THREADS_SLAVE_SENDER)
	{
		/* create threads */
		for (int i = 0; i < TESTS_NTHREADS; i++)
			KASSERT(kthread_create(&tids[i], thread_work, NULL) == 0);

		for (int i = 0; i < NPAGES; i++)
		{
			pg = (unsigned  *)(UBASE_VIRT) + i * PAGE_SIZE;

			KASSERT(page_alloc(VADDR(pg)) == 0);

			/* Write. */
			for (size_t j = 0; j < PAGE_SIZE/sizeof(unsigned); j++)
				pg[j] = magic;

		}
		test_assert(ksync_signal(syncout) == 0);
		test_assert(ksync_wait(syncin) == 0);
		kmigrate_to(THREADS_SLAVE_RECEIVER);
	}

	done = 1;

	#if DEBUG

	for (int i = 0; i < NPAGES; i++)
	{
		pg = (unsigned  *)(UBASE_VIRT) + i * PAGE_SIZE;

		for (size_t j = 0; j < PAGE_SIZE/sizeof(unsigned); j++)
			KASSERT(pg[j] == magic);
	}

	#endif

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);

	for (int i = 0; i < TESTS_NTHREADS; i++)
		kthread_join(tids[i], NULL);
}

PRIVATE void test_api_migration()
{
	void (*fn)(void);

	nodes[0] = MASTER_NODENUM;
	for (int i = 1; i < NR_NODES; i++)
		nodes[i] = SLAVE_NODENUM + i - 1;

	#if TEST_CIRCULAR_MIGRATION
	fn = knode_get_num() == MASTER_NODENUM ? master_fn : slave_fn_circular;
	#elif TEST_MULTIPLE_THREADS
	fn = knode_get_num() == MASTER_NODENUM ? master_fn : slave_fn_threads;
	#elif TEST_PARALLEL_MIGRATION
	fn = knode_get_num() == MASTER_NODENUM ? master_fn : slave_fn_parallel;
	#endif

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