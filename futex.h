#define _FUTEX_H_
#ifndef _COMMON_H_
#include <stdint.h>
#endif

#include <linux/futex.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#include <err.h>

typedef struct
{
	uint32_t value;
} futex_t;

static inline int futex(uint32_t* address, int op, uint32_t val, const struct timespec* timeout, uint32_t address2, uint32_t val3)
{
	return syscall(SYS_futex, address, op, val, timeout, address2, val3);
}

void fwait(futex_t* pfutex);
void fpost(futex_t* pfutex);
