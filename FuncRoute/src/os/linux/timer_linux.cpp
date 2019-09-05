#if !defined(_WIN32) && !defined(_WIN64)

#include "../timer.h"
#include <sys/time.h>

#define MSDK_TIME_MHZ 1000000

long long time_get_tick()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (long long)tv.tv_sec * (long long)MSDK_TIME_MHZ + (long long)tv.tv_usec;
}


long long time_get_frequency()
{
	return (long long)MSDK_TIME_MHZ;
}


unsigned long long rdtsc()
{
	unsigned int lo,hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((unsigned long long)hi << 32) | lo;
}

#endif // #if !defined(_WIN32) && !defined(_WIN64)
