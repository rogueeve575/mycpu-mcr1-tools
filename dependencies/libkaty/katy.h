
#ifndef _KATY_H
#define _KATY_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <math.h>

#include "misc.h"
#include "stat.h"
#include "timer.h"

#include "atree.h"
#include "qstree.h"

#include "BList.h"
#include "StringList.h"
#include "DBuffer.h"
#include "DString.h"
#include "RingBuffer.h"

#include "tcpstuff.h"
#include "udpstuff.h"

#include "ini.h"
#include "email.h"
#include "llist.h"
#include "locker.h"
#include "semaphore.h"
#include "fifo.h"
#include "autofree.h"

#ifndef safe
#define safe(str)	((str) ? (str) : "")
#endif

#ifndef SWAP
#define SWAP(A, B)	\
do	{	\
	A ^= B;	\
	B ^= A;	\
	A ^= B;	\
} while(0)
#endif

enum { READ_END, WRITE_END };	// pipe() ends

#endif
