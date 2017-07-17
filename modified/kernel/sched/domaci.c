#include <linux/latencytop.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/cpuidle.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/interrupt.h>
#include <linux/mempolicy.h>
#include <linux/migrate.h>
#include <linux/task_work.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <trace/events/sched.h>
#include <linux/jiffies.h>
#include "domaci.h"
#include "sched.h"

DEFINE_SPINLOCK(domaci_lock);

const struct sched_class domaci_sched_class;

unsigned long domaci_index = 0;
unsigned int domaci_pids[PROCESS_BUFF_LEN] = {0};
unsigned long domaci_ticks[PROCESS_BUFF_LEN] = {0};
unsigned long domaci_rt[PROCESS_BUFF_LEN] = {0};

void domaci_update_statistics(struct task_struct *tsk)
{
	spin_lock(&domaci_lock);
	domaci_pids[domaci_index] = tsk->pid;
	domaci_rt[domaci_index] = jiffies - tsk->domaci_rt;
	domaci_ticks[domaci_index] = tsk->domaci_ticks;

	printk("pid=%d\t rt=%ld\t ticks=%ld\n", domaci_pids[domaci_index], domaci_rt[domaci_index], domaci_ticks[domaci_index]);

	domaci_index = (domaci_index + 1) % PROCESS_BUFF_LEN;
	spin_unlock(&domaci_lock);
}

static void enqueue_node_domaci(struct rq *rq, struct list_head *node)
{
	struct list_head *head_node = &rq->dq.domaci_list_head;

	if(head_node->next != head_node)
	{
		node->prev = head_node->prev;
		node->next = head_node;
		head_node->prev->next = node;
		head_node->prev = node;
	}
	else
	{
		head_node->next = node;
		head_node->prev = node;
		node->next = head_node;
		node->prev = head_node;
	}

}

static void
enqueue_task_domaci(struct rq *rq, struct task_struct *p, int flags)
{
	struct list_head *head_node = &rq->dq.domaci_list_head;

	if(!p->domaci_rt)
	{
		p->domaci_rt = jiffies;
		p->domaci_ticks = 0;
	}

	if(head_node->next != head_node)
	{
		p->domaci_list.prev = head_node->prev;
		p->domaci_list.next = head_node;
		head_node->prev->next = &p->domaci_list;
		head_node->prev = &p->domaci_list;
	}
	else
	{
		head_node->next = &p->domaci_list;
		head_node->prev = &p->domaci_list;
		p->domaci_list.next = head_node;
		p->domaci_list.prev = head_node;
	}

	rq->dq.nr_running++;
	add_nr_running(rq, 1);
}

static void dequeue_node_domaci(struct rq *rq, struct list_head *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;

	node->next = NULL;
	node->prev = NULL;
}

static void dequeue_task_domaci(struct rq *rq, struct task_struct *p, int flags)
{
	p->domaci_list.prev->next = p->domaci_list.next;
	p->domaci_list.next->prev = p->domaci_list.prev;

	p->domaci_list.next = NULL;
	p->domaci_list.prev = NULL;

	rq->dq.nr_running--;
	sub_nr_running(rq, 1);
}

static void yield_task_domaci(struct rq *rq)
{
	//printk("fja yield_task_domaci()\n");

	if (unlikely(rq->nr_running == 1))
		return;

	//pretpostavljam da kernel sam poziva schedule() nakon yield, da bi izabrao novi process

	dequeue_task_domaci(rq, rq->curr, 0);
	enqueue_task_domaci(rq, rq->curr, 0);
}

static bool yield_to_task_domaci(struct rq *rq, struct task_struct *p, bool preempt)
{
	//printk("fja yield_to_task_domaci()\n");

	yield_task_domaci(rq);

	return true;
}

static void check_preempt_domaci(struct rq *rq, struct task_struct *p, int wake_flags)
{
	//printk("fja check_preempt_domaci()\n");

	return;
}

static struct task_struct *
pick_next_task_domaci(struct rq *rq, struct task_struct *prev)
{
	struct task_struct *task;
	struct list_head *head_node = &rq->dq.domaci_list_head;
	struct list_head *node;

	if(head_node->next == head_node)
		 return NULL;

	task = list_entry(head_node->next, struct task_struct, domaci_list);

	node = head_node->next;
	dequeue_node_domaci(rq, node);
	enqueue_node_domaci(rq, node);

	task->domaci_time_slice = DOMACI_SLICE;
	return task;
}

/*called before the currently executing task is replaced with another
one. Note that pick_next_task and put_prev_task are not equivalent to putting tasks on and off
the run queue like enqueue_task and dequeue_task. Instead, they are responsible for giving the
CPU to a task and taking it away, respectively. Switching between tasks, however, still requires
performing a low-level context switch.*/

static void put_prev_task_domaci(struct rq *rq, struct task_struct *prev)
{
	return;
}

static int
select_task_rq_domaci(struct task_struct *p, int prev_cpu, int sd_flag, int wake_flags)
{
	//printk("fja select_task_rq_domaci()\n");

	return prev_cpu;
}

//updates statistics for the current task when the scheduling policy changes.
static void set_curr_task_domaci(struct rq *rq)
{
	//printk("fja set_curr_task_domaci()\n");

	return;
}

static void task_tick_domaci(struct rq *rq, struct task_struct *p, int queued)
{
	p->domaci_time_slice--;
	p->domaci_ticks++;

	if(p->domaci_time_slice < 1)
		set_tsk_need_resched(p);
}

/*
 * Priority of the task has changed. This may cause
 * us to initiate a push or pull.
 */
static void
prio_changed_domaci(struct rq *rq, struct task_struct *p, int oldprio)
{
	//printk("fja prio_changed_domaci\n");

	return;
}

static void switched_to_domaci(struct rq *rq, struct task_struct *p)
{
	//printk("fja switched_to_domaci\n");

	return;
}

static unsigned int get_rr_interval_domaci(struct rq *rq, struct task_struct *task)
{
	//printk("fja get_rr_interval_domaci\n");

	return DOMACI_SLICE;
}

/*
 * Update the current task's runtime statistics. Skip current tasks that
 * are not in our scheduling class.
 */
static void update_curr_domaci(struct rq *rq)
{
	//printk("fja update_curr_domaci\n");

	return;
}

asmlinkage long sys_domaci(unsigned long* niz)
{
			int i, ret = 0;
			unsigned long podaci[3 * PROCESS_BUFF_LEN] = {0};

			for(i = 0; i < PROCESS_BUFF_LEN; i++)
			{
				if(domaci_pids[i] > 0)
				{
					podaci[i] = domaci_pids[i];
					podaci[i + PROCESS_BUFF_LEN] = domaci_ticks[i];
					podaci[i + 2*PROCESS_BUFF_LEN] = domaci_rt[i];
				}
				else
					break;
			}

			if(copy_to_user(niz, podaci, sizeof(long) * 3 * PROCESS_BUFF_LEN))
				ret = -EFAULT;

			return ret;
}

const struct sched_class domaci_sched_class = {
	.next			= &fair_sched_class,
	.enqueue_task		= enqueue_task_domaci,
	.dequeue_task		= dequeue_task_domaci,
	.yield_task		= yield_task_domaci,
	.yield_to_task		= yield_to_task_domaci,

	.check_preempt_curr	= check_preempt_domaci,

	.pick_next_task		= pick_next_task_domaci,
	.put_prev_task		= put_prev_task_domaci,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_domaci,
	.migrate_task_rq	= NULL,

	.rq_online		= NULL,
	.rq_offline		= NULL,

	.task_waking		= NULL,
	.task_dead		= NULL,
	.set_cpus_allowed	= set_cpus_allowed_common,
#endif

	.set_curr_task	= set_curr_task_domaci,
	.task_tick		= task_tick_domaci,
	.task_fork		= NULL,

	.prio_changed		= prio_changed_domaci,
	.switched_from		= NULL,
	.switched_to		= switched_to_domaci,

	.get_rr_interval	= get_rr_interval_domaci,

	.update_curr		= update_curr_domaci,

#ifdef CONFIG_FAIR_GROUP_SCHED
	.task_move_group	= NULL,
#endif
};
