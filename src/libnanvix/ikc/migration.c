/*
 * MIT License
 *
 * Copyright(c) 2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
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

#include <nanvix/kernel/kernel.h>
#include <nanvix/kernel/uarea.h>
#include <nanvix/sys/task.h>
#include <nanvix/sys/migration.h>
#include <nanvix/sys/noc.h>
#include <nanvix/sys/portal.h>
#include <nanvix/sys/sync.h>


#define FRAMES_BIT_LENGTH (FRAMES_LENGTH * sizeof(bitmap_t))
#define MPORT 0
#define SENDER PROCESSOR_NODENUM_LEADER
#define RECEIVER (PROCESSOR_NODENUM_LEADER + 1)

#define VERBOSE 0

struct mtask_args {
	int lower_bound;
	int upper_bound;
	char *(*cond_fn)(int);
	char *buffer;
	int size;
};

struct mtask_args margs = {
	.lower_bound = 0,
	.upper_bound = 1,
	.cond_fn = NULL,
	.buffer = NULL,
	.size = 0
};

int migration_inportal = -1;
int migration_outportal = -1;

PRIVATE int sender = SENDER;

PRIVATE int mstate = -1;
PRIVATE int loop_index = -1;

PRIVATE ktask_t mtask_finish;
PRIVATE ktask_t mtask_state_handler;
PRIVATE ktask_t mtask_write;
PRIVATE ktask_t mtask_read;
PRIVATE ktask_t mtask_loop;
PRIVATE ktask_t awrite;
PRIVATE ktask_t wwait;
PRIVATE ktask_t allow;
PRIVATE ktask_t aread;
PRIVATE ktask_t rwait;
// PRIVATE ktask_t mtask_portal_write;
// PRIVATE ktask_t mtask_portal_allow;
// PRIVATE ktask_t mtask_portal_read;

// PRIVATE int mportal_allow(
// 	word_t arg0,
// 	word_t arg1,
// 	word_t arg2,
// 	word_t arg3,
// 	word_t arg4
// )
// {
// 	int inportal = (int) arg0;
// 	int remote = (int) arg1;
// 	int port = (int) arg2;
// 	char *buffer = (char *) arg3;
// 	int size = (int) arg4;

// 	KASSERT(kportal_allow(inportal, remote, port) == 0);
	
// 	ktask_exit3(
// 		0, 
// 		KTASK_MANAGEMENT_USER0,
// 		KTASK_MERGE_ARGS_FN_REPLACE,
// 		(word_t) inportal,
// 		(word_t) buffer,
// 		(word_t) size
// 	);
// 	return (0);
// }

// PRIVATE int mportal_read(
// 	word_t arg0,
// 	word_t arg1,
// 	word_t arg2,
// 	word_t arg3,
// 	word_t arg4
// )
// {
// 	int inportal = (int) arg0;
// 	char *buffer = (char *) arg1;
// 	int size = (int) arg2;

// 	UNUSED(arg3);
// 	UNUSED(arg4);

// 	KASSERT(kportal_read(inportal, buffer, size) >= 0);

// 	ktask_exit0(0, KTASK_MANAGEMENT_USER0);

// 	return (0);
// }

// PRIVATE int mportal_write(
// 	word_t arg0,
// 	word_t arg1,
// 	word_t arg2,
// 	word_t arg3,
// 	word_t arg4
// )
// {
// 	int outportal = (int) arg0;
// 	char *buffer = (char *) arg1;
// 	int size = (int) arg2;

// 	UNUSED(arg3);
// 	UNUSED(arg4);

// 	KASSERT(kportal_write(outportal, buffer, size) >= 0);
// 	ktask_exit0(0, KTASK_MANAGEMENT_USER0);

// 	return (0);
// }

// ASK: maybe trigger user0 on rw?

PRIVATE int mwrite(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	struct mtask_args *args = (struct mtask_args *) arg0;

	// int lower_bound = args->lower_bound;
	// int upper_bound = args->upper_bound;
	char *(*cond_fn)(int) = args->cond_fn;
	char *buffer = args->buffer;
	int size = args->size;

	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	// for (int i = lower_bound; i < upper_bound; i++)
	// {
		/* checks if we need to send current index */
		char *buffer_send = cond_fn ? (*cond_fn)(loop_index) : buffer;
		if (buffer_send)
		{
			#if VERBOSE
				kprintf("sending %x", buffer_send);
			#endif
			ktask_exit3(
				0,
				KTASK_MANAGEMENT_USER1,
				KTASK_MERGE_ARGS_FN_REPLACE,
				(word_t) migration_outportal,
				(word_t) buffer_send,
				(word_t) size
			);
			// KASSERT(kportal_write(migration_outportal, buffer_send, size) >= 0);
		}
	// }
	else
	{
		#if VERBOSE
			kprintf("done");
		#endif
		ktask_exit0(0, KTASK_MANAGEMENT_USER0);
	}

	return (0);
}

PRIVATE int mread(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	struct mtask_args *args = (struct mtask_args *) arg0;

	// int lower_bound = args->lower_bound;
	// int upper_bound = args->upper_bound;
	char *(*cond_fn)(int) = args->cond_fn;
	char *buffer = args->buffer;
	int size = args->size;

	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	// for (int i = lower_bound; i < upper_bound; i++)
	// {
		/* checks if we need to send current index */
		char *buffer_recv = cond_fn ? (*cond_fn)(loop_index) : buffer;
		if (buffer_recv)
		{
			#if VERBOSE
				kprintf("receiving %x", buffer_recv);
			#endif

			ktask_exit5(
				0, 
				KTASK_MANAGEMENT_USER1,
				KTASK_MERGE_ARGS_FN_REPLACE,
				(word_t) migration_inportal,
				(word_t) buffer_recv,
				(word_t) size,
				(word_t) sender,
				(word_t) MPORT
			);
			// KASSERT(kportal_allow(migration_inportal, sender, MPORT) == 0);
			// KASSERT(kportal_read(migration_inportal, buffer_recv, size) >= 0);
		}
	// }
	else
	{
		#if VERBOSE
			kprintf("done");
		#endif
		ktask_exit0(0, KTASK_MANAGEMENT_USER0);
	}
	return (0);
}

PRIVATE int mloop(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int upper_bound = (int) arg0;
	int is_sender = (int) arg1;
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	loop_index++;

	if (loop_index >= upper_bound)
		ktask_exit0(
			0,
			KTASK_MANAGEMENT_USER2
		);
	else if (is_sender)
		ktask_exit0(
			0,
			KTASK_MANAGEMENT_USER0
		);
	else
		ktask_exit0(
			0,
			KTASK_MANAGEMENT_USER1
		);
	
	return (0);
}

PRIVATE char *mcond_thread_ustacks(int index)
{
	#if VERBOSE
		kprintf("mcond_thread_ustacks index %d", index);
	#endif
	if (uarea.threads[index].tid == KTHREAD_NULL_TID)
		return NULL;
	return (char *) uarea.ustacks[index];
}

PRIVATE char *mcond_thread_kstacks(int index)
{
	#if VERBOSE
		kprintf("mcond_thread_kstacks index %d", index);
	#endif
	if (uarea.threads[index].tid == KTHREAD_NULL_TID)
		return NULL;
	return (char *) uarea.kstacks[index];
}

PRIVATE char *mcond_kpage(int index)
{
	if (!kpages_alias[index])
		return NULL;
	return (char *) kpool_id_to_addr(index);	
}

PRIVATE char *mcond_frames(int index)
{
	if (!bitmap_check_bit(frames_alias, index))
		return NULL;
	return (char *) (UBASE_PHYS + (index << PAGE_SHIFT));
}

PRIVATE int mstate_handler(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int is_sender = (int) arg0;
	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	margs.lower_bound = 0;
	margs.upper_bound = 1;
	margs.cond_fn = NULL;
	margs.buffer = NULL;
	margs.size = 0;

	kprintf("mstate: %d", mstate);

	switch (mstate) {
		case MSTATE_INIT:
			break;
		case MSTATE_SECTIONS:
			margs.buffer = (char *) &__USER_START;
			margs.size = &__USER_END - &__USER_START;
			break;
		case MSTATE_UAREA:
			margs.buffer = (char *) &uarea;
			margs.size = sizeof(uarea);
			break;
		case MSTATE_THREAD_USTACKS:
			margs.upper_bound = THREAD_MAX;
			margs.cond_fn = &mcond_thread_ustacks;
			margs.buffer = (char *) uarea.ustacks;
			margs.size = PAGE_SIZE;
			break;
		case MSTATE_THREAD_KSTACKS:
			margs.upper_bound = THREAD_MAX;
			margs.cond_fn = &mcond_thread_kstacks;
			margs.buffer = (char *) uarea.kstacks;
			margs.size = PAGE_SIZE;
			break;
		case MSTATE_PAGEDIR:
			margs.buffer = (char *) root_pgdir;
			margs.size = sizeof(struct pde) * PGDIR_LENGTH;
			break;
		case MSTATE_PAGETAB:
			margs.buffer = (char *) kernel_pgtab;
			margs.size = ROOT_PGTAB_NUM * PGTAB_LENGTH * sizeof(struct pte);
			break;
		case MSTATE_KSTACKSIDS:
			margs.buffer = (char *) kpages_alias;
			margs.size = NUM_KPAGES * sizeof(int);
			break;
		case MSTATE_KSTACKSPHYS:
			margs.lower_bound = 3;
			margs.upper_bound = NUM_KPAGES;
			margs.cond_fn = &mcond_kpage;
			margs.buffer = (char *) KPOOL_VIRT;
			margs.size = PAGE_SIZE;
			break;
		case MSTATE_FRAMES_BITMAP:
			margs.buffer = (char *) frames_alias;
			margs.size = FRAMES_LENGTH * sizeof(bitmap_t);
			break;
		case MSTATE_FRAMES_PHYS:
			margs.upper_bound = FRAMES_LENGTH * sizeof(bitmap_t);
			margs.cond_fn = &mcond_frames;
			margs.buffer = (char *) UBASE_PHYS;
			margs.size = PAGE_SIZE;
			break;
		case MSTATE_FINISH:
			ktask_exit1(
				0, 
				KTASK_MANAGEMENT_USER1,
				KTASK_MERGE_ARGS_FN_REPLACE,
				(word_t) &margs
			);
			return (0);
		default:
			KASSERT(0 == 1);
	}

	mstate++;
	loop_index = margs.lower_bound - 1; // to compensate for the ++ in the loop
	
	ktask_t *comm_task = is_sender ? &mtask_write : &mtask_read;
	ktask_set_arguments(
		comm_task,
		(word_t) &margs, 0, 0, 0, 0
	);

	
	// if (margs.cond_fn != NULL)
		ktask_exit2(
			0, 
			KTASK_MANAGEMENT_USER0,
			KTASK_MERGE_ARGS_FN_REPLACE,
			(word_t) margs.upper_bound,
			(word_t) is_sender
		);
	// else
	// 	ktask_exit1(
	// 		0, 
	// 		KTASK_MANAGEMENT_USER0,
	// 		KTASK_MERGE_ARGS_FN_REPLACE,
	// 		(word_t) &margs
	// 	);

	return (0);
}

PRIVATE int mfinish(
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
	return 0;
}

PRIVATE int __kernel_migrate(int is_sender)
{
	int ret;
	// int (*mtask_fn)(word_t, word_t, word_t, word_t, word_t);

	// mtask_fn = is_sender ? mwrite : mread;

	// state handler tasks
	ktask_create(&mtask_state_handler, &mstate_handler, 0, KTASK_MANAGEMENT_DEFAULT);
	ktask_create(&mtask_write, &mwrite, 0, KTASK_MANAGEMENT_DEFAULT);
	ktask_create(&mtask_read, &mread, 0, KTASK_MANAGEMENT_DEFAULT);
	ktask_create(&mtask_finish, &mfinish, 0, KTASK_MANAGEMENT_DEFAULT);
	ktask_create(&mtask_loop, &mloop, 0, KTASK_MANAGEMENT_DEFAULT);

	// portal read/write handling tasks
	// if (is_sender)
		// ktask_create(&mtask_portal_write, &mportal_write, 0, KTASK_MANAGEMENT_DEFAULT);
		ktask_portal_write(&awrite, &wwait);
	// else
	// {
		// ktask_create(&mtask_portal_allow, &mportal_allow, 0, KTASK_MANAGEMENT_DEFAULT);
		// ktask_create(&mtask_portal_read, &mportal_read, 0, KTASK_MANAGEMENT_DEFAULT);
		ktask_portal_read(&allow, &aread, &rwait);
	// } 

	// state handler connections
	ktask_connect(
		&mtask_state_handler,
		&mtask_loop,
		KTASK_CONN_IS_FLOW,
		KTASK_CONN_IS_PERSISTENT,
		KTASK_TRIGGER_USER0
	);
	ktask_connect(
		&mtask_loop,
		&mtask_write,
		KTASK_CONN_IS_FLOW,
		KTASK_CONN_IS_PERSISTENT,
		KTASK_TRIGGER_USER0
	);
	ktask_connect(
		&mtask_write,
		&mtask_loop,
		KTASK_CONN_IS_FLOW,
		KTASK_CONN_IS_PERSISTENT,
		KTASK_TRIGGER_USER0
	);
	ktask_connect(
		&mtask_loop,
		&mtask_read,
		KTASK_CONN_IS_FLOW,
		KTASK_CONN_IS_PERSISTENT,
		KTASK_TRIGGER_USER1
	);
	ktask_connect(
		&mtask_read,
		&mtask_loop,
		KTASK_CONN_IS_FLOW,
		KTASK_CONN_IS_PERSISTENT,
		KTASK_TRIGGER_USER0
	);
	ktask_connect(
		&mtask_loop,
		&mtask_state_handler,
		KTASK_CONN_IS_FLOW,
		KTASK_CONN_IS_PERSISTENT,
		KTASK_TRIGGER_USER2
	);
	ktask_connect(
		&mtask_state_handler,
		&mtask_finish,
		KTASK_CONN_IS_FLOW,
		KTASK_CONN_IS_PERSISTENT,
		KTASK_TRIGGER_USER1
	);

	// portal read/write handling connections
	// if (is_sender)
	// {
		KASSERT(ktask_connect(
			&mtask_write,
			&awrite,
			KTASK_CONN_IS_FLOW,
			KTASK_CONN_IS_PERSISTENT,
			KTASK_TRIGGER_USER1
		) == 0);
		
		KASSERT(ktask_connect(
			&wwait,
			&mtask_loop,
			KTASK_CONN_IS_FLOW,
			KTASK_CONN_IS_PERSISTENT,
			KTASK_TRIGGER_USER0
		) == 0);
	// }
	// else
	// {
		KASSERT(ktask_connect(
			&mtask_read,
			&allow,
			KTASK_CONN_IS_FLOW,
			KTASK_CONN_IS_PERSISTENT,
			KTASK_TRIGGER_USER1
		) == 0);

		KASSERT(ktask_connect(
			&rwait,
			&mtask_loop,
			KTASK_CONN_IS_FLOW,
			KTASK_CONN_IS_PERSISTENT,
			KTASK_TRIGGER_USER0
		) == 0);
	// }

	mstate = MSTATE_SECTIONS;

	kprintf("dispatching");
	if ((ret = (ktask_dispatch1(&mtask_state_handler, is_sender))) != 0)
		return (ret);

	kprintf("waiting");
	ktask_wait(&mtask_finish);

	kprintf("unlinking");
    ktask_unlink(&mtask_state_handler);
	ktask_unlink(&mtask_read);
	ktask_unlink(&mtask_write);
	ktask_unlink(&mtask_finish);

	return (0);
}

/*
PRIVATE int msend(
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

	int receiver = (int) (arg0);
	int sender = (int) (arg1);
	int out;

	UNUSED(receiver);
	UNUSED(sender);
	// int syncin, syncout;

	// int nodes[2];
	// nodes[0] = receiver;
	// nodes[1] = sender;

	kfreeze();

	out = migration_outportal;
	
	
	kprintf("port %d",kportal_get_port(out));

	// KASSERT((syncin = ksync_create(nodes, 2, SYNC_ONE_TO_ALL)) >= 0);
	// KASSERT((syncout = ksync_open(nodes, 2, SYNC_ALL_TO_ONE)) >= 0);
	// KASSERT(ksync_wait(syncin) == 0);
	// KASSERT(ksync_signal(syncout) == 0);

	kprintf("[sender] sending user sections");
	KASSERT(kportal_write(out, &__USER_START, &__USER_END - &__USER_START) >= 0);

	kprintf("[sender] sending uarea");
	KASSERT(kportal_write(out, &uarea, sizeof(struct uarea)) >= 0);


	for (int i = 0; i < THREAD_MAX; ++i)
	{
		if (uarea.threads[i].tid != KTHREAD_NULL_TID)
		{
			kprintf("[sender] sending stacks");
			KASSERT(kportal_write(out, uarea.ustacks[i], PAGE_SIZE) >= 0);
			KASSERT(kportal_write(out, uarea.kstacks[i], PAGE_SIZE) >= 0);
		}
	}

	kprintf("[sender] sending page directories");
	KASSERT(kportal_write(out, root_pgdir, sizeof(struct pde) * PGDIR_LENGTH) >= 0);
	
	kprintf("[sender] sending page tables");
	KASSERT(kportal_write(out, kernel_pgtab, ROOT_PGTAB_NUM * PGTAB_LENGTH * sizeof(struct pte)) >= 0);
	
	kprintf("[sender] sending kstacks ids");
	KASSERT(kportal_write(out, kpages_alias, NUM_KPAGES * sizeof(int)) >= 0);

	kprintf("[sender] sending kstacks");
	for (int i = NUM_KPAGES - 1; i > 2; i--)
	{
		if (!kpages_alias[i])
			continue;
		KASSERT(kportal_write(out, (void *) kpool_id_to_addr(i), PAGE_SIZE) >= 0);
	}

	kprintf("[sender] sending frames");
	KASSERT(kportal_write(out, (void *) frames_alias, FRAMES_LENGTH * sizeof(bitmap_t)) >= 0);
	
	for (unsigned int i = 0; i < FRAMES_LENGTH * sizeof(bitmap_t); i++)
	{
		if (!bitmap_check_bit(frames_alias, i))
			continue;
		
		KASSERT(kportal_write(out, (void *) (UBASE_PHYS + (i << PAGE_SHIFT)), PAGE_SIZE) >= 0);
	}

	KASSERT(kportal_close(out) == 0);

	kunfreeze();

	return (0);
}
*/

/*
PRIVATE int mrecv(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
    int receiver = (int) arg0;
	int sender = (int) arg1;
	int in;

	UNUSED(receiver);
	UNUSED(sender);
	// int syncin, syncout;

	// int nodes[2];
	// nodes[0] = receiver;
	// nodes[1] = sender;

    UNUSED(arg2);
    UNUSED(arg3);
    UNUSED(arg4);

	kfreeze();

	// kprintf("port %d",kportal_get_port(in));

	in = migration_inportal;

	// KASSERT((syncin = ksync_create(nodes, 2, SYNC_ALL_TO_ONE)) >= 0);
	// KASSERT((syncout = ksync_open(nodes, 2, SYNC_ONE_TO_ALL)) >= 0);
	// KASSERT(ksync_signal(syncout) == 0);
	// KASSERT(ksync_wait(syncin) == 0);

	// int dummy = 50;
	// while(--dummy)
	// {
	// 	kprintf("dummy");
	// }


	kprintf("[receiver] reading user sections");
	KASSERT(kportal_allow(in, sender, MPORT) == 0);
	KASSERT(kportal_read(in, &__USER_START, &__USER_END - &__USER_START) >= 0);

	kprintf("[receiver] reading uarea");
	KASSERT(kportal_allow(in, sender, MPORT) == 0);
	KASSERT(kportal_read(in, &uarea, sizeof(struct uarea)) >= 0);

	for (int i = 0; i < THREAD_MAX; ++i)
	{
		if (uarea.threads[i].tid != KTHREAD_NULL_TID)
		{
			
			kprintf("[receiver] reading stacks");
			KASSERT(kportal_allow(in, sender, MPORT) == 0);
			KASSERT(kportal_read(in, uarea.ustacks[i], PAGE_SIZE) >= 0);
			KASSERT(kportal_allow(in, sender, MPORT) == 0);
			KASSERT(kportal_read(in, uarea.kstacks[i], PAGE_SIZE) >= 0);
		}
	}

	kprintf("[receiver] reading page directories");
	KASSERT(kportal_allow(in, sender, MPORT) == 0);
	KASSERT(kportal_read(in, root_pgdir, sizeof(struct pde) * PGDIR_LENGTH) >= 0);

	kprintf("[receiver] reading page tables");
	KASSERT(kportal_allow(in, sender, MPORT) == 0);
	KASSERT(kportal_read(in, kernel_pgtab, ROOT_PGTAB_NUM * PGTAB_LENGTH * sizeof(struct pte)) >= 0);

	kprintf("[receiver] reading kstacks ids");
	KASSERT(kportal_allow(in, sender, MPORT) == 0);
	KASSERT(kportal_read(in, kpages_alias, NUM_KPAGES * sizeof(int)) >= 0);

	// dcache_invalidate();

	kprintf("[receiver] reading kstacks");
	for (int i = NUM_KPAGES - 1; i > 2; i--)
	{
		if (!kpages_alias[i])
			continue;
		// kprintf("kpage id %d", i);
		// kprintf("kpage address %d", (void *) kpool_id_to_addr(i));
		KASSERT(kportal_allow(in, sender, MPORT) == 0);
		KASSERT(kportal_read(in, (void *) kpool_id_to_addr(i), PAGE_SIZE) >= 0);
	}

	kprintf("[receiver] reading frames");
	KASSERT(kportal_allow(in, sender, MPORT) == 0);
	KASSERT(kportal_read(in, (void *) frames_alias, FRAMES_LENGTH * sizeof(bitmap_t)) >= 0);

	for (int i = 0; i < FRAMES_LENGTH * 32; i++)
	{
		if (!bitmap_check_bit(frames_alias, i))
			continue;

		KASSERT(kportal_allow(in, sender, MPORT) == 0);
		KASSERT(kportal_read(in, (void *) (UBASE_PHYS + (i << PAGE_SHIFT)), PAGE_SIZE) >= 0);
	}

	kunfreeze();

	return (0);
}
*/


PUBLIC int kmigrate_to(int receiver_nodenum)
{
    int ret;

    if (UNLIKELY((unsigned int) receiver_nodenum >= PROCESSOR_NOC_NODES_NUM))
        return (-EINVAL);

	kprintf("to %d %d", receiver_nodenum, knode_get_num());
	
	ret = __kernel_migrate(1);

	// ret = msend();

    return (ret);
}

PUBLIC int kmigrate_from(int sender_nodenum)
{
    int ret;

    if (UNLIKELY((unsigned int) sender_nodenum >= PROCESSOR_NOC_NODES_NUM))
        return (-EINVAL);

	kprintf("from %d %d", knode_get_num(), sender_nodenum);

	ret = __kernel_migrate(0);

	// ret = mrecv();

    return (ret);
}

PUBLIC void kmigration_init(void)
{
	int node = knode_get_num();

	if (node == RECEIVER)
		KASSERT((migration_inportal = kportal_create(RECEIVER, MPORT)) >= 0);
	else if (node == SENDER)
		KASSERT((migration_outportal = kportal_open(SENDER, RECEIVER, MPORT)) >= 0);
}
