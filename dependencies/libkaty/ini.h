
#ifndef _KINI_H
#define _KINI_H

struct KINIKey
{
	DString name;
	DString value;
};

struct KINISection
{
	~KINISection()
	{
		for(int i=0;;i++)
		{
			KINIKey *key = (KINIKey *)keys.ItemAt(i);
			if (key) delete key; else break;
		}
		
		keys.MakeEmpty();
	}
	
	DString name;
	List<KINIKey> keys;
};

/*
void c------------------------------() {}
*/

enum {		// KINILine types
	KINI_BLANK,		// blank or comment
	KINI_SECTION,	// section header - key points to name
	KINI_KEYPAIR,	// key/value, key points to key, value to value
};

struct KINILine
{
	KINILine() { key = value = NULL; }
	~KINILine() { if (key) free(key); if (value) free(value); }
	
	char *key;
	char *value;
	int type;
};

/*
void c------------------------------() {}
*/

class KINIFile
{
public:
	KINIFile();
	~KINIFile();
	
	bool load(const char *fname);
	bool save(const char *fname);
	
	const char *GetSection();
	const char *EnumSections(int index);
	bool SetSection(const char *sectname, bool createIfNeeded=false);
	
	const char *ReadKey(const char *keyname, const char *def = NULL);
	char *DReadKey(const char *keyname, const char *def = NULL);
	int ReadKeyInt(const char *keyname, int def = 0);
	double ReadKeyFloat(const char *keyname, double def = 0);
	int64_t ReadKeyInt64(const char *keyname, int64_t def = 0);
	const char *EnumKeys(int index);
	
	bool WriteKey(const char *keyname, const char *newvalue);
	bool WriteKeyInt(const char *keyname, int newValue);
	bool WriteKeyFloat(const char *keyname, double newValue);
	bool WriteKeyInt64(const char *keyname, int64_t newValue);
	
	bool DeleteKey(const char *keyname);
	void Clear();

private:
	KINISection *FindSection(const char *name);
	KINIKey *FindKey(const char *name);
	
	List<KINISection> sections;
	KINISection *cursection;
};



#endif








