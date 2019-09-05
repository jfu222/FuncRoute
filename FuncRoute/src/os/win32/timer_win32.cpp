#if defined(_WIN32) || defined(_WIN64)

#include "../timer.h"


long long time_get_tick()
{
	LARGE_INTEGER t1;

	QueryPerformanceCounter(&t1); //int take_time = (((time2.QuadPart - time1.QuadPart) * 1000) / freq.QuadPart); //单位：ms毫秒
	return t1.QuadPart;
}


long long time_get_frequency()
{
	LARGE_INTEGER t1;

	QueryPerformanceFrequency(&t1); //获取每秒多少CPU Performance Tick
	return t1.QuadPart;
}


unsigned long long rdtsc()
{
	return __rdtsc(); //多核情况下可能计时不准
}

#endif // #if defined(_WIN32) || defined(_WIN64)
