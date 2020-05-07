
#ifndef _K_MISC_H
#define _K_MISC_H

class DString;

uint16_t fgeti(FILE *fp);
uint32_t fgetl(FILE *fp);
void fputi(uint16_t word, FILE *fp);
void fputl(uint32_t word, FILE *fp);

uint64_t fget64(FILE *fp);
void fput64(uint64_t word, FILE *fp);

int fgetline(FILE *fp, char *str, int maxlen);
int fpeekline(FILE *fp, char *str, int maxlen);

size_t filesize(FILE *fp);
void freadstring(FILE *fp, char *buf, int max);
void freadstring(FILE *fp, DString *buf);
void fwritestring(const char *buf, FILE *fp);
void fwritestring(DString *str, FILE *fp);

int randrange(int min, int max);
void touch(const char *filename);
void strtoupper(char *str);
void strtolower(char *str);
char *GetStaticStr(int *size_out = NULL);
const char *GetFileSpec(const char *file_and_path);
const char *GetExtension(const char *fname);
const char *RemoveExtension(const char *fname_in);
const char *ChangeExtension(const char *fname_in, const char *new_ext);
char *GetFileSpec(char *file_and_path);
char *RemoveFileSpec(const char *input_file);
char *GetFilePath(const char *input_file);
char *stprintf(const char *fmt, ...);
char *staticstr(const char *str);

char *comma_number(int64_t value);
char *number_comma_fmt(int64_t value);	// deprecated name for comma_number (we accidently wrote two functions to do the same thing)

bool file_exists(const char *filename);
bool dir_exists(const char *filename);
void maxcpy(char *dst, const char *src, int maxlen);
void hexdump(const void *data, int len);
const char *prtbin(size_t value, int nbits);
bool strbegin(const char *bigstr, const char *smallstr);
bool strcasebegin(const char *bigstr, const char *smallstr);
void EnsureTrailingSlash(char *path);
void EnsureNoTrailingSlash(char *path);
uint8_t *readfile(const char *fname, int *size_out = NULL);
char *GetOpenFileName(int fildes);
bool mvfile(const char *srcname, const char *dstname);
bool CopyFile(const char *srcname, const char *dstname);

char *GetCurrentEXEFile(char *buffer = NULL, int maxlen = -1);
char *GetCurrentEXEPath(char *buffer = NULL, int maxlen = -1);
bool SetCWDToEXEPath(void);

bool is_string_numeric(const char *str, bool allow_decimals = false, bool allow_negative = true);
const char *human_kb(int64_t value);
const char *printChar(int ch);

char *which(const char *fname, const char *additional_paths[] = NULL);
char *search_paths_for_file(const char *fname, const char *paths[]);
class DString;
DString *path_combine(DString *part1, const char *part2, bool check_exists = false);

bool path_match(const char *path1, const char *path2);
bool string_match(const char *haystack, const char *needle);

bool sysbeep(void);		// should really be defined in beep.h, but, a whole new header just for one function?


#define fetch_atomic(V)	\
	__atomic_load_n(&V, __ATOMIC_SEQ_CST)

#define store_atomic(V, X)	\
	__atomic_store_n(&V, (X), __ATOMIC_SEQ_CST)

#define add_atomic(V, X)	\
	__sync_fetch_and_add(&V, (X))
	
#define sub_atomic(V, X)	\
	__sync_fetch_and_sub(&V, (X))



template <typename T>
T wrap(T x, T count)
{
	while(x >= count) x -= count;
	while(x < 0) x += count;
	return x;
}

template <typename T, typename U>
T clamp(T current, U min, U max)
{
	if (current < min) return min;
	if (current > max) return max;
	return current;
}

template <typename T>
T _swap(T &a, T &b)
{
	T temp = a;
	a = b;
	b = temp;
}

#endif
