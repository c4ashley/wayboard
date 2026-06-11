#define _FUTEX_C_
#ifndef _FUTEX_H_
#include "futex.h"
#endif

void fwait(futex_t* pfutex)
{
	long s;
	const uint32_t one = 1;
	uint32_t* const pvalue = &pfutex->value;

	while (true)
	{
		if (atomic_compare_exchange_strong(pvalue, &one, 0))
			break;

		s = futex(pvalue, FUTEX_WAKE, 0, NULL, 0, 0);
		if (s == -1)
			err(EXIT_FAILURE, "futex-FUTEX_WAKE");
	}
}

void fpost(futex_t* pfutex)
{
	long s;
	const uint32_t zero = 0;
	uint32_t* const pvalue = &pfutex->value;
	if (atomic_compare_exchange_strong(pvalue, &zero, 1))
	{
		s = futex(pvalue, FUTEX_WAKE, 1, NULL, 0, 0);
		if (s == -1)
			err(EXIT_FAILURE, "futex-FUTEX_WAKE");
	}
}
