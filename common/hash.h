
#ifndef _HASH_H
#define _HASH_H

#define HT_NUM_BUCKETS			128

namespace KeyValueType
{
	enum
	{
		Number = 1,
		String,
		Pointer
	};
}

struct KeyValuePair
{
public:
	KeyValuePair()
	{
		type = 0;	// invalid
	}
	
	~KeyValuePair()
	{
		free(keyname);
		
		if (type == KeyValueType::String)
			free(str);
	}
	
	char *Describe();

public:
	char *keyname;
	int type;
	
	union
	{
		int value;
		char *str;
		void *ptr;
	};
};


class HashTable
{
public:
	HashTable();
	~HashTable();
	
	// insertion functions
	bool AddKey(KeyValuePair *pair);
	
	bool AddValue(const char *key, int value);
	bool AddString(const char *key, const char *str);
	bool AddPtr(const char *key, void *ptr);
	
	// removal functions
	bool RemoveKey(const char *keyname);
	bool RemoveIndex(int index);
	void Clear();
	
	// lookup functions
	KeyValuePair *LookupPair(const char *keyname);
	inline bool HasKey(const char *keyname) { return (LookupPair(keyname) != NULL); }
	
	int LookupValue(const char *keyname);
	const char *LookupString(const char *keyname);
	void *LookupPtr(const char *keyname);
	
	// reverse lookup functions (slow)
	const char *ValueToKeyName(int value);
	const char *StringToKeyName(const char *str);
	const char *PtrToKeyName(void *ptr);
	
	// iteration functions
	inline int CountItems()	{ return allEntries.CountItems(); }
	inline KeyValuePair *ItemAt(int i) { return allEntries.ItemAt(i); }
	
	char *Describe();
	
private:
	List<KeyValuePair> *buckets[HT_NUM_BUCKETS];
	List<KeyValuePair> allEntries;
};

#endif
