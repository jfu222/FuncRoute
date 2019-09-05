#if defined(_WIN32) || defined(_WIN64)

#include "../thread.h"
#include <new>
#include <stdio.h>


//-------------------------------------------------------
CMutex::CMutex()
{
	InitializeCriticalSection(&m_cs);
}


CMutex::~CMutex()
{
	Unlock();
	DeleteCriticalSection(&m_cs);
}


int CMutex::Lock()
{
	EnterCriticalSection(&m_cs);
	return 0;
}


int CMutex::Unlock()
{
	LeaveCriticalSection(&m_cs);
	return 0;
}


int CMutex::Try()
{
	return TryEnterCriticalSection(&m_cs);
}


//-------------------------------------------------------
CAutoMutex::CAutoMutex(CMutex &mutex) : m_mutex(mutex)
{
	m_bLocked = false;
	if(Lock() != 0){throw std::bad_alloc();}
}


CAutoMutex::~CAutoMutex()
{
	Unlock();
}


int CAutoMutex::Lock()
{
	int ret = 0;
	if(!m_bLocked)
	{
		if(!m_mutex.Try())
		{
			ret = m_mutex.Lock();
		}
		m_bLocked = true;
	}
	return ret;
}


int CAutoMutex::Unlock()
{
	int ret = 0;
	if(m_bLocked)
	{
		ret = m_mutex.Unlock();
		m_bLocked = false;
	}
	return ret;
}


//-------------------------------------------------------
CSemaphore::CSemaphore()
{
	m_semaphore = NULL;
}


CSemaphore::CSemaphore(int &ret, uint32_t count)
{
	ret = 0;
	m_semaphore = CreateSemaphoreA(NULL, count, LONG_MAX, 0);
	if(!m_semaphore){throw std::bad_alloc();}
}


CSemaphore::~CSemaphore()
{
	CloseHandle(m_semaphore);
}


int CSemaphore::Post()
{
	return (ReleaseSemaphore(m_semaphore, 1, NULL) == false) ? -1 : 0;
}


int CSemaphore::Wait()
{
	return (WaitForSingleObject(m_semaphore, INFINITE) != WAIT_OBJECT_0) ? -1 : 0;
}


//-------------------------------------------------------
CEvent::CEvent()
{
	m_event = NULL;
}


CEvent::CEvent(int &ret, bool manual, bool state)
{
	ret = 0;
	m_event = CreateEventA(NULL, manual, state, NULL);
	if(!m_event){throw std::bad_alloc();}
}


CEvent::~CEvent()
{
	if(m_event)
	{
		Signal();
		CloseHandle(m_event);
		m_event = NULL;
	}
}


int CEvent::Init(bool manual, bool state)
{
	m_event = CreateEventA(NULL, manual, state, NULL); //manual=TRUE时，需要调用ResetEvent来手动复位到无信号状态。state=TRUE时，表示初始状态为有信号状态。
	return (m_event) ? 0 : -1;
}


int CEvent::Signal()
{
	return (SetEvent(m_event) == false) ? -1 : 0;
}


int CEvent::Reset()
{
	return (ResetEvent(m_event) == false) ? -1 : 0;
}


int CEvent::Wait()
{
	return (WaitForSingleObject(m_event, INFINITE) != WAIT_OBJECT_0) ? -1 : 0;
}


int CEvent::TimedWait(uint32_t msec)
{
	if(msec == 0xFFFFFFFF){return -1;}
	int ret = -3;
	DWORD res = WaitForSingleObject(m_event, msec);

	if(res == WAIT_OBJECT_0)
	{
		ret = 0;
	}else if (res == WAIT_TIMEOUT)
	{
		ret = -4;
	}else
	{
		ret = -1;
	}

	return ret;
}


//-------------------------------------------------------
CThread::CThread()
{
	m_thread = NULL;
	m_is_thread_end = false;
	m_is_thread_dead = false;
	m_thread_id = 0;
	m_suspend_count = 0;
}


CThread::CThread(int &ret, thread_callback func, void *arg)
{
	ret = 0;

	unsigned int thread_id = 0;

	m_thread = (void *)_beginthreadex(NULL, 0, func, arg, 0, &thread_id);
	if(!m_thread){throw std::bad_alloc();}

	m_thread_id = thread_id;
}


CThread::~CThread()
{
	if(m_thread)
	{
		CloseHandle(m_thread);
		m_thread = NULL;
	}
}


int CThread::Begin(thread_callback func, void *arg)
{
	m_mutex.Lock();
	m_is_thread_end = false;
	m_is_thread_dead = false;
	m_mutex.Unlock();
	
	unsigned int thread_id = 0;

	m_thread = (void *)_beginthreadex(NULL, 0, func, arg, 0, &thread_id);
	if(!m_thread){return -1;}

	m_thread_id = thread_id;

	return 0;
}


int CThread::End()
{
//	m_mutex.Lock();
	m_is_thread_end = true;
//	m_mutex.Unlock();

	return 0;
}


bool CThread::IsEnd()
{
//	bool is_end = false;

//	m_mutex.Lock();
//	is_end = m_is_thread_end;
//	m_mutex.Unlock();

	return m_is_thread_end;
}


int CThread::Dead()
{
	m_is_thread_dead = true;
	return 0;
}


bool CThread::IsDead()
{
	return m_is_thread_dead;
}


int CThread::Wait()
{
	return (WaitForSingleObject(m_thread, INFINITE) != WAIT_OBJECT_0) ? -1 : 0;
}


int CThread::TimedWait(uint32_t msec)
{
	if(msec == 0xFFFFFFFF){return -1;}

	int ret = 0;
	DWORD res = WaitForSingleObject(m_thread, msec);
	
	if(res == WAIT_OBJECT_0)
	{
		ret = 0;
	}else if (res == WAIT_TIMEOUT)
	{
		ret = -4;
	}else
	{
		ret = -1;
	}

	return ret;
}


int CThread::GetExitCode()
{
	int ret = -1;

	DWORD code = 0;
	int ret2 = 0;
	ret2 = GetExitCodeThread(m_thread, &code);

	if(ret2 == 0)
	{
		ret = -1;
	}else if(code == STILL_ACTIVE)
	{
		ret = -4;
	}else
	{
		ret = 0;
	}

	return ret;
}


unsigned long CThread::GetThreadId()
{
	return m_thread_id;
}


int CThread::Suspend()
{
	m_mutex.Lock();
	int previous_suspend_count = m_suspend_count;
	m_suspend_count++;
	m_mutex.Unlock();

//	DWORD ret = SuspendThread(m_thread);
//	if(ret == -1){return -1;}

	return previous_suspend_count;
}


int CThread::Resume()
{
	m_mutex.Lock();
	int previous_suspend_count = m_suspend_count;
	m_suspend_count--;
	m_mutex.Unlock();

	m_event.Signal();

//	DWORD ret = ResumeThread(m_thread);
//	if(ret == -1){return -1;}

	return previous_suspend_count;
}


bool CThread::IsSuspended()
{
	bool is_suspended = false;

//	m_mutex.Lock();
	is_suspended = (m_suspend_count > 0) ? true : false;
//	m_mutex.Unlock();

	return is_suspended;
}


//-------------------------------------------------------
#include <intrin.h>

//#pragma intrinsic(_InterlockedIncrement16) //生成内含函数
//#pragma intrinsic(_InterlockedDecrement16)

uint16_t atomic_inc16(volatile uint16_t *pVariable)
{
	return _InterlockedIncrement16((volatile short*)pVariable);
}


uint16_t atomic_dec16(volatile uint16_t *pVariable)
{
	return _InterlockedDecrement16((volatile short*)pVariable);
}


#endif // #if defined(_WIN32) || defined(_WIN64)
