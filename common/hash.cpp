
#include <katy/katy.h>
#include "hash.h"
#include "hash.fdh"

HashTable::HashTable()
{
	memset(buckets, 0, sizeof(buckets));
}

HashTable::~HashTable()
{
	Clear();
}

/*
void c------------------------------() {}
*/

bool HashTable::AddValue(const char *keyname, int value)
{
	if (value < 0)
	{
		staterr("illegal value %d for keyname '%s'. Positive integer required.", keyname, value);
		return 1;
	}
	
	KeyValuePair *kv = new KeyValuePair();
	kv->type = KeyValueType::Number;
	kv->keyname = strdup(keyname);
	kv->value = value;
	
	return AddKey(kv);
}

bool HashTable::AddString(const char *keyname, const char *str)
{
	KeyValuePair *kv = new KeyValuePair();
	kv->type = KeyValueType::String;
	kv->keyname = strdup(keyname);
	kv->str = strdup(str);
	
	return AddKey(kv);
}

bool HashTable::AddPtr(const char *keyname, void *ptr)
{
	KeyValuePair *kv = new KeyValuePair();
	kv->type = KeyValueType::Pointer;
	kv->keyname = strdup(keyname);
	kv->ptr = ptr;
	
	return AddKey(kv);
}

bool HashTable::AddKey(KeyValuePair *pair)
{
	if (LookupPair(pair->keyname))
	{
		staterr("key '%s' already exists in hash table", pair->keyname);
		return 1;
	}
	
	allEntries.AddItem(pair);
	
	int len = strlen(pair->keyname);
	int lastChar = toupper(pair->keyname[len ? len - 1 : 0]);
	if (lastChar >= 0 && lastChar < HT_NUM_BUCKETS)
	{
		if (!buckets[lastChar])
			buckets[lastChar] = new List<KeyValuePair>();
		
		buckets[lastChar]->AddItem(pair);
	}
	
	return 0;
}

/*
void c------------------------------() {}
*/

KeyValuePair *HashTable::LookupPair(const char *keyname)
{
List<KeyValuePair> *list;
	
	int len = strlen(keyname);
	int lastChar = toupper(keyname[len ? len - 1 : 0]);
	if (lastChar >= 0 && lastChar < HT_NUM_BUCKETS)
	{
		list = buckets[lastChar];
		if (!list) return NULL;	// no bucket for first char, so we already know the key doesn't exist
	}
	else
	{
		list = &allEntries;		// outside of range of buckets, we'll do it the slow way
	}
	
	//stat("iterate %p: %d entries", list, list->CountItems());
	for(int i=0;;i++)
	{
		KeyValuePair *kv = list->ItemAt(i);
		if (!kv || !strcasecmp(kv->keyname, keyname))
			return kv;
	}
}
	
int HashTable::LookupValue(const char *keyname)
{
	KeyValuePair *kv = LookupPair(keyname);
	if (!kv || kv->type != KeyValueType::Number) return -1;
	
	return kv->value;
}
	
const char *HashTable::LookupString(const char *keyname)
{
	KeyValuePair *kv = LookupPair(keyname);
	if (!kv || kv->type != KeyValueType::String) return NULL;
	
	return kv->str;
}
	
void *HashTable::LookupPtr(const char *keyname)
{
	KeyValuePair *kv = LookupPair(keyname);
	if (!kv || kv->type != KeyValueType::Pointer) return NULL;
	
	return kv->ptr;
}

/*
void c------------------------------() {}
*/

const char *HashTable::ValueToKeyName(int value)
{
	for(int i=0;;i++)
	{
		KeyValuePair *kv = allEntries.ItemAt(i);
		if (!kv) break;
		if (kv->type != KeyValueType::Number) continue;
		
		if (kv->value == value)
			return kv->keyname;
	}
	
	return NULL;
}

const char *HashTable::StringToKeyName(const char *str)
{
	for(int i=0;;i++)
	{
		KeyValuePair *kv = allEntries.ItemAt(i);
		if (!kv) break;
		if (kv->type != KeyValueType::String) continue;
		
		if (!strcasecmp(str, kv->str))
			return kv->keyname;
	}
	
	return NULL;
}

const char *HashTable::PtrToKeyName(void *ptr)
{
	for(int i=0;;i++)
	{
		KeyValuePair *kv = allEntries.ItemAt(i);
		if (!kv) break;
		if (kv->type != KeyValueType::Pointer) continue;
		
		if (kv->ptr == ptr)
			return kv->keyname;
	}
	
	return NULL;
}

/*
void c------------------------------() {}
*/

bool HashTable::RemoveKey(const char *keyname)
{
	KeyValuePair *kv = LookupPair(keyname);
	if (!kv) return 0;
	
	allEntries.RemoveItem(kv);
	
	int len = strlen(kv->keyname);
	int lastChar = toupper(kv->keyname[len ? len - 1 : 0]);
	if (lastChar >= 0 && lastChar < HT_NUM_BUCKETS)
		buckets[lastChar]->RemoveItem(kv);
	
	delete kv;
	return 0;
}

bool HashTable::RemoveIndex(int index)
{
	KeyValuePair *kv = allEntries.ItemAt(index);
	if (!kv) return 1;
	
	allEntries.RemoveItem(index);
	
	int len = strlen(kv->keyname);
	int lastChar = toupper(kv->keyname[len ? len - 1 : 0]);
	if (lastChar >= 0 && lastChar < HT_NUM_BUCKETS)
		buckets[lastChar]->RemoveItem(kv);
	
	delete kv;
	return 0;
}

void HashTable::Clear()
{
	// empty master list and free all KeyValuePair instances
	allEntries.Clear();
	
	// now delete each bucket--the list itself--which was allocated
	for(int i=0;i<HT_NUM_BUCKETS;i++)
		if (buckets[i]) delete buckets[i];
	
	memset(buckets, 0, sizeof(buckets));
}

/*
void c------------------------------() {}
*/

char *KeyValuePair::Describe()
{
	switch(type)
	{
		case KeyValueType::Number:
		{
			return stprintf("{ '%s' = %d }", keyname, value);
		}
		break;
		
		case KeyValueType::String:
		{
			return stprintf("{ '%s' = '%s' }", keyname, str);
		}
		break;
		
		case KeyValueType::Pointer:
		{
			if (ptr)
				return stprintf("{ '%s' = pointer<%p> }", keyname, ptr);
			else
				return stprintf("{ '%s' = pointer<null> }", keyname);
		}
		break;
		
		default:
			staterr("{ KeyValuePair with unknown type %d }", type);
		break;
	}
}

char *HashTable::Describe()
{
int sz = CountItems();

	DString str;
	str.AppendString(stprintf("{ HashTable(%d entries) = [ ", sz));
	
	for(int i=0;i<sz;i++)
	{
		KeyValuePair *kv = ItemAt(i);
		
		if (i) str.AppendString(", ");
		str.AppendString(kv->Describe());
	}
	
	if (sz) str.AppendChar(' ');
	str.AppendString("] }");
	
	return str.StaticString();
}
