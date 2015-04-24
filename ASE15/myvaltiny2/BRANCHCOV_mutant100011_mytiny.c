int __covered0;
int __covered1;
int __covered2;
int __covered3;
int __covered4;
int __covered5;
int __covered6;
int __covered7;
int __covered8;
int __covered9;
int __covered10;
int __covered11;
int __covered12;
int __covered13;
int __covered14;
int __covered15;
int __covered16;
int mutant_covered = 0;

int total_covered = 0;

/*
 * Read-Copy Update mechanism for mutual exclusion, the Bloatwatch edition.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Copyright IBM Corporation, 2008
 *
 * Author: Paul E. McKenney <paulmck@linux.vnet.ibm.com>
 *
 * For detailed explanation of Read-Copy Update mechanism see -
 *		Documentation/RCU
 */
#include "fake.h"

#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/rcupdate.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/cpu.h>
#include <linux/prefetch.h>
#include <linux/ftrace_event.h>

#include "rcu.h"

/* Forward declarations for tiny_plugin.h. */
struct rcu_ctrlblk;
static void __rcu_process_callbacks(struct rcu_ctrlblk *rcp);
static void rcu_process_callbacks(struct softirq_action *unused);
static void __call_rcu(struct rcu_head *head,
void (*func)(struct rcu_head *rcu),
struct rcu_ctrlblk *rcp);

static long long rcu_dynticks_nesting = DYNTICK_TASK_EXIT_IDLE;

#include "tiny_plugin.h"

/* Common code for rcu_idle_enter() and rcu_irq_exit(), see kernel/rcu/tree.c. */
static void rcu_idle_enter_common(long long newval)
{
	if (newval) {
		if (!__covered0) {__covered0 = 1; total_covered += 1;}
		RCU_TRACE(trace_rcu_dyntick(TPS("--="),
rcu_dynticks_nesting, newval));
		rcu_dynticks_nesting = newval;
		return;
	}
	RCU_TRACE(trace_rcu_dyntick(TPS("Start"),
				    rcu_dynticks_nesting, newval));
	if (IS_ENABLED(CONFIG_RCU_TRACE) && !is_idle_task(current)) {
		if (!__covered1) {__covered1 = 1; total_covered += 1;}
		struct task_struct *idle __maybe_unused = idle_task(smp_processor_id());

		RCU_TRACE(trace_rcu_dyntick(TPS("Entry error: not idle task"),
					    rcu_dynticks_nesting, newval));
		ftrace_dump(DUMP_ALL);
 mutant_covered = 1;
 /* MUTANT (rep_const) */		WARN_ONCE(-1, "Current pid: %d comm: %s / Idle pid: %d comm: %s",
			  current->pid, current->comm,
			  idle->pid, idle->comm); /* must be idle task! */
	}
	rcu_sched_qs(); /* implies rcu_bh_inc() */
	barrier();
	rcu_dynticks_nesting = newval;
}

/*
 * Enter idle, which is an extended quiescent state if we have fully
 * entered that mode (i.e., if the new value of dynticks_nesting is zero).
 */
void rcu_idle_enter(void)
{
	unsigned long flags;
	long long newval;

	local_irq_save(flags);
	WARN_ON_ONCE((rcu_dynticks_nesting & DYNTICK_TASK_NEST_MASK) == 0);
	if ((rcu_dynticks_nesting & DYNTICK_TASK_NEST_MASK) == DYNTICK_TASK_NEST_VALUE) {
		if (!__covered2) {__covered2 = 1; total_covered += 1;}
		newval = 0;
	}
	else {
		if (!__covered3) {__covered3 = 1; total_covered += 1;}
		newval = rcu_dynticks_nesting - DYNTICK_TASK_NEST_VALUE;
	}
	rcu_idle_enter_common(newval);
	local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(rcu_idle_enter);

/*
 * Exit an interrupt handler towards idle.
 */
void rcu_irq_exit(void)
{
	unsigned long flags;
	long long newval;

	local_irq_save(flags);
	newval = rcu_dynticks_nesting - 1;
	WARN_ON_ONCE(newval < 0);
	rcu_idle_enter_common(newval);
	local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(rcu_irq_exit);

/* Common code for rcu_idle_exit() and rcu_irq_enter(), see kernel/rcu/tree.c. */
static void rcu_idle_exit_common(long long oldval)
{
	if (oldval) {
		if (!__covered4) {__covered4 = 1; total_covered += 1;}
		RCU_TRACE(trace_rcu_dyntick(TPS("++="),
					    oldval, rcu_dynticks_nesting));
		return;
	}
	RCU_TRACE(trace_rcu_dyntick(TPS("End"), oldval, rcu_dynticks_nesting));
	if (IS_ENABLED(CONFIG_RCU_TRACE) && !is_idle_task(current)) {
		if (!__covered5) {__covered5 = 1; total_covered += 1;}
		struct task_struct *idle __maybe_unused = idle_task(smp_processor_id());

		RCU_TRACE(trace_rcu_dyntick(TPS("Exit error: not idle task"),
oldval, rcu_dynticks_nesting));
		ftrace_dump(DUMP_ALL);
		WARN_ONCE(1, "Current pid: %d comm: %s / Idle pid: %d comm: %s",
			  current->pid, current->comm,
			  idle->pid, idle->comm); /* must be idle task! */
	}
}

/*
 * Exit idle, so that we are no longer in an extended quiescent state.
 */
void rcu_idle_exit(void)
{
	unsigned long flags;
	long long oldval;

	local_irq_save(flags);
	oldval = rcu_dynticks_nesting;
	WARN_ON_ONCE(rcu_dynticks_nesting < 0);
	if (rcu_dynticks_nesting & DYNTICK_TASK_NEST_MASK) {
		if (!__covered6) {__covered6 = 1; total_covered += 1;}
		rcu_dynticks_nesting += DYNTICK_TASK_NEST_VALUE;
	}
	else {
		if (!__covered7) {__covered7 = 1; total_covered += 1;}
		rcu_dynticks_nesting = DYNTICK_TASK_EXIT_IDLE;
	}
	rcu_idle_exit_common(oldval);
	local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(rcu_idle_exit);

/*
 * Enter an interrupt handler, moving away from idle.
 */
void rcu_irq_enter(void)
{
	unsigned long flags;
	long long oldval;

	local_irq_save(flags);
	oldval = rcu_dynticks_nesting;
	rcu_dynticks_nesting++;
	WARN_ON_ONCE(rcu_dynticks_nesting == 0);
	rcu_idle_exit_common(oldval);
	local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(rcu_irq_enter);

#if defined(CONFIG_DEBUG_LOCK_ALLOC) || defined(CONFIG_RCU_TRACE)

/*
 * Test whether RCU thinks that the current CPU is idle.
 */
bool notrace __rcu_is_watching(void)
{
	return rcu_dynticks_nesting;
}
EXPORT_SYMBOL(__rcu_is_watching);

#endif /* defined(CONFIG_DEBUG_LOCK_ALLOC) || defined(CONFIG_RCU_TRACE) */

/*
 * Test whether the current CPU was interrupted from idle.  Nested
 * interrupts don't count, we must be running at the first interrupt
 * level.
 */
static int rcu_is_cpu_rrupt_from_idle(void)
{
	return rcu_dynticks_nesting <= 1;
}

/*
 * Helper function for rcu_sched_qs() and rcu_bh_qs().
 * Also irqs are disabled to avoid confusion due to interrupt handlers
 * invoking call_rcu().
 */
static int rcu_qsctr_help(struct rcu_ctrlblk *rcp)
{
	RCU_TRACE(reset_cpu_stall_ticks(rcp));
	if (rcp->rcucblist != NULL && rcp->donetail != rcp->curtail) {
		if (!__covered8) {__covered8 = 1; total_covered += 1;}
		rcp->donetail = rcp->curtail;
		return 1;
	}

	return 0;
}

/*
 * Record an rcu quiescent state.  And an rcu_bh quiescent state while we
 * are at it, given that any rcu quiescent state is also an rcu_bh
 * quiescent state.  Use "+" instead of "||" to defeat short circuiting.
 */
void rcu_sched_qs(void)
{
	unsigned long flags;

	local_irq_save(flags);
	if (rcu_qsctr_help(&rcu_sched_ctrlblk) + rcu_qsctr_help(&rcu_bh_ctrlblk)) {
		if (!__covered9) {__covered9 = 1; total_covered += 1;}
		raise_softirq(RCU_SOFTIRQ); 
	}
	local_irq_restore(flags);
}

/*
 * Record an rcu_bh quiescent state.
 */
void rcu_bh_qs(void)
{
	unsigned long flags;

	local_irq_save(flags);
	if (rcu_qsctr_help(&rcu_bh_ctrlblk)) {
		if (!__covered10) {__covered10 = 1; total_covered += 1;}
		raise_softirq(RCU_SOFTIRQ);
	}
	local_irq_restore(flags);
}

/*
 * Check to see if the scheduling-clock interrupt came from an extended
 * quiescent state, and, if so, tell RCU about it.  This function must
 * be called from hardirq context.  It is normally called from the
 * scheduling-clock interrupt.
 */
void rcu_check_callbacks(int user)
{
	RCU_TRACE(check_cpu_stalls());
	if (user || rcu_is_cpu_rrupt_from_idle()) {
		if (!__covered11) {__covered11 = 1; total_covered += 1;}
		rcu_sched_qs();
	}
	else if (!in_softirq()) {
		if (!__covered12) {__covered12 = 1; total_covered += 1;}
		rcu_bh_qs();
	}
	if (user) {
		if (!__covered13) {__covered13 = 1; total_covered += 1;}
		rcu_note_voluntary_context_switch(current);
	}
}

/*
 * Invoke the RCU callbacks on the specified rcu_ctrlkblk structure
 * whose grace period has elapsed.
 */
static void __rcu_process_callbacks(struct rcu_ctrlblk *rcp)
{
	const char *rn = NULL;
	struct rcu_head *next, *list;
	unsigned long flags;
	RCU_TRACE(int cb_count = 0);

	/* If no RCU callbacks ready to invoke, just return. */
	if (&rcp->rcucblist == rcp->donetail) {
		if (!__covered14) {__covered14 = 1; total_covered += 1;}
		RCU_TRACE(trace_rcu_batch_start(rcp->name, 0, 0, -1));
		RCU_TRACE(trace_rcu_batch_end(rcp->name, 0,
!!ACCESS_ONCE(rcp->rcucblist),
need_resched(),
is_idle_task(current),
false));
		return;
	}

	/* Move the ready-to-invoke callbacks to a local list. */
	local_irq_save(flags);
	RCU_TRACE(trace_rcu_batch_start(rcp->name, 0, rcp->qlen, -1));
	list = rcp->rcucblist;
	rcp->rcucblist = *rcp->donetail;
	*rcp->donetail = NULL;
	if (rcp->curtail == rcp->donetail) {
		if (!__covered15) {__covered15 = 1; total_covered += 1;}
		rcp->curtail = &rcp->rcucblist;
	}
	rcp->donetail = &rcp->rcucblist;
	local_irq_restore(flags);

	/* Invoke the callbacks on the local list. */
	RCU_TRACE(rn = rcp->name);
	while (list) {
		if (!__covered16) {__covered16 = 1; total_covered += 1;}
		next = list->next;
		prefetch(next);
		debug_rcu_head_unqueue(list);
		local_bh_disable();
		__rcu_reclaim(rn, list);
		local_bh_enable();
		list = next;
		RCU_TRACE(cb_count++);
	}
	RCU_TRACE(rcu_trace_sub_qlen(rcp, cb_count));
	RCU_TRACE(trace_rcu_batch_end(rcp->name,
cb_count, 0, need_resched(),
is_idle_task(current),
false));
}

static void rcu_process_callbacks(struct softirq_action *unused)
{
	__rcu_process_callbacks(&rcu_sched_ctrlblk);
	__rcu_process_callbacks(&rcu_bh_ctrlblk);
}

/*
 * Wait for a grace period to elapse.  But it is illegal to invoke
 * synchronize_sched() from within an RCU read-side critical section.
 * Therefore, any legal call to synchronize_sched() is a quiescent
 * state, and so on a UP system, synchronize_sched() need do nothing.
 * Ditto for synchronize_rcu_bh().  (But Lai Jiangshan points out the
 * benefits of doing might_sleep() to reduce latency.)
 *
 * Cool, huh?  (Due to Josh Triplett.)
 *
 * But we want to make this a static inline later.  The cond_resched()
 * currently makes this problematic.
 */
void synchronize_sched(void)
{
	rcu_lockdep_assert(!lock_is_held(&rcu_bh_lock_map) &&
!lock_is_held(&rcu_lock_map) &&
!lock_is_held(&rcu_sched_lock_map),
"Illegal synchronize_sched() in RCU read-side critical section");
	cond_resched();
}
EXPORT_SYMBOL_GPL(synchronize_sched);

/*
 * Helper function for call_rcu() and call_rcu_bh().
 */
static void __call_rcu(struct rcu_head *head,
void (*func)(struct rcu_head *rcu),
struct rcu_ctrlblk *rcp)
{
	unsigned long flags;

	debug_rcu_head_queue(head);
	head->func = func;
	head->next = NULL;

	local_irq_save(flags);
	*rcp->curtail = head;
	rcp->curtail = &head->next;
	RCU_TRACE(rcp->qlen++);
	local_irq_restore(flags);
}

/*
 * Post an RCU callback to be invoked after the end of an RCU-sched grace
 * period.  But since we have but one CPU, that would be after any
 * quiescent state.
 */
void call_rcu_sched(struct rcu_head *head, void (*func)(struct rcu_head *rcu))
{
	__call_rcu(head, func, &rcu_sched_ctrlblk);
}
EXPORT_SYMBOL_GPL(call_rcu_sched);

/*
 * Post an RCU bottom-half callback to be invoked after any subsequent
 * quiescent state.
 */
void call_rcu_bh(struct rcu_head *head, void (*func)(struct rcu_head *rcu))
{
	__call_rcu(head, func, &rcu_bh_ctrlblk);
}
EXPORT_SYMBOL_GPL(call_rcu_bh);

void __init rcu_init(void)
{
	open_softirq(RCU_SOFTIRQ, rcu_process_callbacks);

	rcu_early_boot_tests();
}
