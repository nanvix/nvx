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
#include <nanvix/sys/mailbox.h>


#define FRAMES_BIT_LENGTH (FRAMES_LENGTH * sizeof(bitmap_t))

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


PRIVATE int migration_inportal = MIGRATION_PORTAL_NULL;
PRIVATE int migration_outportal = MIGRATION_PORTAL_NULL;
PRIVATE int migration_inmailbox = MIGRATION_MAILBOX_NULL;
PRIVATE int migration_outmailbox_sender = MIGRATION_MAILBOX_NULL;
PRIVATE int migration_outmailbox_receiver = MIGRATION_MAILBOX_NULL;

PRIVATE struct migration_message request_message;
PRIVATE struct migration_message mmsg;

PRIVATE int mstate = MSTATE_INIT;
PRIVATE int loop_index = -1;
PRIVATE int sender_notification_status = NOT_NOTIFIED;
PRIVATE int receiver_notification_status = NOT_NOTIFIED;

PRIVATE ktask_t mtask_migrate_to;

PRIVATE ktask_t mmbox_aread;
PRIVATE ktask_t mmbox_rwait;
PRIVATE ktask_t mmbox_awrite;
PRIVATE ktask_t mmbox_wwait;
PRIVATE ktask_t mhandler;

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

PRIVATE void migration_control_reset()
{
	mstate = MSTATE_INIT;
	migration_message_clear(&mmsg);
	sender_notification_status = NOT_NOTIFIED;
	receiver_notification_status = NOT_NOTIFIED;
	if (migration_outportal != MIGRATION_PORTAL_NULL)
	{
		kportal_close(migration_outportal);
		migration_outportal = MIGRATION_PORTAL_NULL;
	}
	if (migration_outmailbox_receiver != MIGRATION_MAILBOX_NULL)
	{
		kmailbox_close(migration_outmailbox_receiver);
		migration_outmailbox_receiver = MIGRATION_MAILBOX_NULL;
	}
	if (migration_outmailbox_sender != MIGRATION_MAILBOX_NULL)
	{
		kmailbox_close(migration_outmailbox_sender);
		migration_outmailbox_sender = MIGRATION_MAILBOX_NULL;
	}
	
}

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
	}
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
			(word_t) mmsg.sender,
			(word_t) MIGRATION_PORTAL_PORTNUM
		);
	}
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
	
	ktask_exit2(
		0, 
		KTASK_MANAGEMENT_USER0,
		KTASK_MERGE_ARGS_FN_REPLACE,
		(word_t) margs.upper_bound,
		(word_t) is_sender
	);


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

	migration_control_reset();
	kprintf("unfrezing");
	
	dcache_invalidate();
	tlb_flush();
	
	kunfreeze();

	return 0;
}

PRIVATE int nanvix_mhandler(
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

	int local_node = knode_get_num();

	if (mmsg.receiver == mmsg.sender)
		goto error;

	switch(mmsg.opcode)
	{
		case MOPCODE_MIGRATE_TO:
			if (mmsg.receiver == local_node)
				goto error;
			KASSERT((migration_outportal = kportal_open(
				mmsg.sender, mmsg.receiver, MIGRATION_PORTAL_PORTNUM
			)) >= 0);
			ktask_exit1(
				0,
				KTASK_MANAGEMENT_USER0,
				KTASK_MERGE_ARGS_FN_REPLACE,
				(word_t) 1 // is sender
			);
			break;
		case MOPCODE_MIGRATE_FROM:
			kfreeze();
			if (mmsg.sender == local_node)
				goto error;
			ktask_exit1(
				0,
				KTASK_MANAGEMENT_USER0,
				KTASK_MERGE_ARGS_FN_REPLACE,
				(word_t) 0 // is not sender
			);
			break;
		default:
			goto error;
	}

	return (0);

	error:
		kunfreeze();
		ktask_exit0(0, KTASK_MANAGEMENT_USER1);
	
	return (-1);
}

PRIVATE int migration_states_task_flow_init()
{
	// state handler tasks
	ktask_create(&mtask_state_handler, &mstate_handler, 0, KTASK_MANAGEMENT_DEFAULT);
	ktask_create(&mtask_write, &mwrite, 0, KTASK_MANAGEMENT_DEFAULT);
	ktask_create(&mtask_read, &mread, 0, KTASK_MANAGEMENT_DEFAULT);
	ktask_create(&mtask_finish, &mfinish, 0, KTASK_MANAGEMENT_DEFAULT);
	ktask_create(&mtask_loop, &mloop, 0, KTASK_MANAGEMENT_DEFAULT);

	ktask_portal_write(&awrite, &wwait);
	ktask_portal_read(&allow, &aread, &rwait);

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

	return (0);
}

PRIVATE int __migrate_to(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int receiver_nodenum = (int) arg0;
	UNUSED(arg1);
	UNUSED(arg2);
	UNUSED(arg3);
	UNUSED(arg4);

	if(receiver_notification_status == NOT_NOTIFIED && sender_notification_status == NOT_NOTIFIED)	
		kfreeze();

	request_message.receiver = receiver_nodenum;
	request_message.sender = knode_get_num();
	
	if (receiver_notification_status == NOT_NOTIFIED)
	{
		receiver_notification_status = NOTIFIED;
		request_message.opcode = MOPCODE_MIGRATE_FROM;
		ktask_exit3(
			0,
			KTASK_MANAGEMENT_USER0,
			KTASK_MERGE_ARGS_FN_REPLACE,
			(word_t) migration_outmailbox_receiver,
			(word_t) &request_message,
			(word_t) sizeof(struct migration_message)
		);
	}
	else if (sender_notification_status == NOT_NOTIFIED)
	{
		sender_notification_status = NOTIFIED;
		request_message.opcode = MOPCODE_MIGRATE_TO;
		ktask_exit3(
			0,
			KTASK_MANAGEMENT_USER0,
			KTASK_MERGE_ARGS_FN_REPLACE,
			(word_t) migration_outmailbox_sender,
			(word_t) &request_message,
			(word_t) sizeof(struct migration_message)
		);
	}
	else
	{
		ktask_exit0(0, KTASK_MANAGEMENT_USER1);
	}

    return (0);
}

PUBLIC int kmigrate_to(int receiver_nodenum)
{
	int local_node = knode_get_num();
	if (UNLIKELY((unsigned int) receiver_nodenum >= PROCESSOR_NOC_NODES_NUM))
        return (-EINVAL);

	if (UNLIKELY(receiver_nodenum == local_node))
		return (-EINVAL);

	KASSERT((migration_outmailbox_receiver = kmailbox_open(receiver_nodenum, MIGRATION_MAILBOX_PORTNUM)) >= 0);
	KASSERT((migration_outmailbox_sender = kmailbox_open(local_node, MIGRATION_MAILBOX_PORTNUM)) >= 0);

	KASSERT(ktask_dispatch1(&mtask_migrate_to, (word_t) receiver_nodenum) == 0);

	return (0);
}

PUBLIC void kmigration_init(void)
{
	KASSERT((migration_inmailbox = kmailbox_create(knode_get_num(), MIGRATION_MAILBOX_PORTNUM)) >= 0);
	KASSERT((migration_inportal = kportal_create(knode_get_num(), MIGRATION_PORTAL_PORTNUM)) >= 0);

	KASSERT(ktask_create(&mhandler, &nanvix_mhandler, 0, KTASK_MANAGEMENT_DEFAULT) == 0);
	KASSERT(ktask_create(&mtask_migrate_to, &__migrate_to, 0, KTASK_MANAGEMENT_DEFAULT) == 0);

	KASSERT(ktask_mailbox_read(&mmbox_aread, &mmbox_rwait) == 0);
	KASSERT(ktask_mailbox_write(&mmbox_awrite, &mmbox_wwait) == 0);	

	KASSERT(ktask_connect(&mtask_migrate_to, &mmbox_awrite, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER0) == 0);
	KASSERT(ktask_connect(&mmbox_wwait, &mtask_migrate_to, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER0) == 0);

	KASSERT(ktask_connect(&mmbox_rwait, &mhandler, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER0) == 0);
	KASSERT(ktask_connect(&mhandler, &mtask_state_handler, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER0) == 0);
	KASSERT(ktask_connect(&mhandler, &mtask_finish, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER1) == 0);
	KASSERT(ktask_connect(&mtask_finish, &mmbox_aread, KTASK_CONN_IS_FLOW, KTASK_CONN_IS_PERSISTENT, KTASK_TRIGGER_USER0) == 0);

	migration_states_task_flow_init();
	migration_control_reset();

	KASSERT(ktask_dispatch3(
		&mmbox_aread,
		(word_t) migration_inmailbox,
		(word_t) &mmsg,
		(word_t) sizeof(struct migration_message)
	) == 0);
}
