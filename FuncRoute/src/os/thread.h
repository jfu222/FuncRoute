//该文档参考了Intel Media SDK     [fuj & 2017.04.26]

#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdint.h>

#ifdef _WIN32
	#define C_CDECL __cdecl
	#define C_STDCALL __stdcall
#else
	#define C_CDECL
	#define C_STDCALL
#endif //_WIN32


typedef unsigned int (C_STDCALL * thread_callback)(void *);


#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <process.h>


struct MutexHandle
{
	CRITICAL_SECTION m_cs;
};


struct SemaphoreHandle
{
	void *m_semaphore;
};


struct EventHandle
{
	void *m_event;
};


struct ThreadHandle
{
	void *m_thread;
};

#else // #if defined(_WIN32) || defined(_WIN64)

#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

typedef unsigned char       BYTE;


struct MutexHandle
{
	pthread_mutex_t m_mutex_pthread;
};


struct SemaphoreHandle
{
	SemaphoreHandle()
	{
		m_count = 0;
//		m_semaphore = 0;
//		m_mutex = 0;
	}

	SemaphoreHandle(uint32_t count)
	{
		m_count = count;
//		m_semaphore = 0;
//		m_mutex = 0;
	}

	~SemaphoreHandle()
	{

	}

	uint32_t m_count;
	pthread_cond_t m_semaphore;
	pthread_mutex_t m_mutex;
};


struct EventHandle
{
	EventHandle()
	{
		m_manual = false;
		m_state = false;
	}

	EventHandle(bool manual, bool state)
	{
		m_manual = manual;
		m_state = state;
	}

	~EventHandle()
	{

	}

	bool m_manual;
	bool m_state;
	pthread_cond_t m_event;
	pthread_mutex_t m_mutex;
};


struct ThreadHandle
{
	ThreadHandle()
	{
		m_func = 0;
		m_arg = 0;
		m_thread = 0;
	}

	ThreadHandle(thread_callback func, void *arg)
	{
		m_func = func;
		m_arg = arg;
		m_thread = 0;
	}

	~ThreadHandle()
	{

	}

	thread_callback m_func;
	void *m_arg;
	pthread_t m_thread;
};

#endif // #if defined(_WIN32) || defined(_WIN64)


//------------------------------------------
class CMutex : public MutexHandle
{
public:
	CMutex();
	~CMutex();

	int Lock();
	int Unlock();
	int Try();
};


//------------------------------------------
class CAutoMutex
{
public:
	CMutex & m_mutex;
	bool m_bLocked;

public:
//	CAutoMutex();
	CAutoMutex(CMutex &mutex);
	~CAutoMutex();

	int Lock();
	int Unlock();
};


//------------------------------------------
class CSemaphore : public SemaphoreHandle
{
public:
	CSemaphore();
	CSemaphore(int &ret, uint32_t count = 0);
	~CSemaphore();

	int Post();
	int Wait();
};


//------------------------------------------
class CEvent : public EventHandle
{
public:
	CEvent();
	CEvent(int &ret, bool manual, bool state);
	~CEvent();

	int Init(bool manual, bool state);
	int Signal();
	int Reset();
	int Wait();
	int TimedWait(uint32_t msec);
};


//------------------------------------------
class CThread : public ThreadHandle
{
public:
	CMutex m_mutex;
	CEvent m_event; //for suspend thread
	bool m_is_thread_end; //用户发送一个线程结束的信号，但是线程可能还要运行一段时间才死亡
	bool m_is_thread_dead; //线程已经死亡
	unsigned long m_thread_id;
	int m_suspend_count;

public:
	CThread();
	CThread(int &ret, thread_callback func, void *arg);
	~CThread();

	int Begin(thread_callback func, void *arg);
	int End();
	bool IsEnd();
	int Dead();
	bool IsDead();
	int Wait();
	int TimedWait(uint32_t msec);
	int GetExitCode();
	unsigned long GetThreadId();
	int Suspend();
	int Resume();
	bool IsSuspended();

#if !defined(_WIN32) && !defined(_WIN64)
	friend void *thread_start(void *arg);
#endif
};


//--------------原子递增/减操作----------------------------
uint16_t atomic_inc16(volatile uint16_t *pVariable); //Thread-safe 16-bit variable incrementing
uint16_t atomic_dec16(volatile uint16_t *pVariable); //Thread-safe 16-bit variable decrementing


#endif //__THREAD_H__
