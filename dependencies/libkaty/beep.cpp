
/*
	provides sysbeep() function, which tries to make some kind of general beep sound,
	first tries to use ALSA to make the beep through the default sound card if any sound cards
	can be found by ALSA. Otherwise it looks for the "beep" program and tries to use it to
	make a beep through the PC speaker if equipped.
	
	bool sysbeep(void);
		attempts to make a "beep" sound through whatever hardware is available.
		the sound will play asynchronously with the function returning as soon as the beep starts.
		returns 1 if any failure could be detected, and we're definitely not making any sound.
		a 0 means that things didn't go awful and we might be playing the beep (but no guarantees!).
		
		this function is currently prototyped in misc.h, where it totally doesn't belong. but, do you
		really want to create a whole new header file just for one function?
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/param.h>
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include "locker.h"
#include "misc.h"
#include "beep.fdh"

#include "beepdata.inc"		// contains a short .wav file of a blip sound for piping to aplay
static Locker CheckForSoundCardsLock;
static bool checkedForSoundCards = false;
static bool haveSoundCards;

static bool try_alsa_beep(void)
{
	// check if there are any sound cards, if we haven't already
	CheckForSoundCards();
	
	if (!haveSoundCards)
		return 1;
	
	// double-fork to keep zombies away
	int child1 = fork();
	if (child1)
	{
		int status;
		waitpid(child1, &status, 0);
	}
	else
	{
		int child2 = fork();
		if (child2)
		{
			exit(0);
		}
		else
		{
			DoPlayAlsaBeep();
			exit(0);
		}
	}
	
	return 0;
}

static bool DoPlayAlsaBeep(void)
{
	FILE *fp = popen("aplay -q -", "w");
	if (!fp)
	{
		return 1;
	}
	
	fwrite(blip_wav, blip_wav_len, 1, fp);
	pclose(fp);
	
	return 0;
}

// IF we haven't done so already,
// check to see if ALSA reports there are any soundcards, and set haveSoundCards appropriately.
// This will also conveniently set haveSoundCards false if the aplay command is missing,
// as it is used to do the check - good cause soundcards or no, we can't play without aplay!
static void CheckForSoundCards(void)
{
	CheckForSoundCardsLock.Lock();
	{
		if (checkedForSoundCards)
		{
			CheckForSoundCardsLock.Unlock();
			return;
		}
		
		checkedForSoundCards = true;
		haveSoundCards = false;		// assume
		
		// check to see if aplay reports there are any sound cards
		FILE *fp = popen("aplay -l", "r");
		if (!fp)
		{
			CheckForSoundCardsLock.Unlock();
			return;
		}
		
		while(!feof(fp))
		{
			char line[1024];
			fgetline(fp, line, sizeof(line));
			//staterr("'%s'", line);
			
			if (strcasestr(line, "PLAYBACK") && strcasestr(line, "devices"))
			{
				haveSoundCards = true;
				break;
			}
			
			if (strbegin(line, "card "))
			{
				haveSoundCards = true;
				break;
			}
			
			if (strcasestr(line, "no soundcards found"))
				break;
		}
		
		pclose(fp);
	}
	CheckForSoundCardsLock.Unlock();
}

/*
void c------------------------------() {}
*/

static bool try_x_bell(void)
{
	const char *DISPLAY = getenv("DISPLAY");
	if (!DISPLAY || !DISPLAY[0])
		return 0;
	
	if (which("xkbbell"))
	{
		system("xkbbell &");
		return 0;
	}

	return 1;
}

static bool try_pc_beep(void)
{
	if (!which("beep"))
		return 1;
	
	system("beep -f 900 -l 100 &");
	return 0;
}

bool sysbeep(void)
{
	if (try_alsa_beep() || try_pc_beep() || try_x_bell())
		return 1;
	
	return 0;
}
