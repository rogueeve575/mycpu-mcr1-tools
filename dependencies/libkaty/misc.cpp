
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
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <inttypes.h>
#include "DString.h"
#include "StringList.h"
#include "stat.h"
#include "misc.h"
#include "timer.h"
#include "misc.fdh"

void stat(const char *fmt, ...);

#if __BYTE_ORDER == __LITTLE_ENDIAN
uint16_t fgeti(FILE *fp)
{
uint16_t value;
	fread(&value, 2, 1, fp);
	return value;
}

uint32_t fgetl(FILE *fp)
{
uint32_t value;
	fread(&value, 4, 1, fp);
	return value;
}

uint64_t fget64(FILE *fp)
{
uint64_t value;
	fread(&value, 8, 1, fp);
	return value;
}

void fputi(uint16_t word, FILE *fp)
{
	fwrite(&word, 2, 1, fp);
}

void fputl(uint32_t word, FILE *fp)
{
	fwrite(&word, 4, 1, fp);
}

void fput64(uint64_t word, FILE *fp)
{
	fwrite(&word, 8, 1, fp);
}
#else	// versions to explicitly read as little-endian. compiles only on big-endian architectures.
#warning "Big-endian architecture detected; compiling explicitly LE versions of fgeti/fgetl family"
uint16_t fgeti(FILE *fp)
{
	uint16_t a = fgetc(fp);
	uint16_t b = fgetc(fp);
	return (b << 8) | a;
}

uint32_t fgetl(FILE *fp)
{
	uint32_t a = fgetc(fp);
	uint32_t b = fgetc(fp);
	uint32_t c = fgetc(fp);
	uint32_t d = fgetc(fp);
	return (d<<24) | (c<<16) | (b<<8) | (a);
}

uint64_t fget64(FILE *fp)
{
	uint64_t a = fgetc(fp);
	uint64_t b = fgetc(fp);
	uint64_t c = fgetc(fp);
	uint64_t d = fgetc(fp);
	
	uint64_t e = fgetc(fp);
	uint64_t f = fgetc(fp);
	uint64_t g = fgetc(fp);
	uint64_t h = fgetc(fp);
	
	return (h<<56) | (g<<48) | (f<<40) | (e<<32) |
			(d<<24) | (c<<16) | (b<<8) | (a);
}

void fputi(uint16_t word, FILE *fp)
{
	fputc(word, fp);
	fputc(word >> 8, fp);
}

void fputl(uint32_t word, FILE *fp)
{
	fputc(word, fp);
	fputc(word >> 8, fp);
	fputc(word >> 16, fp);
	fputc(word >> 24, fp);
}

void fput64(uint64_t word, FILE *fp)
{
	fputc(word, fp);
	fputc(word >> 8, fp);
	fputc(word >> 16, fp);
	fputc(word >> 24, fp);
	
	fputc(word >> 32, fp);
	fputc(word >> 40, fp);
	fputc(word >> 48, fp);
	fputc(word >> 56, fp);
}
#endif		// __BYTE_ORDER  == __LITTLE_ENDIAN


// read a string from a file until a null is encountered
void freadstring(FILE *fp, char *buf, int max)
{
int i;

	max--;	// ensure room for null pointer
	for(i=0;i<max;i++)
	{
		int ch = fgetc(fp);
		if (ch <= 0) break;		// end of string or EOF
		buf[i] = ch;
	}
	
	// null-terminate string
	buf[i] = 0;
}

// read a string from a file into a DString
void freadstring(FILE *fp, DString *buf)
{
	for(;;)
	{
		int ch = fgetc(fp);
		if (ch <= 0) break;		// end of string or EOF
		buf->AppendChar(ch);
	}
}

// write a null-terminated string to a file
void fwritestring(const char *buf, FILE *fp)
{
	fwrite(buf, strlen(buf) + 1, 1, fp);
}

void fwritestring(DString *str, FILE *fp)
{
	fwrite(str->String(), str->Length() + 1, 1, fp);
}


// read a line from a file
// handles Unix formatted files (LF ending),
//		DOS formatted files (CR/LF ending),
//		Mac formatted files (CR ending).
//		Does not properly handle files that would have LF/CR endings, as these are super rare.
//		Also handles mixed endings within the same file.
// does not include any line endings on the returned string.
// maxlen should generally be sizeof(str).
// returns the length of the line, or -1 and an empty line once end of file is reached.
int fgetline(FILE *fp, char *str, int maxlen)
{
	if (!fgets(str, maxlen - 1, fp))
	{	// EOF
		str[0] = 0;
		return -1;
	}
	
	// trim the line from any CR's or LF's
	char *ptr = strchr(str, 0) - 1;
	while(ptr >= str)
	{
		if (*ptr != '\n' && *ptr != '\r') break;
		*(ptr--) = 0;
	}
	
	return (ptr - str) + 1;
}

int fpeekline(FILE *fp, char *str, int maxlen)
{
	if (feof(fp)) { str[0] = 0; return 1; }
	
	off_t orgPos = ftell(fp);
	int result = fgetline(fp, str, maxlen);
	fseek(fp, orgPos, SEEK_SET);
	
	return result;
}

// this function will move the cursor up to the start of the previous line; e.g.
//		fgetline(fp, str1, sizeof(str1))
//		fungetline(fp)
//		fgetline(fp, str2, sizeof(str2))
// in this example str2 will read the same line as str1.
// if the cursor is already at the beginning of the file, returns nonzero and performs no action.
/*bool __fungetline(FILE *fp)
{
const int BUFFER_SIZE = 4;
char buffer[BUFFER_SIZE];
	
	off_t filePos = ftell(fp);
	if (filePos == 0) return 1;
	
	int bufferPos = 0;
	for(;;)
	{
		if (bufferPos == 0)
		{
			int bytesToRead;
			if (filePos >= BUFFER_SIZE)
			{
				bytesToRead = BUFFER_SIZE;
				filePos -= BUFFER_SIZE;
			}
			else
			{
				if (filePos == 0)	// reached beginning of file
					break;
				
				bytesToRead = filePos;
				filePos = 0;
			}
			
			fseek(fp, filePos, SEEK_SET);
			fread(buffer, bytesToRead, 1, fp);
			bufferPos = bytesToRead - 1;
			
			stat("refill buffer: read %d bytes from offset %lld", bytesToRead, (long long int)filePos);
			hexdump(buffer, bytesToRead);
		}
		
		
	}
}*/

/*
void c------------------------------() {}
*/

// returns the size of an open file.
size_t filesize(FILE *fp)
{
	off_t orgPos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	
	size_t fileSize = (size_t)ftell(fp);
	fseek(fp, orgPos, SEEK_SET);
	
	return fileSize;
}

// reads an entire file into memory and returns a buffer to it
uint8_t *readfile(const char *fname, int *size_out)
{
	FILE *fp = fopen(fname, "rb");
	if (!fp) return NULL;
	
	fseek(fp, 0, SEEK_END);
	off_t fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	if (fsz >= INT_MAX)
	{
		staterr("file %s too large!", fname);
		fclose(fp);
		return NULL;
	}
	
	uint8_t *buffer = (uint8_t *)malloc(fsz + 1);
	if (!buffer)
	{
		staterr("unable to allocate %d bytes for file '%s'", fsz + 1, fname);
		fclose(fp);
		return NULL;
	}
	
	fread(buffer, fsz, 1, fp);
	buffer[fsz] = 0;
	fclose(fp);
	
	if (size_out)
		*size_out = fsz;
	
	return buffer;
}

/*
void c------------------------------() {}
*/

// return a random number between min and max inclusive
int randrange(int min, int max)
{
int range, val;

	range = (max - min);
	
	if (range < 0)
	{	// if they got min and max mixed up, fix it
		min ^= max;
		max ^= min;
		min ^= max;
		range = (max - min);
	}
	
	if (range >= RAND_MAX)
	{
		staterr("range > RAND_MAX", min, max);
		return 0;
	}
	
	val = rand() % (range + 1);
	return val + min;
}

// if the file does not exist, creates it
void touch(const char *filename)
{
FILE *fp;

	if (!file_exists(filename))
	{
		if ((fp = fopen(filename, "wb")))
			fclose(fp);
	}
}

void strtoupper(char *str)
{
	for(int i=0;str[i];i++)
		str[i] = toupper(str[i]);
}

void strtolower(char *str)
{
	for(int i=0;str[i];i++)
		str[i] = tolower(str[i]);
}

// given a path to a file, returns a pointer to the extension of the file,
// which will be a pointer within the input string
const char *GetExtension(const char *fname)
{
	fname = GetFileSpec(fname);
	const char *ptr = strrchr(fname, '.');
	if (!ptr) return strchr(fname, 0);	// no extension--return empty string by pointing at null terminator of input string
	
	return ptr + 1;
}

// given a path to a file, return a new string with the extension removed.
const char *RemoveExtension(const char *fname_in)
{
	char *fname = staticstr(fname_in);
	char *ptr = strrchr(GetFileSpec(fname), '.');
	if (ptr) *ptr = 0;
	return fname;
}

// given a path to a file, return a new statically-allocated string
// with the extension of the file changed to new_ext.
// new_ext can either include the leading '.' or not.
// you can also change to no extension, or add an extension to a file
// without one.
const char *ChangeExtension(const char *fname_in, const char *new_ext)
{
	char *fname = staticstr(fname_in);
	char *ptr = strrchr(GetFileSpec(fname), '.');
	if (ptr) *ptr = 0;
	
	if (!new_ext || new_ext[0] == 0)
		return fname;	// changing to having no extension; don't add dot
	
	if (new_ext[0] == '.') new_ext++;
	return stprintf("%s.%s", fname, new_ext);
}

// given the full path to a file, return just the name of the file without the path.
// the pointer returned is within the original string.
const char *GetFileSpec(const char *file_and_path)
{
	return GetFileSpec((char *)file_and_path);
}

char *GetFileSpec(char *file_and_path)
{
	char *ptr = strrchr(file_and_path, '/');
	if (ptr)
		return ptr + 1;
	else
		return file_and_path;
}

// given the full path to a file, returns the path to the directory that the file
// is contained in, without the filename.
char *GetFilePath(const char *input_file)
{
	char *buffer = staticstr(input_file);
	
	// so input_file is supposed to be a path to a file--
	// but if we can verify that they passed us a directory instead, just return the same input.
	if (dir_exists(buffer))
	{
		// if the last char is not a trailing slash, add it
		char *lastch = strchr(buffer, 0) - 1;
		if (lastch >= buffer && *lastch != '/')
		{
			lastch[1] = '/';
			lastch[2] = 0;
		}
		
		return buffer;
	}
	
	// find the last slash if any and cut it off right after that slash
	char *ptr = strrchr(buffer, '/');
	if (ptr) *(ptr + 1) = 0;
	return buffer;
}

// back-compat function; please switch to GetFilePath in new code
char *RemoveFileSpec(const char *input_file)
{
static bool hasBeenCalled = 0;
	if (!hasBeenCalled)
	{
		hasBeenCalled = 1;
		fprintf(stderr, "<libkaty> RemoveFileSpec is DEPRECATED: please use GetFilePath instead\n");
		fflush(stderr);
	}
	
	char *result = GetFilePath(input_file);
	if (!result) return NULL;
	return strdup(result);
}

// given the fd of an open file, return the name of the file
char *GetOpenFileName(int fildes)
{
	// check in /proc to find out what file this fds points to
	char linknam[MAXPATHLEN];
	sprintf(linknam, "/proc/%d/fd/%d", getpid(), fildes);
	
	int bufferLen;
	char *buffer = GetStaticStr(&bufferLen);
	if (readlink(linknam, buffer, bufferLen) < 0)
	{	// no such link
		return NULL;
	}
	else
	{
		char *ptr = strchr(buffer, 0) - 10;
		if (ptr >= buffer && !strcmp(ptr, " (deleted)"))
			*ptr = 0;
		
		return buffer;
	}
}

// if the given path doesn't end in a '/', add one.
// (it just assumes that there is enough room in the string)
// if an empty string is passed, returns an empty string back
void EnsureTrailingSlash(char *path)
{
	char *ptr = strchr(path, 0) - 1;
	if (ptr >= path && *ptr != '/')
	{
		ptr[1] = '/';
		ptr[2] = 0;
	}
}

// if the given path ends in a trailing '/', remove it.
void EnsureNoTrailingSlash(char *path)
{
	char *ptr = strchr(path, 0) - 1;
	while(ptr >= path && *ptr == '/')
		*(ptr--) = 0;
}

bool mvfile(const char *srcname, const char *dstname)
{
	if (!srcname) { staterr("NULL srcname"); return 1; }
	if (!dstname) { staterr("NULL dstname"); return 1; }
	if (!srcname[0]) { staterr("empty srcname"); return 1; }
	if (!dstname[0]) { staterr("empty dstname"); return 1; }
	
	if (!file_exists(srcname))
	{
		staterr("source file not found: '%s'", srcname);
		return 1;
	}
	
	// if dstname is a directory, fix up what they MEAN by moving the file into that directory
	DString fixedDstName;
	if (dir_exists(dstname))
	{
		fixedDstName.SetTo(dstname);
		
		const char *lastChar = strchr(dstname, 0) - 1;
		if (lastChar >= dstname && *lastChar != '/')
			fixedDstName.AppendChar('/');
		
		fixedDstName.AppendString(GetFileSpec(srcname));
		dstname = fixedDstName.String();
		//stat("fixed dst name: %s", dstname);
		//return 1;
	}
	
	if (!strcmp(srcname, dstname))
	{
		staterr("source and destination names are the same: '%s'", srcname);
		return 0;
	}
	
	int result = rename(srcname, dstname);
	if (result)
	{
		if (errno == EXDEV)
		{
			if (CopyFile(srcname, dstname))
				return 1;
			
			if (remove(srcname))
			{
				staterr("failed to remove original copy '%s': %s", srcname, strerror(errno));
				remove(dstname);
				return 1;
			}
			
			return 0;
		}
		else
		{
			staterr("rename failed for \"%s\" => \"%s\": %s", srcname, dstname, strerror(errno));
			return 1;
		}
	}
	
	return 0;
}

bool CopyFile(const char *srcname, const char *dstname)
{
	DString dstnameTempBuffer(dstname);
	dstnameTempBuffer.AppendString(stprintf(".mvtemp.%d.%d.%d", getpid(), randrange(0, 50000), (int)time(NULL)));
	const char *dstnameTemp = dstnameTempBuffer.String();
	
	// open source file
	FILE *fpi = fopen(srcname, "rb");
	if (!fpi)
	{
		staterr("failed to open source file '%s'", srcname);
		return 1;
	}
	
	// get file size and allocate temporary buffers
	int fileSize = filesize(fpi);
	int bytesLeft = fileSize;
	
	int chunkSize = MIN(fileSize, 1048576);
	uint8_t *chunk = NULL;
	
	for(;;)
	{
		chunk = (uint8_t *)malloc(chunkSize);
		if (chunk) break;
		
		if (chunkSize > 64)
		{
			chunkSize /= 2;
			continue;
		}
		
		staterr("failed to allocate %d bytes of memory", chunkSize);
		fclose(fpi);
		return 1;
	}
	
	// open destination file
	FILE *fpo = fopen(dstnameTemp, "wb");
	if (!fpo)
	{
		staterr("failed to open temporary destination file '%s'", dstnameTemp);
		fclose(fpi);
		free(chunk);
		return 1;
	}
	
	/*stat("srcname: '%s'", srcname);
	stat("dstname: '%s'", dstname);
	stat("dstnameTemp: '%s'", dstnameTemp);
	stat("total file size: %d", fileSize);
	stat("chunk size: %d", chunkSize);*/
	
	// copy the file
	for(;;)
	{
		if (bytesLeft <= 0) break;
		int bytesToCopy = MIN(chunkSize, bytesLeft);
		//stat("%s => %s: copying %d bytes", srcname, dstname, bytesToCopy);
		
		if (fread(chunk, bytesToCopy, 1, fpi) != 1 ||
			fwrite(chunk, bytesToCopy, 1, fpo) != 1)
		{
			staterr("error during copy of \"%s\" => \"%s\": %s", srcname, dstname, strerror(errno));
			fclose(fpi);
			fclose(fpo);
			remove(dstnameTemp);
			return 1;
		}
		
		bytesLeft -= bytesToCopy;
	}
	
	fclose(fpi);
	fclose(fpo);
	free(chunk);
	
	// atomically move the resulting copied file into position with rename() again,
	// now that it's on the same device
	if (rename(dstnameTemp, dstname))
	{
		staterr("failed to rename final tempfile '%s' to '%s': %s", dstnameTemp, dstname, strerror(errno));
		remove(dstnameTemp);
		return 1;
	}
	
	return 0;
}

bool file_exists(const char *filename)
{
struct stat buffer;
	
	if (!stat(filename, &buffer) && !S_ISDIR(buffer.st_mode))
		return 1;
	
	return 0;
}

bool dir_exists(const char *filename)
{
struct stat buffer;
	
	if (!stat(filename, &buffer) && S_ISDIR(buffer.st_mode))
		return 1;
	
	return 0;
}

/*
void c------------------------------() {}
*/

char *GetStaticStr(int *size_out)
{
static pthread_mutex_t static_str_mutex = PTHREAD_MUTEX_INITIALIZER;
#define GSS_STR_SIZE	(PATH_MAX+8)
static uint8_t counter = 0;
static struct
{
	char str[GSS_STR_SIZE];
} roundRobin[256];
char *result;

	pthread_mutex_lock(&static_str_mutex);
	{
		result = roundRobin[counter].str;
		counter++;
	}
	pthread_mutex_unlock(&static_str_mutex);
	
	if (size_out) *size_out = GSS_STR_SIZE;
	return result;
}

char *stprintf(const char *fmt, ...)
{
va_list ar;
int maxlen;

	char *str = GetStaticStr(&maxlen);
	va_start(ar, fmt);
	vsnprintf(str, maxlen, fmt, ar);
	va_end(ar);
	
	return str;
}

// equivalent to old common usage of "stprintf("%s", str)"
char *staticstr(const char *str)
{
	int maxlen;
	char *staticArea = GetStaticStr(&maxlen);
	maxcpy(staticArea, str, maxlen);
	return staticArea;
}

/*
void c------------------------------() {}
*/

// a strncpy that works as you might expect
void maxcpy(char *dst, const char *src, int maxlen)
{
int len = strlen(src);

	if (len >= maxlen)
	{
		if (maxlen <= 0) return;
		
		maxlen--;
		memcpy(dst, src, maxlen);
		dst[maxlen] = 0;
	}
	else
	{
		memcpy(dst, src, len + 1);
	}
}


void hexdump(const void *datain, int len)
{
const uint8_t *data = (const uint8_t *)datain;
int i;
int off = 0;
const uint8_t *linedata;
char line[68];
char *ptr;
const char *offfmt;
static const char *hexalphabet = "0123456789abcdef";

	if (len < 256)
		offfmt = "  %02X: ";
	else
		offfmt = "  %04X: ";
	
	do
	{
		linedata = &data[off];
		sprintf(line, offfmt, off);
		
		ptr = strchr(line, '\0');
		
		// print 16 chars of hex data
		for(i=0;i<16;i++)
		{
			if (off+i >= len)
			{
				*(ptr++) = ' ';
				*(ptr++) = ' ';
			}
			else
			{
				*(ptr++) = hexalphabet[(linedata[i] >> 4) & 0x0f];
				*(ptr++) = hexalphabet[linedata[i] & 0x0f];
			}
			
			if (i&1)
			{
				*(ptr++) = ' ';
			}
		}
		
		memset(ptr, ' ', 4);
		ptr += 4;
		
		// print 16 chars of hex data
/*		for(i=0;i<16;i++)
		{
			if (off+i < len)
				sprintf(line, "%s%02x", line, linedata[i]);
			else
				strcat(line, "  ");
			
			if (i & 1) strcat(line, " ");
		}
		
		strcat(line, "    ");
		
		// print the same chars again, as ASCII data
		ptr = strchr(line, '\0');*/
		for(i=0;i<16;i++)
		{
			if (off+i >= len) break;
			//*(ptr++) = ((linedata[i] > 30 && linedata[i] < 129) ? linedata[i] : '.');
			*(ptr++) = (isprint(linedata[i]) ? linedata[i] : '.');
		}
		
		//*(ptr++) = '\n';
		*ptr = 0;
		
		stat("%s", line);
		off += 0x10;
	}
	while(off < len);
}

const char *prtbin(size_t value, int nbits)
{
	//char str[nbits+1];
	char *str = GetStaticStr();
	str[nbits] = '\0';
	
	size_t mask = 1 << (nbits - 1);
	for(int i=0;i<nbits;i++)
	{
		str[i] = (value&mask) ? '1' : '0';
		mask >>= 1;
	}
	
	return str;
}

/*
void c------------------------------() {}
*/

bool strbegin(const char *bigstr, const char *smallstr)
{
	for(int i=0;smallstr[i];i++)
		if (bigstr[i] != smallstr[i]) return false;
	return true;
}

bool strcasebegin(const char *bigstr, const char *smallstr)
{
	for(int i=0;smallstr[i];i++)
	{
		if (bigstr[i] != smallstr[i] && tolower(bigstr[i]) != tolower(smallstr[i]))
			return false;
	}
	
	return true;
}

/*
void c------------------------------() {}
*/

bool is_string_numeric(const char *str, bool allow_decimals, bool allow_negative)
{
	if (allow_negative && str[0] == '-')
		str++;
	
	if (str[0] == 0)
		return false;
	
	bool haveSeenDecimal = false;
	for(int i=0;str[i];i++)
	{
		char ch = str[i];
		
		if (ch > '9' || ch < '0')
		{
			if (ch == '.')	// we might be allowing up to no more than one '.'
			{
				if (haveSeenDecimal || !allow_decimals)
					return false;
				
				haveSeenDecimal = true;
				continue;
			}
			
			return false;	// non-numeric char
		}
	}
	
	return true;
}

// deprecated name for comma_number (we accidently wrote two functions to do the same thing)
char *number_comma_fmt(int64_t value)
{
	return comma_number(value);
}

// format a number in style of 1,048,576
char *comma_number(int64_t value)
{
char *str = GetStaticStr(NULL);
int index = 0, a, b, ch1, ch2;
int digit;

	// translate number to string
	b = 0;
	for(;;)
	{
		digit = value % 10;
		value /= 10;
		str[index++] = digit + '0';
		if (value == 0)
			break;
		
		if (++b >= 3)
		{
			b = 0;
			str[index++] = ',';
		}
	}
	
	str[index] = 0;
	
	// reverse order of string
	a = 0, b = index - 1;
	while(a < b)
	{
		ch1 = str[a], ch2 = str[b];
		str[a] = ch2, str[b] = ch1;
		a++, b--;
	}
	
	return str;
}

/*char *comma_number(int64_t value)
{
char num[64];
DString out;
	
	bool negative = false;
	if (value < 0) { value = -value; negative = true; }
	
	sprintf(num, "%lld", (long long int)value);
	
	int count = 0;
	char *ptr = strchr(num, 0) - 1;
	while(ptr >= num)
	{
		out.AppendChar(*ptr);
		
		if (++count >= 3)
		{
			count = 0;
			if (ptr > num) out.AppendChar(',');
		}
		
		ptr--;
	}
	
	DString rev;
	char *str = out.String();
	
	if (negative)
		rev.AppendChar('-');
	
	for(int i=out.Length() - 1; i>=0; i--)
		rev.AppendChar(str[i]);
	
	return rev.StaticString();
}*/

// format a byte size as bytes, kb, mb, or gb
const char *human_kb(int64_t value)
{
	int buffersz;
	char *orgBuffer = GetStaticStr(&buffersz);
	char *buffer = orgBuffer;
	
	if (value < 0)
	{
		value = -value;
		*(buffer++) = '-';
	}
	
	if (value >= 1000*1000*1000)
	{
		double gb = ((double)value / (1000*1000*1000));
		snprintf(buffer, buffersz, "%.2f gb", gb);
		
		char *ptr = strchr(buffer, '.');
		if (ptr && ptr[1] == '0' && ptr[2] == '0')
			snprintf(buffer, buffersz, "%.f gb", gb);
	}
	else if (value >= 1000*1000)
	{
		double mb = ((double)value / (1000*1000));
		snprintf(buffer, buffersz, "%.2f mb", mb);
		
		char *ptr = strchr(buffer, '.');
		if (ptr && ptr[1] == '0' && ptr[2] == '0')
			snprintf(buffer, buffersz, "%.f mb", mb);
	}
	else if (value >= 1000)
	{
		snprintf(buffer, buffersz, "%d kb", (int)(value / 1000));
	}
	else
	{
		snprintf(buffer, buffersz, "%d bytes", (int)value);
	}
	
	return orgBuffer;
}

// given a character, return what we should print to represent that character
const char *printChar(int ch)
{
static const char *other_printable_chars = ",./ <>?;':\"[]\\{}|`~!@#$%^&*()_-+=";

	if (isalnum(ch) || strchr(other_printable_chars, ch))
		return stprintf("%c", ch);
	else if (!ch)
		return "(EOF/NUL)";
	else
		return stprintf("(%d [0x%02x])", ch, ch);
}

// returns true if the two paths are the same.
// this is useful to detect e.g.
//	/mnt	/mnt/	/etc/../mnt/
// it will also read through symlinks to determine samenity
bool path_match(const char *path1, const char *path2)
{
char rpath1[MAXPATHLEN+1], rpath2[MAXPATHLEN+1];
	
	// try to realpath() to get the absolute canonical path of both paths.
	// this'll work if both paths exists.
	if (!realpath(path1, rpath1) || !realpath(path2, rpath2))
	{
		// one or both of the paths don't actually exist, so just try to fix them up
		// by ensuring they both are in the trailing slash form, although that's cheesy
		strcpy(rpath1, path1);
		strcpy(rpath2, path2);
		EnsureTrailingSlash(rpath1);
		EnsureTrailingSlash(rpath2);
	}
	
	//stat("R1 '%s' R2 '%s' ", rpath1, rpath2);
	return !strcmp(rpath1, rpath2);
}

// match a string with support for * wildcards kinda like shell does
// returns true if haystack contains needle, where needle can contain '*' as wildcards
// example: haystack="alsa-utils-1.3.10.tar.gz", needle="*utils*gz"
bool string_match(const char *haystack, const char *needle)
{
	if (!needle[0])			// an empty string matches everything
		return true;
	
	int ndlpos = 0;
	int haypos = 0;
	for(;;)
	{
		char ndlchar = needle[ndlpos++];
		//stat("read ndlchar '%s'", chr(ndlchar));
		
		if (ndlchar == '*')
		{
			// get the next needle char after the wildcard
			char nextndl = needle[ndlpos];
			while(nextndl == '*') nextndl = needle[++ndlpos];
			
			//stat("nextndl = '%s'", chr(nextndl));
			
			// if the char is 0, then needle ends in a wildcard, rest of string
			// doesn't matter, we have a match
			if (nextndl == 0)
			{
				///stat("* at end of string, match");
				return true;
			}
			
			// read the string to be matched by moving forward in needle until we
			// hit the next * or EOL
			ndlpos++;
			DString match_string;
			match_string.AppendChar(nextndl);
			for(;;)
			{
				char ch = needle[ndlpos];
				if (ch == '*' || ch == 0) break;
				match_string.AppendChar(ch);
				ndlpos++;
			}
			
			/*stat("match_string = '%s'", match_string.String());
			stat("needle left: '%s'", &needle[ndlpos]);
			stat("hay left: '%s'", &haystack[haypos]);*/
			
			// try to find the match_string in the remaining haystack
			const char *hayremain = &haystack[haypos];
			const char *match_pos = strstr(hayremain, match_string.String());
			if (!match_pos)
			{
				///stat("match string '%s' not found in remaining hay, MISMATCH");
				return false;
			}
			
			// get index that match was found at
			// move forward haystack to just after the match
			int matchIndex = (match_pos - hayremain);
			//stat("Match found at index %d", matchIndex);
			haypos += matchIndex;
			haypos += match_string.Length();
			
			/*stat("And again:");
			stat("needle left: '%s'", &needle[ndlpos]);
			stat("hay left: '%s'", &haystack[haypos]);*/
		}
		else
		{
			char haychar = haystack[haypos++];
			//stat("comparing haychar '%s' to needlechar '%s'", chr(haychar), chr(ndlchar));
			
			if (haychar != ndlchar)
			{
				///stat("MISMATCH on hay/ndl compare");
				return false;
			}
			else if (haychar == 0)		// they match, and both reached end of string at same time
			{
				///stat("MATCH, both reached end of string");
				return true;
			}
		}
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/

char *GetCurrentEXEFile(char *buffer, int maxlen)
{
	if (!buffer)
		buffer = GetStaticStr(&maxlen);
	else if (maxlen < 0)
		maxlen = MAXPATHLEN;
	
	char linkname[128];
	sprintf(linkname, "/proc/%d/exe", getpid());
	
	int result = readlink(linkname, buffer, maxlen - 1);
	if (result < 0)
	{
		buffer[0] = 0;
		return NULL;
	}
	
	buffer[result] = 0;
	return buffer;
}

char *GetCurrentEXEPath(char *buffer, int maxlen)
{
	if (!buffer)
		buffer = GetStaticStr(&maxlen);
	else if (maxlen < 0)
		maxlen = MAXPATHLEN;
	
	char temp[MAXPATHLEN];
	if (!GetCurrentEXEFile(temp, sizeof(temp)))
		return NULL;
	
	maxcpy(buffer, GetFilePath(temp), maxlen);
	return buffer;
}

bool SetCWDToEXEPath(void)
{
	char path[MAXPATHLEN];
	if (!GetCurrentEXEPath(path, sizeof(path)))
		return 1;
	
	return chdir(path);
}

/*
void c------------------------------() {}
*/

// search for a file within all directories of the PATH variable
// similar to "which" command
char *which(const char *fname, const char *additional_paths[])
{
	char *path = getenv("PATH");
	if (!path || path[0] == 0)
		return NULL;//path = "/bin:/usr/bin:/usr/local/bin";
	
	DString pathBuffer(path);
	if (additional_paths)
	{
		for(int i=0;additional_paths[i];i++)
		{
			pathBuffer.AppendChar(':');
			pathBuffer.AppendString(additional_paths[i]);
		}
	}
	
	path = pathBuffer.String();
	//stat("PATH = '%s'", path);
	
	StringList paths;
	char *tokdir = path;
	char testpath[MAXPATHLEN+strlen(fname)+6];
	for(;;)
	{
		char *nextpath = strtok(tokdir, ":");
		if (!nextpath) break;
		tokdir = NULL;
		
		if (!dir_exists(nextpath)) continue;
		
		maxcpy(testpath, nextpath, sizeof(testpath));
		EnsureTrailingSlash(testpath);
		
		if (paths.ContainsString(testpath)) continue;
		paths.AddString(testpath);
		//stat("%s", testpath);
	}
	
	if (paths.CountItems() == 0)
		return NULL;
	
	// turn the stringlist into an array
	const char *pathsArray[paths.CountItems() + 1];
	for(int i=0;;i++)
	{
		const char *str = paths.StringAt(i);
		pathsArray[i] = str;
		if (!str) break;
	}
	
	return search_paths_for_file(fname, pathsArray);
}

// search the specified (null terminated) array of paths looking for one which
// contains the specified filename. returns the full path to the file if found, or NULL.
char *search_paths_for_file(const char *fname, const char *paths[])
{
	for(int i=0;;i++)
	{
		const char *path = paths[i];
		if (!path) break;
		
		DString fullpath(path);
		path_combine(&fullpath, fname);
		
		if (file_exists(fullpath.String()))
			return staticstr(fullpath.String());
	}
	
	return NULL;
}

// modify the directory part1 by adding in the directory or filename part2, getting the slashes right.
// if part1 is non-empty, part2 must be a relative path (not start with /)
//
// most common intended usage of this function is to append a filename to a directory to get a full path.
//
// if check_exists is true (default false), the function will also check that the file or directory named
// after combining actually exists, and return NULL if it doesn't.
//
// returns back the modified string for convenience.
DString *path_combine(DString *part1, const char *part2, bool check_exists)
{
	if (part2[0] == '/')
	{
		if (!part1->Length())
		{
			part1->SetTo(part2);
			return part1;
		}
		
		staterr("absolute path specified for part2; relative path required: '%s'", part2);
		return NULL;
	}
	
	// append the slash separator to part1 if needed
	if (part1->LastChar() != '/')
		part1->AppendChar('/');
	
	part1->AppendString(part2);
	
	if (check_exists)
	{
		struct stat st;
		if (stat(part1->String(), &st))
			return NULL;
	}
	
	return part1;
}
