
#ifndef _TOKEN_H_
#define _TOKEN_H_

#define TKN_STRING		0x00		// regular string

#define TKN_SRCFILE		0x01		// %SRCFILE% (main.c)
#define TKN_MODULE		0x02		// %MODULE% (main)
#define TKN_OUTPUT		0x03		// %OUTPUT% (test.exe)
#define TKN_OUTPUTMODULE 0x04		// %OUTPUTMODULE% (test)
#define TKN_CMDFILE		0x05		// %CMDFILE% (makegenxx.cmd, MSVC oddity)
#define TKN_OBJ_EXT		0x06		// default '.o'
#define TKN_H_EXT		0x07		// default '.h'
#define TKN_FDH_EXT		0x08		// default '.fdh'
#define TKN_MLNAME		0x09		// default 'makelist.ml'
#define TKN_MAKEFILE	0x0A		// default 'Makefile'
#define TKN_CC		0x0B		// C compiler
#define TKN_HOSTFLAGS	0x0C		// any host-specific flags we may want to add

typedef struct token
{
	uchar type;
	char *str;
	struct token *next;
} token;


#endif
