#ifndef __SSGX_H__
#define __SSGX_H__

#define CORES_PER_CPU 16	//we allow 16 logical cores on one CPU
#define CACHE_LINE_SIZE  64

typedef struct ssgx_param {
	int policy;
	int priority;
	volatile int exit;
	volatile unsigned char challenge; //!!! MUST using volatile, otherwise threads CANNOT sync the latest value !!!
	volatile unsigned char state[CORES_PER_CPU][CACHE_LINE_SIZE]; //!!! MUST using volatile, otherwise threads CANNOT sync the latest value !!!
} ssgx_param;
//!!! Do NOT recursive call them !!!
int ssgx_save(ssgx_param* param);
int ssgx_restore(ssgx_param* param);

#endif	//!__SSGX_H__
