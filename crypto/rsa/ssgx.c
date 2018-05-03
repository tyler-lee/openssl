#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/sysinfo.h>	//for get_nprocs()
#ifndef _GNU_SOURCE
#define _GNU_SOURCE	//For CPU_SET, CPU_ZERO, pthread_setaffinity_np
#endif
#ifndef __USE_GNU
#define __USE_GNU	//For CPU_SET, CPU_ZERO, pthread_setaffinity_np
#endif
#include <sched.h>
#include <pthread.h>

#include "ssgx.h"
/*#define SSGX_DEBUG*/

void print_policy_string(int policy) {
	switch (policy)
	{
		case SCHED_FIFO:
			printf ("policy= SCHED_FIFO");
			break;
		case SCHED_RR:
			printf ("policy= SCHED_RR");
			break;
		case SCHED_OTHER:
			printf ("policy= SCHED_OTHER");
			break;
		default:
			printf ("policy= UNKNOWN");
			break;
	}
}
void show_thread_policy_and_priority() {
	int policy;
	struct sched_param sched;

	int ret = pthread_getschedparam(pthread_self(), &policy, &sched);
	if(ret != 0) printf("%s\n", strerror(errno));
	assert(ret == 0);

	printf("Thread %ld: ", pthread_self());
	print_policy_string(policy);
	printf (", priority= %d\n", sched.sched_priority);
}
void set_thread_policy_and_priority(int policy, int priority) {
	struct sched_param sched;
	sched.sched_priority = priority;
	int ret = pthread_setschedparam(pthread_self(), policy, &sched);
	if(ret != 0) printf("%s\n", strerror(errno));
	assert(ret == 0);

	printf ("Set thread %ld priority to %d\n", pthread_self(), priority);
}

typedef struct thread_info {
	int cpu;
	pthread_t tid;
	ssgx_param* param;
} thread_info;

void* pthread_seize_core(void* arg) {
	thread_info* info = (thread_info*)arg;
	int cpu = info->cpu;
	ssgx_param* param = info->param;
#ifdef SSGX_DEBUG
	printf("Enter core %d\n", cpu);
#endif	//! SSGX_DEBUG

	//set thread affinity
    /*cpu_set_t mask;*/
    /*CPU_ZERO(&mask);*/
    /*CPU_SET(cpu, &mask);*/
	/*int ret = pthread_setaffinity_np(info->tid, sizeof(mask), &mask);*/
	/*assert(ret == 0);*/
	//show_thread_policy_and_priority();

	do {
		param->state[cpu][0] = param->challenge;
		/*printf("Challenge: %d, state[%d]: %d\n", param->challenge, cpu, param->state[cpu][0]);*/
	} while (param->exit == 0);
#ifdef SSGX_DEBUG
	printf("Exit core %d\n", cpu);
#endif	//! SSGX_DEBUG

	pthread_exit((void*)0);
}

int ssgx_save(ssgx_param* param)
{
	memset(param, 0, sizeof(ssgx_param));

	//backup schedule policy and priority
	struct sched_param sched;
	int ret = pthread_getschedparam(pthread_self(), &(param->policy), &sched);
	assert(ret == 0);
	param->priority = sched.sched_priority;

#ifdef SSGX_DEBUG
	printf("================ Before Set ==============\n");
	show_thread_policy_and_priority();
#endif	//! SSGX_DEBUG

	//set new policy and priority
	/*int new_policy = SCHED_FIFO;*/
	int new_policy = SCHED_RR;
	struct sched_param new_sched;
	int new_priority = sched_get_priority_max(new_policy);
	assert(new_priority != -1);
	new_sched.sched_priority = new_priority;
	ret = pthread_setschedparam(pthread_self(), new_policy, &new_sched);
	assert(ret == 0);

#ifdef SSGX_DEBUG
	printf("================ After Set ==============\n");
	show_thread_policy_and_priority();
#endif	//! SSGX_DEBUG


	//occupy logical CPU cores with N-1 thread
	param->exit = 0;
	int cores = get_nprocs();
	pthread_attr_t attr;
	ret = pthread_attr_init(&attr);
	assert(ret == 0);
	/*ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);*/
	/*assert(ret == 0);*/
	ret = pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
	assert(ret == 0);
	thread_info info[CORES_PER_CPU];
	for(int i=1; i<cores; i++) {
		info[i].cpu = i;
		info[i].param = param;
		ret = pthread_create(&(info[i].tid), &attr, pthread_seize_core, &info[i]);
		assert(ret == 0);
	}
#ifdef SSGX_DEBUG
	printf("Successfully create dummy threads\n");
#endif	//! SSGX_DEBUG
	ret = pthread_attr_destroy(&attr);
	assert(ret == 0);

	//check dummy threads
	srand(time(NULL));
	param->challenge = rand() & 0xFF;
	int limit = 4000;
#if 1
	int all_online = 1;
	int count = 0;
	while(++count) {
		all_online = 1;
		for(int i=1; i<cores; ++i) {
			if(param->challenge != param->state[i][0]) {all_online=0;break;}
		}
		if(all_online == 1) break;
	}
	if(count > limit) {
		printf("Exceed time limit: %d (limit: %d)\n", count, limit);
		abort();
	}
#else
	while (limit-- > 0) {
		int all_online = 1;
		for(int i=1; i<cores; ++i) {
			if(param->challenge != param->state[i][0]) {all_online=0;break;}
		}
		if(all_online == 1) break;
	}
#ifdef SSGX_DEBUG
	printf("Left times: %d, exit: %d\n", limit, param->exit);
#endif	//! SSGX_DEBUG
	if(limit < 0) {
		//TODO: raise an alarm
		printf("Check dummy threads fail\n");
		abort();
	}
#endif

	//TODO: do extra check, i.e., AEX detection

	return 0;
}
int ssgx_restore(ssgx_param* param)
{
	//send dummy threads exit signal
	param->exit = 1;
#ifdef SSGX_DEBUG
	printf("Send exit signal\n");
#endif	//! SSGX_DEBUG


	//retore schdule policy and priority
	struct sched_param sched;
	sched.sched_priority = param->priority;
	int ret = pthread_setschedparam(pthread_self(), param->policy, &sched);
	assert(ret == 0);

#ifdef SSGX_DEBUG
	printf("================ After Restore ==============\n");
	show_thread_policy_and_priority();
#endif	//! SSGX_DEBUG

	return 0;
}


#if 0
#include <unistd.h>
int main() {
	ssgx_param param;
	ssgx_save(&param);
	/*while(1) printf("in main loop");*/
	/*printf("secure world\n");*/
	/*sleep(20);*/
	ssgx_restore(&param);
	/*printf("normal world\n");*/

	return 0;
}
#endif
