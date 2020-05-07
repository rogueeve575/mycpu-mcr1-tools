
#ifndef _COMMON_H
#define _COMMON_H

#include <katy/katy.h>

#define NUM_OUTPUT_PINS		32
#define NUM_INPUT_PINS		19

// this type is used to hold a set of output pins (bytecode)
// it's also abused to hold a ROM address (i.e. set of input pins)
// therefore, it should be of a type having equal or more bits to both NUM_OUTPUT_PINS and NUM_INPUT_PINS
typedef uint32_t	romword_t;

#ifdef DEBUG
	#define dstat	stat
#else
	#define dstat(...)		do { } while(0)
#endif

#include "hash.h"
#include "scanner.h"
#include "tokenizer.h"
#include "controls.h"
#include "alu.h"
#include "pp.h"

#include "rpnparser.h"

#endif
