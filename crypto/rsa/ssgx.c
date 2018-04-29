#include "ssgx.h"

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

int local_sched_policy_priority_save(int new_policy, int new_priority, int* policy, int* priority) {
	//backup old policy and priority
	struct sched_param sched;
	int ret = pthread_getschedparam(pthread_self(), policy, &sched);
	if(ret != 0) printf("%s\n", strerror(errno));
	assert(ret == 0);
	*priority = sched.sched_priority;

	//set new policy and priority
	struct sched_param new_sched;
	new_sched.sched_priority = new_priority;
	ret = pthread_setschedparam(pthread_self(), new_policy, &new_sched);
	if(ret != 0) printf("%s\n", strerror(errno));
	assert(ret == 0);

	return 0;
}
int local_sched_policy_priority_restore(int policy, int priority) {
	struct sched_param sched;
	sched.sched_priority = priority;
	int ret = pthread_setschedparam(pthread_self(), policy, &sched);
	if(ret != 0) printf("%s\n", strerror(errno));
	assert(ret == 0);

	return 0;
}

int test() {
	int policy;
	int priority;
	int new_policy = SCHED_RR;
	int new_priority = sched_get_priority_max(new_policy);
	printf("Priority for '%d', max: %d, min: %d\n", new_policy, sched_get_priority_max(new_policy), sched_get_priority_min(new_policy));
	show_thread_policy_and_priority();
	int ret = local_sched_policy_priority_save(new_policy, new_priority, &policy, &priority);
	assert(ret == 0);
	show_thread_policy_and_priority();
	ret = local_sched_policy_priority_restore(policy, priority);
	assert(ret == 0);
	show_thread_policy_and_priority();

	return 0;
}
