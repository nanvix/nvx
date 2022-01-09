#include <nanvix/sys/task.h>
#include <nanvix/sys/semaphore.h>
#include <nanvix/sys/portal.h>
#include <nanvix/sys/mailbox.h>
#include <nanvix/kernel/uarea.h>
#include <nanvix/sys/sync.h>

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
	int nodes[] = {MASTER_NODENUM, SLAVE_NODENUM};
	char dummy = 'd';

	local  = SLAVE_NODENUM;
	remote = SLAVE_NODENUM + 1;

	kprintf("sender -> user length: %d", &__USER_END - &__USER_START);

	global = 0xc0ffee;
	
	test_assert((syncin = ksync_create(nodes, 2, SYNC_ONE_TO_ALL)) >= 0);
	test_assert((syncout = ksync_open(nodes, 2, SYNC_ALL_TO_ONE)) >= 0);

	test_assert((out = kmailbox_open(MASTER_NODENUM, PORT2)) >= 0);

	test_assert(ksync_wait(syncin) == 0);
	test_assert(ksync_signal(syncout) == 0);
	
	test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));
	test_assert(kmailbox_write(out, &dummy, sizeof(char)) == sizeof(char));

	test_assert(ktask_create(&tsend, task_send, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	test_assert(ktask_dispatch2(&tsend, local, remote) == 0);

	semaphore_down(&sem3);

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

PRIVATE void master()
{
	int in, syncin, syncout;
	char dummy;
	uint64_t t1, error;
	int nodes[] = {MASTER_NODENUM, SLAVE_NODENUM};

	test_assert((syncin = ksync_create(nodes, 2, SYNC_ALL_TO_ONE)) >= 0);
	test_assert((syncout = ksync_open(nodes, 2, SYNC_ONE_TO_ALL)) >= 0);

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
		fn = (knode_get_num() == SLAVE_NODENUM) ? sender : receiver;
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