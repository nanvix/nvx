/*
 * MIT License
 *
 * Copyright(c) 2011-2020 The Maintainers of Nanvix
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

#define __NEED_RESOURCE

#include <nanvix/sys/thread.h>

#include "task.h"

#if __NANVIX_USE_COMM_WITH_TASKS

/**
 * @brief Cast to word_t.
 */
#define TO_W(x) ((word_t) (x))

/**
 * @brief Number of flows.
 */
#define IKC_FLOWS_MAX THREAD_MAX

/**
 * @brief Invalid communicator ID.
 */
#define IKC_FLOW_CID_INVALID (~0UL)

/**
 * @brief Get the flow pointer from a task pointer.
 */
#define IKC_FLOW_FROM_TASK(task, is_config) ((struct ikc_flow *) (task - (is_config ? 0 : 1)))

/**
 * We can use the core id instead of the thread id because the dispatcher
 * shares the same core of the master thread and then there is not exist
 * other thread that will call this function. So, core_get_id is faster than
 * kthread_self.
 *
 * Alternative: kthread_self() != KTHREAD_DISPATCHER_TID
 */
#define IKC_FLOW_THREAD_IS_USER (core_get_id() != KTHREAD_DISPATCHER_CORE)

/**
 * @brief Get the kernel ID of a thread.
 */
#define KERNEL_TID (((thread_get_curr() - KTHREAD_MASTER) - SYS_THREAD_MAX))

/**
 * @name Communication status from noc/active.h.
 */
/**@{*/
#define ACTIVE_COMM_SUCCESS  (0)
#define ACTIVE_COMM_AGAIN    (1)
#define ACTIVE_COMM_RECEIVED (2)
/**@}*/

/**
 * @brief Communication Flow
 *
 * @details The 'config' and 'wait' tasks are permanently connected. On
 * a succeded configuration, the handler task is connected to the 'wait' task.
 * When the communication completes, the handler releases 'wait' task to
 * complete the user-side communication.
 *
 *             +--------------------------------+
 *             v                                |
 *          config -------------------------> wait
 *                                             ^
 *          handler (setted on active) - - - - +
 *
 */
struct ikc_flow
{
	ktask_t config;
	ktask_t wait;

	int ret;
	int type;
	word_t cid;
	struct resource resource;
};

/**
 * @brief Communication Flow Pool
 *
 * @details This structure holds the tasks that compose a communication flow by
 * any thread. Currently, we only support one communication per thread. So,
 * a thread can request a read/write and wait for its conclusion. In the future,
 * we can modify to support more flows but we must study how we will preserve
 * memory because this can request huge static arrays of structures.
 * However, when we are dealing with the Dispatcher, we provide more
 * communication because its can executes multiples types of flows and merge
 * them.
 */
PRIVATE struct ikc_flow_pool
{
	struct ikc_flow dispatchers[IKC_FLOWS_MAX];
	struct ikc_flow users[IKC_FLOWS_MAX];
} ikc_flows;

/**
 * @brief Protection.
 */
PRIVATE spinlock_t ikc_flow_lock;

/**
 * @brief Get current communication flow.
 *
 * @param is_config Specify if we are on __ikc_flow_config or __ikc_flow_wait function.
 *
 * @return Current communication flow.
 */
PRIVATE struct ikc_flow * ikc_flow_get_flow(bool is_config)
{
	struct ikc_flow * flow;

#ifndef NDEBUG
	if (UNLIKELY(kthread_self() != KTHREAD_DISPATCHER_TID))
		kpanic("[kernel][ikc][task] Communication must be executed by the dispatcher.");
#endif

	flow = IKC_FLOW_FROM_TASK(ktask_current(), is_config);

	if (UNLIKELY(!resource_is_used(&flow->resource)))
		kpanic("[kernel][ikc][task] Communication flow not used.");

	if (UNLIKELY(!WITHIN(flow->type, IKC_FLOW_MAILBOX_READ, IKC_FLOW_INVALID)))
		kpanic("[kernel][ikc][task] Unknown flow.");

	return (flow);
}

/**
 * @brief Configure the communication.
 *
 * @param arg0 communicator ID
 * @param arg1 buffer pointer
 * @param arg2 size
 *
 * @return Non-negative on success, negative on failure.
 */
PRIVATE inline int ___ikc_flow_config(
	struct ikc_flow * flow,
	word_t arg0,
	word_t arg1,
	word_t arg2
)
{
	size_t size;

	size = (size_t) arg2;

	/* Do not need to send another piece? Complete. */
	switch (flow->type)
	{
		case IKC_FLOW_MAILBOX_READ:
			return (
				__kmailbox_aread(
					(int) arg0,
					(void *)(long) arg1,
					size
				)
			);

		case IKC_FLOW_MAILBOX_WRITE:
			return (
				__kmailbox_awrite(
					(int) arg0,
					(const void *)(long) arg1,
					size
				)
			);

		case IKC_FLOW_PORTAL_READ:
			return (
				__kportal_aread(
					(int) arg0,
					(void *)(long) arg1,
					(size <= KPORTAL_MESSAGE_DATA_SIZE) ? (size) : (KPORTAL_MESSAGE_DATA_SIZE)
				)
			);

		case IKC_FLOW_PORTAL_WRITE:
			return (
				__kportal_awrite(
					(int) arg0,
					(const void *)(long) arg1,
					(size <= KPORTAL_MESSAGE_DATA_SIZE) ? (size) : (KPORTAL_MESSAGE_DATA_SIZE)
				)
			);

		default:
			kpanic("[kernel][ikc][task] Incorrect communication type.");
			return (-1);
	}
}

/**
 * @brief Compare the return value with flags that results on a Again action.
 */
PRIVATE inline bool __ikc_flow_config_do_again(struct ikc_flow * flow, int ret)
{
	switch (flow->type)
	{
		case IKC_FLOW_MAILBOX_READ:
			return ((ret == -ETIMEDOUT) || (ret == -EBUSY) || (ret == -ENOMSG));

		case IKC_FLOW_MAILBOX_WRITE:
			return ((ret == -ETIMEDOUT) || (ret == -EBUSY) || (ret == -EAGAIN));

		case IKC_FLOW_PORTAL_READ:
			return ((ret == -EBUSY) || (ret == -ENOMSG));

		case IKC_FLOW_PORTAL_WRITE:
			return ((ret == -EACCES) || (ret == -EBUSY));

		/* Error. */
		default:
			return (false);
	}
}

/**
 * @brief Configure the communication.
 *
 * @param arg0 communicator ID
 * @param arg1 buffer pointer
 * @param arg2 size
 *
 * @return Non-negative on success, negative on failure.
 *
 * @details Exit action based on return value:
 * CONTINUE : Continue the loop to waiting task (don't release semaphore).
 * AGAIN    : Retries to configure the communication.
 * ERROR    : Complete the task with error.
 */
PRIVATE int __ikc_flow_config(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int ret;
	struct ikc_flow * flow;

	flow = ikc_flow_get_flow(true);

	/* Successful configure. */
	if ((ret = ___ikc_flow_config(flow, arg0, arg1, arg2)) >= 0)
	{
		ktask_exit5(ret,
			KTASK_MANAGEMENT_USER0,
			KTASK_MERGE_ARGS_FN_REPLACE,
			arg0, arg1, arg2, arg3, arg4
		);
	}

	/* Must do again. */
	else if (__ikc_flow_config_do_again(flow, ret))
		ktask_exit0(ret, KTASK_MANAGEMENT_AGAIN);

	/* Error. */
	else
		ktask_exit0(ret, KTASK_MANAGEMENT_ERROR);

	return (ret);
}

/**
 * @brief Wait form the communication.
 *
 * @param arg0 communicator ID
 *
 * @return Non-negative on success, negative on failure.
 */
PRIVATE inline int ___ikc_flow_wait(struct ikc_flow * flow, word_t arg0)
{
	switch (flow->type)
	{
		case IKC_FLOW_MAILBOX_READ:
		case IKC_FLOW_MAILBOX_WRITE:
			return (__kmailbox_wait((int) arg0));

		case IKC_FLOW_PORTAL_READ:
		case IKC_FLOW_PORTAL_WRITE:
			return (__kportal_wait((int) arg0));

		default:
			kpanic("[kernel][ikc][task] Incorrect communication type.");
			return (-1);
	}
}

/**
 * @brief Verify if we need send another piece of the message.
 */
PRIVATE inline int __ikc_flow_verify_continuation(
	struct ikc_flow * flow,
	int ret,
	int cid,
	void * buffer,
	size_t size,
	int remote,
	int port
)
{
	switch (flow->type)
	{
		/* Just complete the communication. */
		case IKC_FLOW_MAILBOX_READ:
		case IKC_FLOW_MAILBOX_WRITE:
			return (ktask_exit0(ret, KTASK_MANAGEMENT_USER0));

		case IKC_FLOW_PORTAL_READ:
		case IKC_FLOW_PORTAL_WRITE:
		{
			/* Do not need to send another piece? Complete. */
			if (size <= KPORTAL_MESSAGE_DATA_SIZE)
				return (ktask_exit0(ret, KTASK_MANAGEMENT_USER0));

			size   -= KPORTAL_MESSAGE_DATA_SIZE;
			buffer += KPORTAL_MESSAGE_DATA_SIZE;

			if (flow->type == IKC_FLOW_PORTAL_READ)
				kportal_allow(cid, remote, port);

			return (
				ktask_exit5(ret, 
					KTASK_MANAGEMENT_USER1,
					KTASK_MERGE_ARGS_FN_REPLACE,
					TO_W(cid), TO_W(buffer), TO_W(size), TO_W(remote), TO_W(port)
				)
			);
		}

		default:
			kpanic("[kernel][ikc][task] Incorrect communication type.");
			return (-1);
	}
}

/**
 * @brief Wait the communication.
 *
 * @param arg0 communicator ID
 *
 * @return Non-negative on success, negative on failure.
 *
 * @details Exit action based on return value:
 * FINISH   : Complete the task and stop de loop.
 * CONTINUE : Continue the loop to reconfigure (don't release semaphore).
 * ERROR    : Complete the task with error.
 */
PRIVATE int __ikc_flow_wait(
	word_t arg0,
	word_t arg1,
	word_t arg2,
	word_t arg3,
	word_t arg4
)
{
	int ret;
	struct ikc_flow * flow;

	UNUSED(arg1);
	UNUSED(arg2);

	flow = ikc_flow_get_flow(false);

	/* Successful waits for. */
	if (LIKELY((ret = ___ikc_flow_wait(flow, arg0)) >= 0))
	{
		if (UNLIKELY(ret == ACTIVE_COMM_RECEIVED))
			kpanic("[kernel][ikc][task] Wait shouldn't return RECEIVED constant.");

		/* Received piece of the message. */
		if (ret == ACTIVE_COMM_SUCCESS)
		{
			KASSERT(__ikc_flow_verify_continuation(
				flow,
				ret,
				(int) arg0,
				(void *) arg1,
				(size_t) arg2,
				(int) arg3,
				(int) arg4
			) >= 0);
		}

		/* Receives a message for other port. */
		else
			ktask_exit0(ret, KTASK_MANAGEMENT_USER1);
	}

	/* Error. */
	else
		ktask_exit0(ret, KTASK_MANAGEMENT_ERROR);

	return (ret);
}

/**
 * @brief Allocate a communication flow.
 *
 * @param is_user Thread type
 * @param type    Communication type
 * @param cid     Communicator ID
 * @param flow    Getter of the flow pointer
 *
 * @return Zero if successful configure the communication flow, non-zero
 * otherwise.
 */
PRIVATE int ikc_flow_alloc(bool is_user, int type, word_t cid, struct ikc_flow ** flow)
{
	int btype;
	struct ikc_flow * flows;

	flows = is_user ? ikc_flows.users : ikc_flows.dispatchers;
	btype = type - (type % 2);

	for (int i = 0; i < IKC_FLOWS_MAX; ++i)
	{
		/* This will get the last unused resource. */
		if (!resource_is_used(&flows[i].resource))
			*flow = &flows[i];

		/* Only one task per communinator. */
		else if (flows[i].cid == cid && WITHIN(flows[i].type, btype, btype + 2))
			return (-EBUSY);
	}

	return (*flow != NULL) ? (0) : (-EAGAIN);
}

/**
 * @brief Search a communication flow.
 *
 * @param is_user Thread type
 * @param type    Communication type
 * @param cid     Communicator ID
 *
 * @return Zero if successful configure the communication flow, non-zero
 * otherwise.
 */
PRIVATE struct ikc_flow * ikc_flow_search(bool is_user, int type, word_t cid)
{
	int btype;
	struct ikc_flow * flows;

	flows = is_user ? ikc_flows.users : ikc_flows.dispatchers;
	btype = type - (type % 2);

	for (int i = (IKC_FLOWS_MAX - 1); i >= 0; --i)
	{
		if (!resource_is_used(&flows[i].resource))
			continue;

		if (flows[i].cid != cid)
			continue;

		if (!WITHIN(flows[i].type, btype, btype + 2))
			continue;

		return (&flows[i]);
	}

	return (NULL);
}

/**
 * @brief Allocate and dispatch a communication flow.
 *
 * @param type Communication type
 * @param cid  Communicator ID
 * @param buf  Buffer pointer
 * @param size Size
 *
 * @return Zero if successful configure the communication flow, non-zero
 * otherwise.
 */
PUBLIC int ikc_flow_config(
	int type,
	word_t cid,
	word_t buf,
	word_t size,
	word_t remote,
	word_t port
)
{
	int ret;
	bool is_user;
	struct task * requester;
	struct ikc_flow * flow;

	/* Invalid type. */
	if (UNLIKELY(!WITHIN(type, IKC_FLOW_MAILBOX_READ, IKC_FLOW_INVALID)))
		return (-EINVAL);

	flow    = NULL;
	is_user = IKC_FLOW_THREAD_IS_USER;

	spinlock_lock(&ikc_flow_lock);

		if ((ret = ikc_flow_alloc(is_user, type, cid, &flow)) < 0)
			goto error0;

		flow->cid  = cid;
		flow->type = type;
		resource_set_used(&flow->resource);

	spinlock_unlock(&ikc_flow_lock);

	if (UNLIKELY((ret = ktask_dispatch5(&flow->config, cid, buf, size, remote, port)) < 0))
		goto error1;

	/**
	 * [user] Waits for the waiting flow because the configuration can failure
	 * and we must gets its error.
	 */
	if (is_user)
	{
		if (UNLIKELY((flow->ret = ktask_wait(&flow->wait)) < 0))
		{
			/**
			 * Error on configuration? Release the flow and return the error code.
			 * Otherwise, go to waiting operation to then return the error.
			 */
			if ((ret = ktask_get_return(&flow->config)) < 0)
				goto error1;
		}
	}

	/**
	 * [dispatcher]
	 */
	else
	{
		/* Requester task. */
		requester = ktask_current();

		/**
		 * Connect the wait operation to the current flow.
		 *
		 * TODO: If a error occurs, the entire flow will be aborted. This means
		 * that the Daemon flow will be abort. We must specify a trigger on
		 * aborting that not propagate the error but notify it.
		 */
		ret = ktask_connect(
			&flow->wait,
			ktask_get_child(requester, 0),
			KTASK_CONN_IS_DEPENDENCY,
			KTASK_CONN_IS_TEMPORARY,
			(KTASK_TRIGGER_USER0 | KTASK_TRIGGER_ERROR_CATCH)
		);

		if (ret < 0)
			goto error1;
	}

	return (size);

error1:
	spinlock_lock(&ikc_flow_lock);
		flow->cid  = IKC_FLOW_CID_INVALID;
		flow->type = IKC_FLOW_INVALID;
		resource_set_unused(&flow->resource);
error0:
	spinlock_unlock(&ikc_flow_lock);

	return (ret);
}

/**
 * @brief Wait for the dispatched communication flow.
 *
 * @param type Communication type
 * @param cid  Communicator ID
 *
 * @return Zero if successful waits for the communication flow, non-zero
 * otherwise.
 */
PUBLIC int ikc_flow_wait(int type, word_t cid)
{
	int ret;
	bool is_user;
	struct ikc_flow * flow;

	/* Invalid type. */
	if (UNLIKELY(!WITHIN(type, IKC_FLOW_MAILBOX_READ, IKC_FLOW_INVALID)))
		return (-EINVAL);

	flow    = NULL;
	is_user = IKC_FLOW_THREAD_IS_USER;

	spinlock_lock(&ikc_flow_lock);

		flow = ikc_flow_search(is_user, type, cid);

	spinlock_unlock(&ikc_flow_lock);

	if (UNLIKELY(flow == NULL))
		return (-EBADF);

	/**
	 * [user] The waiting function is done on config because we must detect if
	 * the config task failed. As we only release the waiting semaphore, we must
	 * wait for it on config to get the error coming from config failure.
	 */
	if (is_user)
		ret = flow->ret;

	/**
	 * [dispatcher] This function only will be executed when the waiting task
	 * completes (on-demand dependency between handler task and this task). So,
	 * we just decrement the waiting semaphore incremented before.
	 */
	else
		ret = ktask_trywait(&flow->wait);

	spinlock_lock(&ikc_flow_lock);

		flow->cid  = IKC_FLOW_CID_INVALID;
		flow->type = IKC_FLOW_INVALID;
		resource_set_unused(&flow->resource);

	spinlock_unlock(&ikc_flow_lock);

	return (ret);
}

/**
 * @brief Configure the base flow connections.
 *
 * @param flow Communication flow
 */
PRIVATE void __ikc_flow_init(struct ikc_flow * flow)
{
	KASSERT(ktask_create(&flow->config, __ikc_flow_config, KTASK_PRIORITY_LOW, 0, KTASK_TRIGGER_DEFAULT) == 0);
	KASSERT(ktask_create(&flow->wait, __ikc_flow_wait, KTASK_PRIORITY_HIGH, 0, KTASK_TRIGGER_DEFAULT) == 0);

	KASSERT(
		ktask_connect(
			&flow->config,
			&flow->wait,
			KTASK_CONN_IS_DEPENDENCY,
			KTASK_CONN_IS_PERSISTENT,
			KTASK_TRIGGER_DEFAULT
		) == 0
	);
	KASSERT(
		ktask_connect(
			&flow->wait,
			&flow->config,
			KTASK_CONN_IS_DEPENDENCY,
			KTASK_CONN_IS_PERSISTENT,
			KTASK_TRIGGER_USER1
		) == 0
	);

	if (IKC_FLOW_FROM_TASK(&flow->config, true) != flow)
		kpanic("[kernel][ikc][task] FLOW_FROM_TASK Macro doesn't work.");

	if (IKC_FLOW_FROM_TASK(&flow->wait, false) != flow)
		kpanic("[kernel][ikc][task] FLOW_FROM_TASK Macro doesn't work.");
}

/**
 * @brief Initialization of communication flows using tasks.
 */
PUBLIC void ikc_flow_init(void)
{
	for (int i = 0; i < IKC_FLOWS_MAX; ++i)
	{
		/* Setup management variables. */
		ikc_flows.users[i].resource       = RESOURCE_INITIALIZER;
		ikc_flows.users[i].type           = IKC_FLOW_INVALID;
		ikc_flows.users[i].cid            = IKC_FLOW_CID_INVALID;
		ikc_flows.users[i].ret            = 0;
		ikc_flows.dispatchers[i].resource = RESOURCE_INITIALIZER;
		ikc_flows.dispatchers[i].type     = IKC_FLOW_INVALID;
		ikc_flows.dispatchers[i].cid      = IKC_FLOW_CID_INVALID;
		ikc_flows.dispatchers[i].ret      = 0;

		/* Configure base flow. */
		__ikc_flow_init(&ikc_flows.users[i]);
		__ikc_flow_init(&ikc_flows.dispatchers[i]);
	}

	spinlock_init(&ikc_flow_lock);
}

#endif /* __NANVIX_USE_COMM_WITH_TASKS */

