
#ifndef _B_SEMAPHORE_H
#define _B_SEMAPHORE_H

#include <semaphore.h>

class Semaphore
{
public:
	Semaphore();
	~Semaphore();
	
	void Wait(void);
	bool Wait(int timeout_ms);
	bool TryWait(void);
	
	void Signal(void);
	void Clear();
	
private:
	sem_t sem;
};

#endif
