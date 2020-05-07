
#include "katy.h"
#include "semaphore.h"
#include "semaphore.fdh"


Semaphore::Semaphore()
{
	if (sem_init(&sem, 0, 0))
		staterr("sem_init failed: %s", strerror(errno));
}

Semaphore::~Semaphore()
{
	if (sem_destroy(&sem))
		staterr("sem_destroy failed: %s", strerror(errno));
}

/*
void c------------------------------() {}
*/

void Semaphore::Wait()
{
int result;

	do {
		result = sem_wait(&sem);
	} while(result == EINTR);
	
	if (result)
		staterr("sem_wait failed: %s", strerror(errno));
}

// returns 0 if semaphore successfully decremented
bool Semaphore::Wait(int timeout_ms)
{
int result;

	const uint64_t MS_TO_NS = 1000000;
	const uint64_t ONE_SECOND = 1000000 * 1000;
	struct timespec expireTime = { 0, 0 };
	
	clock_gettime(CLOCK_REALTIME, &expireTime);
	expireTime.tv_sec += timeout_ms / 1000;
	expireTime.tv_nsec += ((uint64_t)(timeout_ms % 1000)) * MS_TO_NS;
	if (expireTime.tv_nsec >= ONE_SECOND) {
		expireTime.tv_sec += (expireTime.tv_nsec / ONE_SECOND);
		expireTime.tv_nsec %= ONE_SECOND;
	}

	do {
		result = sem_timedwait(&sem, &expireTime);
	} while(result == EINTR);
	
	if (result && result != ETIMEDOUT)
		staterr("sem_timedwait failed: %s", strerror(errno));
	
	return (result != 0);
}

// returns 0 if semaphore successfully decremented
bool Semaphore::TryWait()
{
int result;
	
	do {
		result = sem_trywait(&sem);
	} while(result == EINTR);
	
	if (result == EAGAIN)
		return 1;
	
	if (result)
		staterr("sem_wait failed: %s", strerror(errno));
	
	return (result != 0);
}

/*
void c------------------------------() {}
*/

void Semaphore::Signal()
{
	sem_post(&sem);
}

void Semaphore::Clear()
{
	while(TryWait()) ;
}
