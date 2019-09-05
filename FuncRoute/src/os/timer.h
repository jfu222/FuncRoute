#ifndef __TIMER_H__
#define __TIMER_H__


#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

#define TIME_SLEEP(msec) Sleep(msec)

#define TIME_USLEEP(usec) \
{ \
    LARGE_INTEGER due; \
    due.QuadPart = -(10*(int)usec); \
    HANDLE t = CreateWaitableTimer(NULL, TRUE, NULL); \
    SetWaitableTimer(t, &due, 0, NULL, NULL, 0); \
    WaitForSingleObject(t, INFINITE); \
    CloseHandle(t); \
}

#else // #if defined(_WIN32) || defined(_WIN64)

#include <unistd.h>

#define TIME_SLEEP(msec) \
  do { \
    usleep(1000*msec); \
  } while(0)

#define TIME_USLEEP(usec) \
  do { \
    usleep(usec); \
  } while(0)

#endif // #if defined(_WIN32) || defined(_WIN64)

#define TIME_GET_TIME(T,S,F) ((double)((T)-(S))/(double)(F))


long long time_get_tick();
long long time_get_frequency();
unsigned long long rdtsc();

#endif //__TIMER_H__
