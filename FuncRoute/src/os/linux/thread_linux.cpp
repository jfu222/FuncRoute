#if !defined(_WIN32) && !defined(_WIN64)

#include <new> // std::bad_alloc
#include <stdio.h> // setrlimit
#include <sched.h>

#include "../thread.h"


//-------------------------------------------------------
CMutex::CMutex()
{
	int ret = pthread_mutex_init(&m_mutex_pthread, NULL);
	if(ret){throw std::bad_alloc();}
}


CMutex::~CMutex()
{
	Unlock();
	pthread_mutex_destroy(&m_mutex_pthread);
}


int CMutex::Lock()
{
	return (pthread_mutex_lock(&m_mutex_pthread)) ? -1 : 0;
}


int CMutex::Unlock()
{
	return (pthread_mutex_unlock(&m_mutex_pthread)) ? -1 : 0;
}


int CMutex::Try()
{
	return (pthread_mutex_trylock(&m_mutex_pthread)) ? 0 : 1;
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

}


CSemaphore::CSemaphore(int &ret, uint32_t count) : SemaphoreHandle(count)
{
	ret = 0;
	int ret2 = pthread_cond_init(&m_semaphore, NULL);
	if(!ret2)
	{
		ret2 = pthread_mutex_init(&m_mutex, NULL);
		if(ret2){pthread_cond_destroy(&m_semaphore);}
	}
	if(ret2){throw std::bad_alloc();}
}


CSemaphore::~CSemaphore()
{
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_semaphore);
}


int CSemaphore::Post()
{
	int ret = pthread_mutex_lock(&m_mutex);
	if(!ret)
	{
		if(0 == m_count++){ret = pthread_cond_signal(&m_semaphore);}
	}
	int ret2 = pthread_mutex_unlock(&m_mutex);
	if(!ret){ret = ret2;}
	return (ret) ? -1 : 0;
}


int CSemaphore::Wait()
{
	int ret = pthread_mutex_lock(&m_mutex);
	if(!ret)
	{
		while(!m_count)
		{
			ret = pthread_cond_wait(&m_semaphore, &m_mutex);
		}
		if(!ret){--m_count;}
		int ret2 = pthread_mutex_unlock(&m_mutex);
		if (!ret) ret = ret2;
	}
	return (ret) ? -1 : 0;
}


//-------------------------------------------------------
CEvent::CEvent()
{

}


CEvent::CEvent(int &ret, bool manual, bool state) : EventHandle(manual, state)
{
	ret = 0;

	int ret2 = pthread_cond_init(&m_event, NULL);
	if(!ret2)
	{
		ret2 = pthread_mutex_init(&m_mutex, NULL);
		if(ret2){pthread_cond_destroy(&m_event);}
	}
	if(ret2){throw std::bad_alloc();}
}


CEvent::~CEvent()
{
	Signal();
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_event);
}


int CEvent::Init(bool manual, bool state)
{
	m_manual = manual;
	m_state = state;

	int ret2 = pthread_cond_init(&m_event, NULL);
	if(!ret2)
	{
		ret2 = pthread_mutex_init(&m_mutex, NULL);
		if(ret2){pthread_cond_destroy(&m_event);}
		return -1;
	}

	return 0;
}


int CEvent::Signal()
{
	int ret = pthread_mutex_lock(&m_mutex);
	if(!ret)
	{
		if(!m_state)
		{
			m_state = true;
			if(m_manual)
			{
				ret = pthread_cond_broadcast(&m_event);
			}else
			{
				ret = pthread_cond_signal(&m_event);
			}
		}
		int ret2 = pthread_mutex_unlock(&m_mutex);
		if(!ret){ret = ret2;}
	}
	return (ret) ? -1 : 0;
}


int CEvent::Reset()
{
	int ret = pthread_mutex_lock(&m_mutex);
	if(!ret)
	{
		if(m_state){m_state = false;}
		ret = pthread_mutex_unlock(&m_mutex);
	}
	return (ret) ? -1 : 0;
}


int CEvent::Wait()
{
	int ret = pthread_mutex_lock(&m_mutex);
	if(!ret)
	{
		while(!m_state){ret = pthread_cond_wait(&m_event, &m_mutex);}
		if(!m_manual){m_state = false;}
		int ret2 = pthread_mutex_unlock(&m_mutex);
		if (!ret){ret = ret2;}
	}
	return (ret) ? -1: 0;
}


int CEvent::TimedWait(uint32_t msec)
{
	if(msec == 0xFFFFFFFF){return -1;}
	int ret = -3;

	int ret2 = pthread_mutex_lock(&m_mutex);
	if(!ret2)
	{
		if(!m_state)
		{
			struct timeval tval;
			struct timespec tspec;
			int32_t ret3;

			gettimeofday(&tval, NULL);
			msec = 1000 * msec + tval.tv_usec;
			tspec.tv_sec = tval.tv_sec + msec / 1000000;
			tspec.tv_nsec = (msec % 1000000) * 1000;
			ret3 = pthread_cond_timedwait(&m_event, &m_mutex, &tspec);
			if(!ret3)
			{
				ret = 0;
			}else if(ret3 == ETIMEDOUT)
			{
				ret = -4;
			}else
			{
				ret = -1;
			}
		}else
		{
			ret = 0;
		}

		if(!m_manual){m_state = false;}

		ret2 = pthread_mutex_unlock(&m_mutex);
		if(ret2){ret = -1;}
	}else
	{
		ret = -1;
	}

	return ret;
}


//-------------------------------------------------------
void *thread_start(void *arg)
{
	if(arg)
	{
		CThread* thread = (CThread*)arg;

		if(thread->m_func){thread->m_func(thread->m_arg);}
		thread->m_event.Signal();
	}
	return NULL;
}


//-------------------------------------------------------
CThread::CThread()
{
	m_thread = 0;
	m_is_thread_end = false;
	m_is_thread_dead = false;
	m_thread_id = 0;
	m_suspend_count = 0;

	int ret = m_event.Init(false, false);
}


CThread::CThread(int &ret, thread_callback func, void *arg) : ThreadHandle(func, arg)
{
	ret = m_event.Init(false, false);
	if(pthread_create(&m_thread, NULL, thread_start, this))
	{
		throw std::bad_alloc();
	}

	m_thread_id = pthread_self();
}


CThread::~CThread()
{

}


int CThread::Begin(thread_callback func, void *arg)
{
	m_is_thread_end = false;
	m_is_thread_dead = false;
	m_func = func;
	m_arg = arg;
	if(pthread_create(&m_thread, NULL, thread_start, this))
	{
		return -1;
	}

	m_thread_id = m_thread; //pthread_self();

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
	int ret = pthread_join(m_thread, NULL);
	return (ret) ? -1 : 0;
}


int CThread::TimedWait(uint32_t msec)
{
	if(msec == 0xFFFFFFFF){return -1;}

	int ret = m_event.TimedWait(msec);

	if(ret == 0)
	{
		return (pthread_join(m_thread, NULL)) ? -1 : 0;
	}
	return ret;
}


int CThread::GetExitCode()
{
//	if(!m_event){return -1;}

	return 0;
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

	return previous_suspend_count;
}


int CThread::Resume()
{
	m_mutex.Lock();
	int previous_suspend_count = m_suspend_count;
	m_suspend_count--;
	m_mutex.Unlock();
	
	m_event.Signal();

	return previous_suspend_count;
}


bool CThread::IsSuspended()
{
	bool is_suspended = false;

	m_mutex.Lock();
	is_suspended = (m_suspend_count > 0) ? true : false;
	m_mutex.Unlock();

	return is_suspended;
}


//-------------------------------------------------------
static uint16_t atomic_add16(volatile uint16_t *mem, uint16_t val)
{
	asm volatile ("lock; xaddw %0,%1"
					: "=r" (val), "=m" (*mem)
					: "0" (val), "m" (*mem)
					: "memory", "cc");
	return val;
}


uint16_t atomic_inc16(volatile uint16_t *pVariable)
{
	return atomic_add16(pVariable, 1) + 1;
}


uint16_t atomic_dec16(volatile uint16_t *pVariable)
{
	return atomic_add16(pVariable, (uint16_t)-1) + 1;
}

#endif // #if !defined(_WIN32) && !defined(_WIN64)
