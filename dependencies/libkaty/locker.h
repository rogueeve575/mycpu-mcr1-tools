
#ifndef _KT_LOCKER_H
#define _KT_LOCKER_H

#include <pthread.h>

class Locker
{
public:
	Locker();
	~Locker();
	
	void Lock();
	void Unlock();

private:
	pthread_mutex_t mutex;
};


class AutoLocker
{
public:
	AutoLocker(Locker *l) {
		_lk = l;
		l->Lock();
	}
	AutoLocker(Locker &l) {
		_lk = &l;
		l.Lock();
	}
	
	~AutoLocker() {
		_lk->Unlock();
	}

private:
	Locker *_lk;
};


#endif
