
#include "katy.h"
#include "ini.h"
#include "ini.fdh"

#define WHITESPACE(CH)	((CH) == ' ' || (CH) == '\t')
static const char *DEFAULT_SECTION_NAME = "settings";

KINIFile::KINIFile()
{
	Clear();
}

KINIFile::~KINIFile()
{
	Clear();
}

void KINIFile::Clear(void)
{
	cursection = NULL;
	for(int i=0;;i++)
	{
		KINISection *section = (KINISection *)sections.ItemAt(i);
		if (section) delete section; else break;
	}
	
	sections.MakeEmpty();
	//SetSection(DEFAULT_SECTION_NAME, true);
}

/*
void c------------------------------() {}
*/

bool KINIFile::load(const char *fname)
{
FILE *fp;
char buffer[1024];

	Clear();
	
	fp = fopen(fname, "rb");
	if (!fp)
		return 1;
	
	KINISection *section = NULL;
	while(!feof(fp))
	{
		fgetline(fp, buffer, sizeof(buffer));
		
		KINILine *line = ParseLine(buffer);
		switch(line->type)
		{
			case KINI_SECTION:
			{
				section = new KINISection;
				section->name.SetTo(line->value);
				sections.AddItem(section);
			}
			break;
			
			case KINI_KEYPAIR:
			{
				if (!section)
				{
					staterr("key/value outside of any section: '%s' = '%s'", line->key, line->value);
					section = new KINISection;
					section->name.SetTo("");
				}
				
				KINIKey *key = new KINIKey;
				key->name.SetTo(line->key);
				key->value.SetTo(line->value);
				
				section->keys.AddItem(key);
			}
			break;
		}
		
		delete line;
	}
	
	fclose(fp);
	SetSection(DEFAULT_SECTION_NAME, true);
	return 0;
}

static KINILine *ParseLine(const char *buffer)
{
	KINILine *out = new KINILine;
	#define RETURNBLANK	\
	do {	\
		out->type = KINI_BLANK;	\
		return out;	\
	} while(0)

	if (!buffer[0])
		RETURNBLANK;
	
	// advance past whitespace
	char *line = strdupa(buffer);
	while(WHITESPACE(*line)) line++;
	
	// preliminary remove comments
	if (line[0] == '#' || line[0] == ';' || \
		(line[0] == '/' && line[1] == '/'))
	RETURNBLANK;
	
	if (line[0] == '[')
	{
		char *name = strdup(line + 1);
		char *last = strchr(name, 0) - 1;
		if (last <= name) RETURNBLANK;
		if (*last == ']') *last = 0;
		
		//stat("found section name '%s'", name);
		out->type = KINI_SECTION;
		out->value = name;
		return out;
	}
	
	// split key/value
	char *equals = strchr(line, '=');
	if (!equals) RETURNBLANK;	// don't know what this line is, ignore it
	
	char *value = equals + 1;
	while(WHITESPACE(*value)) value++;
	
	// remote quotes
	if (value[0] == '\"')
	{
		char *ptr = strchr(&value[1], 0) - 1;
		if (ptr >= &value[1] && *ptr == '\"')
		{
			*ptr = 0;
			value++;
		}
	}
	
	//if (!value[0]) RETURNBLANK;
	
	const char *keyname = line;
	char *keyend = equals - 1;
	for(;;)
	{
		if (keyend <= line)	// line is like '=value' or '    =value'
		{
			//keyname = "";
			//break;
			RETURNBLANK;
		}
		
		if (WHITESPACE(*keyend))
			keyend--;
		else
		{
			*(keyend + 1) = 0;
			break;
		}
	}
	
	out->type = KINI_KEYPAIR;
	out->key = strdup(keyname);
	out->value = strdup(value);
	return out;
}

/*
void c------------------------------() {}
*/

bool KINIFile::save(const char *fname)
{
char tmpname[1024];

	strcpy(tmpname, "/tmp/inisaveXXXXXX");
	int fildes = mkstemp(tmpname);
	if (fildes == -1)
	{
		staterr("save failed: mkstemp failed");
		return 1;
	}
	
	FILE *fp = fdopen(fildes, "wb");
	if (!fp)
	{
		staterr("failed to open temp file");
		return 1;
	}
	
	for(int i=0;;i++)
	{
		KINISection *section = (KINISection *)sections.ItemAt(i);
		if (!section) break;
		
		// don't save empty sections
		if (section->keys.CountItems() == 0)
			continue;
		
		fprintf(fp, "[%s]\n", \
			section->name.String());
		
		for(int j=0;;j++)
		{
			KINIKey *key = (KINIKey *)section->keys.ItemAt(j);
			if (!key) break;
			
			fprintf(fp, "%s = %s\n", \
				key->name.String(), key->value.String());
		}
		
		fprintf(fp, "\n");
	}
	
	fclose(fp);
	
	if (!filecmp(tmpname, fname))
	{
		remove(tmpname);
		return 0;
	}
	else
	{
		return movefile(tmpname, fname);
	}
}

static bool movefile(const char *srcname, const char *dstname)
{
	if (rename(srcname, dstname))
	{	// rename doesn't handle cross-device copies, fixme better
		return system(stprintf("mv -f %s %s", srcname, dstname));
	}
	
	return 0;
}

// returns zero if the two files are identical
static bool filecmp(const char *fn1, const char *fn2)
{
FILE *fp1, *fp2;
char buffer1[4096], buffer2[4096];

	fp1 = fopen(fn1, "rb");
	fp2 = fopen(fn2, "rb");
	if (!fp1 || !fp2)
	{
		if (fp1) fclose(fp1);
		if (fp2) fclose(fp2);
		return 1;
	}
	
	// compare lengths
	fseek(fp1, 0, SEEK_END);
	fseek(fp2, 0, SEEK_END);
	if (ftell(fp1) != ftell(fp2))
	{
		fclose(fp1);
		fclose(fp2);
		return 1;
	}
	
	// compare byte-for-byte
	fseek(fp1, 0, SEEK_SET);
	fseek(fp2, 0, SEEK_SET);
	while(!feof(fp1))
	{
		int len = fread(buffer1, sizeof(buffer1), 1, fp1);
		fread(buffer2, sizeof(buffer2), 1, fp2);
		
		if (memcmp(buffer1, buffer2, len) != 0)
		{
			fclose(fp1);
			fclose(fp2);
			return 1;
		}
		
		if (len < sizeof(buffer1))
			break;
	}
	
	fclose(fp1);
	fclose(fp2);
	return 0;
}

/*
void c------------------------------() {}
*/

const char *KINIFile::ReadKey(const char *keyname, const char *def)
{
	KINIKey *key = FindKey(keyname);
	if (key)
		return key->value.String();
	
	return def;
}

char *KINIFile::DReadKey(const char *keyname, const char *def)
{
	const char *value = ReadKey(keyname, def);
	return value ? strdup(value) : NULL;
}

// change the value of a key in an .ini file, or,
// if key not present, add it. returns 1 if key was newly created
bool KINIFile::WriteKey(const char *keyname, const char *newvalue)
{
bool keyNewlyCreated = false;

	if (!cursection)
	{
		staterr("No current section!");
		return 1;
	}
	
	KINIKey *key = FindKey(keyname);
	if (!key)
	{
		key = new KINIKey;
		key->name.SetTo(keyname);
		cursection->keys.AddItem(key);
		keyNewlyCreated = true;
	}
	
	key->value.SetTo(newvalue);
	return keyNewlyCreated;
}

bool KINIFile::DeleteKey(const char *keyname)
{
	KINIKey *key = FindKey(keyname);
	if (!key) return 1;
	
	cursection->keys.RemoveItem(key);
	delete key;
	return 0;
}

/*
void c------------------------------() {}
*/

const char *KINIFile::GetSection()
{
	if (!cursection)
		return NULL;
	
	return cursection->name.String();
}

const char *KINIFile::EnumSections(int index)
{
	KINISection *s = (KINISection *)sections.ItemAt(index);
	if (!s) return NULL;
	
	return s->name.String();
}

bool KINIFile::SetSection(const char *sectname, bool createIfNeeded)
{
	if (!sectname)
	{
		cursection = NULL;
		return 1;
	}
	
	cursection = FindSection(sectname);
	if (!cursection)
	{
		if (createIfNeeded)
		{
			cursection = new KINISection;
			cursection->name.SetTo(sectname);
			sections.AddItem(cursection);
		}
		else
		{
			cursection = FindSection(DEFAULT_SECTION_NAME);
			return 1;
		}
	}
	
	return 0;
}

KINISection *KINIFile::FindSection(const char *sectname)
{
	for(int i=0;;i++)
	{
		KINISection *section = (KINISection *)sections.ItemAt(i);
		if (!section) break;
		
		if (!strcasecmp(sectname, section->name.String()))
			return section;
	}
	
	return NULL;
}

KINIKey *KINIFile::FindKey(const char *keyname)
{
	if (!cursection)
		return NULL;
	
	for(int i=0;;i++)
	{
		KINIKey *key = (KINIKey *)cursection->keys.ItemAt(i);
		if (!key) break;
		
		if (!strcasecmp(key->name.String(), keyname))
			return key;
	}
	
	return NULL;
}

const char *KINIFile::EnumKeys(int index)
{
	KINIKey *k = (KINIKey *)cursection->keys.ItemAt(index);
	if (!k) return NULL;
	
	return k->name.String();
}

/*
void c------------------------------() {}
*/

int KINIFile::ReadKeyInt(const char *keyname, int def)
{
	const char *buffer = ReadKey(keyname, NULL);
	if (buffer)
	{
		int value = atoi(buffer);
		if (value == 0)
		{
			if (!strcasecmp(buffer, "true") || \
				!strcasecmp(buffer, "yes"))
			{
				value = true;
			}
		}
		
		return value;
	}
	
	return def;
}

double KINIFile::ReadKeyFloat(const char *keyname, double def)
{
	const char *buffer = ReadKey(keyname, NULL);
	if (buffer)
		return atof(buffer);
	
	return def;
}

int64_t KINIFile::ReadKeyInt64(const char *keyname, int64_t def)
{
	const char *buffer = ReadKey(keyname, NULL);
	if (buffer)
		return strtoll(buffer, NULL, 10);
	
	return def;
}

/*
void c------------------------------() {}
*/

bool KINIFile::WriteKeyInt(const char *key, int newValue)
{
	char str[1024];
	sprintf(str, "%d", newValue);
	return WriteKey(key, str);
}

bool KINIFile::WriteKeyFloat(const char *key, double newValue)
{
	char str[64];
	sprintf(str, "%.8f", newValue);
	
	char *ptr = strchr(str, 0) - 1;
	while(ptr >= str && (*ptr == '0' || *ptr == '.'))
	{
		if (*ptr == '.')
		{
			*ptr = 0;
			break;
		}
		
		*(ptr--) = 0;
	}
	
	return WriteKey(key, str);
}

bool KINIFile::WriteKeyInt64(const char *key, int64_t newValue)
{
	char str[1024];
	sprintf(str, "%lld", (long long)newValue);
	return WriteKey(key, str);
}
