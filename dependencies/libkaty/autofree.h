
#ifndef __AUTOFREE_H
#define __AUTOFREE_H


class AutoFree
{
public:
	AutoFree(void *ptr) {
		_ptr = ptr;
	}
	
	~AutoFree() {
		free(_ptr);
	}
	
private:
	void *_ptr;
};


#endif
