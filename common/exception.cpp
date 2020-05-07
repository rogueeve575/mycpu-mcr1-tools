
#include <katy/katy.h>
#include <execinfo.h>
#include "exception.h"
#include <cxxabi.h>

#define BACKTRACE_MAX_LENGTH	24

MyException::MyException(const char *function, const char *file, int line, const char *msg)
{
	char *functemp = strdup(function);
	char *funcUse = functemp;
	char *ptr = strchr(functemp, '(');
	if (ptr)
	{
		functemp = (char *)realloc(functemp, strlen(functemp) + 3);
		ptr[1] = ')';
		ptr[2] = 0;
	}
	
	ptr = strrchr(functemp, ' ');
	if (ptr) funcUse = ptr + 1;
	
	this->function = strdup(funcUse);
	free(functemp);
	
	this->file = strdup(file);
	this->msg = strdup(msg);
	this->line = line;
	this->rendered_msg = NULL;
	
	if (obtain_backtrace())
		backtrace_str = NULL;
}
	
MyException::~MyException()
{
	if (function) free(function);
	if (file) free(file);
	if (msg) free(msg);
	if (backtrace_str) free(backtrace_str);
	if (rendered_msg) free(rendered_msg);
}

/*
void c------------------------------() {}
*/

bool MyException::obtain_backtrace()
{
void *bt_array[BACKTRACE_MAX_LENGTH];
char **strings;

	int numEntries = backtrace(bt_array, BACKTRACE_MAX_LENGTH);
	if (!numEntries) return 1;
	
	strings = backtrace_symbols(bt_array, numEntries);
	if (!strings) return 1;
	
	bool first = true;
	DString traceOutput;
	
	for(int i=0;i<numEntries;i++)
	{
		const char *name = strings[i];
		char *demangled_name = NULL;
		
		const char *ptr = strchr(name, '(');
		if (ptr)
		{
			char *temp = strdup(ptr + 1);
			char *ptr;
			ptr = strchr(temp, '+'); if (ptr) *ptr = 0;
			ptr = strchr(temp, ')'); if (ptr) *ptr = 0;
			ptr = strchr(temp, ' '); if (ptr) *ptr = 0;
			ptr = strchr(temp, '['); if (ptr) *ptr = 0;
			
			int status;
			demangled_name = abi::__cxa_demangle(temp, NULL, NULL, &status);
			free(temp);
			
			if (demangled_name)
			{
				ptr = strchr(demangled_name, '(');
				if (ptr) *ptr = 0;
			}
		}
		
		if (i < 2 && demangled_name && strbegin(demangled_name, "MyException::"))
			continue;
		
		traceOutput.AppendString("   - ");
		traceOutput.AppendString(first ? "exception at \e[1;31m" : "called from: \e[1;37m");
		if (demangled_name)
		{
			traceOutput.AppendString(demangled_name);
			traceOutput.AppendString(first ? " \e[0;31m[" : " \e[0m[");
			traceOutput.AppendString(name);
			traceOutput.AppendString("]");
			free(demangled_name);
		}
		else
		{
			char *mangledName = strdup(name);
			char *address = strstr(mangledName, " [");
			if (address) *address = 0;
			
			traceOutput.AppendString(mangledName);
			if (address)
			{
				traceOutput.AppendString("\e[0m [");
				traceOutput.AppendString(address + 2);
			}
			
			free(mangledName);
		}
		
		traceOutput.AppendString("\e[0m\n");
		first = false;
	}
	
	backtrace_str = strdup(traceOutput.String());
	
	free(strings);
	return 0;
}

const char *MyException::Describe(bool incl_backtrace)
{
	if (!rendered_msg)
	{
		DString temp;
		temp.AppendString(stprintf("\e[1;31m%s\e0m: \e[1;37m[ %s:%d ]\e[0m: \e[1;33m%s\e[0m", \
			function, file, line, msg));
		
		if (backtrace_str && incl_backtrace)
		{
			temp.AppendString("\nBacktrace:\n");
			temp.AppendString(backtrace_str);
		}
		
		rendered_msg = strdup(temp.String());
	}
	
	return rendered_msg;
}
	
