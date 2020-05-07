
#ifndef _KT_CONDVAR_H
#define _KT_CONDVAR_H

#include <pthread.h>

class CondVar
{
public:
	CondVar() {
		pthread_mutex_init(&mutex, NULL);
		pthread_cond_init(&cond, NULL);
	}
	
	~CondVar() {
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&mutex);
	}
	
	void Wait() {
		int orgval = counter;
		pthread_mutex_lock(&mutex);
		{
			while(counter == orgval)
				pthread_cond_wait(&cond, &mutex);
		}
		pthread_mutex_unlock(&mutex);
	}
	
	int Wait(int timeout_ms) {
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
		
		int orgval = counter;
		pthread_mutex_lock(&mutex);
		{
			while(counter == orgval)
				pthread_cond_timedwait(&cond, &mutex, &expireTime);
		}
		pthread_mutex_unlock(&mutex);
		
		return (result == ETIMEDOUT);
	}
	
	void Signal() {		// unblock up to one thread waiting on the condition
		pthread_mutex_lock(&mutex);
		{
			flag++;
			pthread_cond_signal(&cond);
		}
		pthread_mutex_unlock(&mutex);
	}
	
	void SignalAll() {	// unblock ALL threads waiting on the condition
		pthread_mutex_lock(&mutex);
		{
			flag++;
			pthread_cond_broadcast(&cond);
		}
		pthread_mutex_unlock(&mutex);
	}

private:
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int flag;
};

#endif
