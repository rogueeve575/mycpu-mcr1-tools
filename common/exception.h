
#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <stdexcept>

class MyException : public std::exception
{
public:
	MyException(const char *function, const char *file, int line, const char *msg);
	~MyException() noexcept;
	
	const char *Describe(bool incl_backtrace = false);
	
	char *function, *file, *msg;
	char *backtrace_str;
	int line;
	
private:
	bool obtain_backtrace(void);
	char *rendered_msg;
};

#define throw_exception(X)	\
	throw MyException(__PRETTY_FUNCTION__, __FILE__, __LINE__, (X))

//	throw runtime_error(stprintf("%s [ %s:%d ]: %s", __PRETTY_FUNCTION__, __FILE__, __LINE__, (X)))

#endif
