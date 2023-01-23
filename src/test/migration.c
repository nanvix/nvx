#include <nanvix/sys/task.h>
#include <nanvix/sys/semaphore.h>
#include <nanvix/sys/portal.h>
#include <nanvix/sys/mailbox.h>
#include <nanvix/kernel/mm.h>
#include <nanvix/kernel/uarea.h>
#include <nanvix/sys/sync.h>
#include <nanvix/sys/page.h>
#include <nanvix/sys/migration.h>

#include "test.h"


#define PORT 0
#define PORT2 10

PRIVATE ktask_t trecv __attribute__ ((section(".libnanvix.data")));
PRIVATE ktask_t tsend __attribute__ ((section(".libnanvix.data")));

PRIVATE int mbx_recv __attribute__ ((section(".libnanvix.data"))); 

PRIVATE struct semaphore sem1;
PRIVATE struct semaphore sem2;
PRIVATE struct semaphore sem3;

unsigned int global = 0;

PRIVATE int task_send(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
) __attribute__ ((section(".libnanvix.text")));	

PRIVATE int task_sleep_core(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
) __attribute__((section(".libnanvix.text")));

PRIVATE int task_receiver(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
) __attribute__ ((section(".libnanvix.text")));

PRIVATE int task_send(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	int local = (int) (arg0);
	int remote = (int) (arg1);
	int out;

	test_assert((out = kportal_open(local, remote, PORT)) >= 0);

#if 0
	ktask_t t2;

	test_assert(ktask_create(&t2, task_sleep_core, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	kprintf("emitting");
	test_assert(ktask_emit0(&t2, core) == 0);

	semaphore_down(&sem1);
#else
	//int busy = 1000;
	//while (--busy);
#endif

	kprintf("[sender] sending user sections");
	test_assert(kportal_write(out, &__USER_START, &__USER_END - &__USER_START) >= 0);

	kprintf("[sender] sending uarea");
	test_assert(kportal_write(out, &uarea, sizeof(struct uarea)) >= 0);

	for (int i = 0; i < THREAD_MAX; ++i)
	{
		if (uarea.threads[i].tid != KTHREAD_NULL_TID)
		{
			kprintf("[sender] sending stacks");
			test_assert(kportal_write(out, uarea.ustacks[i], PAGE_SIZE) >= 0);
			test_assert(kportal_write(out, uarea.kstacks[i], PAGE_SIZE) >= 0);
		}
	}

	kprintf("[sender] sending page directories");
	test_assert(kportal_write(out, root_pgdir, sizeof(struct pde) * PGDIR_LENGTH) >= 0);

	kprintf("[sender] sending page tables");
	test_assert(kportal_write(out, kernel_pgtab, ROOT_PGTAB_NUM * PGTAB_LENGTH * sizeof(struct pte)) >= 0);

	kprintf("[sender] sending kstacks ids");
	test_assert(kportal_write(out, kpages_alias, NUM_KPAGES * sizeof(int)) >= 0);

	kprintf("[sender] sending kstacks");
	// for (int i = 2; i < NUM_KPAGES; i++)
	for (int i = NUM_KPAGES - 1; i > 2; i--)
	{
		if (!kpages_alias[i])
			continue;
		// kprintf("kpage id %d", i);
		// kprintf("kpage address %d", (void *) kpool_id_to_addr(i));
		test_assert(kportal_write(out, (void *) kpool_id_to_addr(i), PAGE_SIZE) >= 0);
	}

	kprintf("[sender] sending frames");
	test_assert(kportal_write(out, (void *) frames_alias, FRAMES_LENGTH * sizeof(bitmap_t)) >= 0);
	
	for (int i = 0; i < FRAMES_LENGTH * 32; i++)
	{
		if (!bitmap_check_bit(frames_alias, i))
			continue;
		
		test_assert(kportal_write(out, (void *) (UBASE_PHYS + (i << PAGE_SHIFT)), PAGE_SIZE) >= 0);
	}

	test_assert(kportal_close(out) == 0);

	global = 0xdeadbeef;

	// semaphore_up(&sem2);
	semaphore_up(&sem3);

	kprintf("[sender] done");

	return (0);
}

PRIVATE int task_sleep_core(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	UNUSED(arg0);
	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	semaphore_up(&sem1);
	semaphore_down(&sem2);

	return (0);
}

PRIVATE void sender()
{
	int local, remote, out;
	int syncin, syncout;
	int nodes[] = {MASTER_NODENUM, SLAVE_NODENUM, SLAVE_NODENUM + 1};
	char dummy = 'd';

	local  = SLAVE_NODENUM;
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

	test_assert(ktask_create(&tsend, task_send, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_dispatch2(&tsend, local, remote) == 0);

	semaphore_down(&sem3);

	upage_inval(VADDR(pg));
	dcache_invalidate();
	tlb_flush();

	// KASSERT(tlb_inval(TLB_INSTRUCTION, VADDR(pg)) == 0);
	// KASSERT(tlb_inval(TLB_DATA, VADDR(pg)) == 0);

	for (size_t i = 0; i < PAGE_SIZE/sizeof(unsigned); i++)
	{
		// kprintf("%d", i);
		// if (i < 10)
		// 	kprintf("pg[i] %d", pg[i]);
		KASSERT(pg[i] == magic);
	}

	// this line should be executed on both clusters: sender and receiver
	kprintf("global = %x", global);

	if (global == 0xc0ffee)
		test_assert(kmailbox_write(mbx_recv, &dummy, sizeof(char)) == sizeof(char));
}

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

	// semaphore_down(&sem3);

	upage_inval(VADDR(pg));
	dcache_invalidate();
	tlb_flush();

	// KASSERT(tlb_inval(TLB_INSTRUCTION, VADDR(pg)) == 0);
	// KASSERT(tlb_inval(TLB_DATA, VADDR(pg)) == 0);

	for (size_t i = 0; i < PAGE_SIZE/sizeof(unsigned); i++)
	{
		// kprintf("%d", i);
		// if (i < 10)
		// 	kprintf("pg[i] %d", pg[i]);
		KASSERT(pg[i] == magic);
	}

	// this line should be executed on both clusters: sender and receiver
	kprintf("global = %x", global);

	if (global == 0xc0ffee)
		test_assert(kmailbox_write(mbx_recv, &dummy, sizeof(char)) == sizeof(char));
}

PRIVATE int task_receiver(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	int local = (int) arg0;
	int remote = (int) arg1;
	int in;

	test_assert((in = kportal_create(local, PORT)) >= 0);
	test_assert((mbx_recv = kmailbox_open(MASTER_NODENUM, PORT2)) >= 0);

	kprintf("[receiver] reading user sections");
	test_assert(kportal_allow(in, remote, PORT) == 0);
	test_assert(kportal_read(in, &__USER_START, &__USER_END - &__USER_START) >= 0);

	kprintf("[receiver] reading uarea");
	test_assert(kportal_allow(in, remote, PORT) == 0);
	test_assert(kportal_read(in, &uarea, sizeof(struct uarea)) >= 0);

	for (int i = 0; i < THREAD_MAX; ++i)
	{
		if (uarea.threads[i].tid != KTHREAD_NULL_TID)
		{
			/* TODO: Maybe must call kpage_get on u/kstasks. */
			kprintf("[receiver] reading stacks");
			test_assert(kportal_allow(in, remote, PORT) == 0);
			test_assert(kportal_read(in, uarea.ustacks[i], PAGE_SIZE) >= 0);
			test_assert(kportal_allow(in, remote, PORT) == 0);
			test_assert(kportal_read(in, uarea.kstacks[i], PAGE_SIZE) >= 0);
		}
	}

	kprintf("[receiver] reading page directories");
	test_assert(kportal_allow(in, remote, PORT) == 0);
	test_assert(kportal_read(in, root_pgdir, sizeof(struct pde) * PGDIR_LENGTH) >= 0);

	kprintf("[receiver] reading page tables");
	test_assert(kportal_allow(in, remote, PORT) == 0);
	test_assert(kportal_read(in, kernel_pgtab, ROOT_PGTAB_NUM * PGTAB_LENGTH * sizeof(struct pte)) >= 0);

	kprintf("[receiver] reading kstacks ids");
	test_assert(kportal_allow(in, remote, PORT) == 0);
	test_assert(kportal_read(in, kpages_alias, NUM_KPAGES * sizeof(int)) >= 0);

	dcache_invalidate();

	kprintf("[receiver] reading kstacks");
	// for (int i = 2 ; i < NUM_KPAGES; i++)
	for (int i = NUM_KPAGES - 1; i > 2; i--)
	{
		if (!kpages_alias[i])
			continue;
		// kprintf("kpage id %d", i);
		// kprintf("kpage address %d", (void *) kpool_id_to_addr(i));
		test_assert(kportal_allow(in, remote, PORT) == 0);
		test_assert(kportal_read(in, (void *) kpool_id_to_addr(i), PAGE_SIZE) >= 0);
	}

	kprintf("[receiver] reading frames");
	test_assert(kportal_allow(in, remote, PORT) == 0);
	test_assert(kportal_read(in, (void *) frames_alias, FRAMES_LENGTH * sizeof(bitmap_t)) >= 0);

	for (int i = 0; i < FRAMES_LENGTH * 32; i++)
	{
		if (!bitmap_check_bit(frames_alias, i))
			continue;

		test_assert(kportal_allow(in, remote, PORT) == 0);
		test_assert(kportal_read(in, (void *) (UBASE_PHYS + (i << PAGE_SHIFT)), PAGE_SIZE) >= 0);
	}

	semaphore_up(&sem3);

	kprintf("[receiver] done");

	return (0);
}

PRIVATE void receiver()
{
	int local, remote;

	local  = SLAVE_NODENUM + 1;
	remote = SLAVE_NODENUM;

	kprintf("receiver -> user length: %d", &__USER_END - &__USER_START);

	test_assert(ktask_create(&trecv, task_receiver, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	test_assert(ktask_dispatch2(&trecv, local, remote) == 0);

	semaphore_down(&sem3);
}

PRIVATE void receiver2()
{
	int remote, syncin, syncout;

	int nodes[] = {MASTER_NODENUM, SLAVE_NODENUM, SLAVE_NODENUM + 1};

	// local  = SLAVE_NODENUM + 1;
	remote = SLAVE_NODENUM;

	test_assert((syncin = ksync_create(nodes, 3, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout = ksync_open(nodes, 3, SYNC_ALL_TO_ONE)) >= 0);

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);

	kprintf("receiver -> user length: %d", &__USER_END - &__USER_START);

	kprintf("migrate from");
	test_assert(kmigrate_from(remote) == 0);
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

	semaphore_init(&sem1, 0);
	semaphore_init(&sem2, 0);
	semaphore_init(&sem3, 0);
	
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