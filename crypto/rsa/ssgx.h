#ifndef __SSGX_H__
#define __SSGX_H__


int local_sched_policy_priority_save(int new_policy, int new_priority, int* policy, int* priority);
int local_sched_policy_priority_restore(int policy, int priority);

#ifdef SSGX_DEBUG
void show_thread_policy_and_priority();
#endif	//! SSGX_DEBUG

#endif	//!__SSGX_H__
