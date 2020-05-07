
#ifndef _STRINGLIST_H
#define _STRINGLIST_H

#include "BList.h"

class StringList : public BList
{
public:
	StringList() { }
	
	StringList(const StringList &other)
	{
		*this = other;
	}
	
	virtual ~StringList();
	
	void AddString(const char *str);
	void AddString(const char *str, int len);
	void AddStringNoCopy(const char *str);
	char *StringAt(int index) const;
	void MakeEmpty();
	
	void Shuffle();
	bool ContainsString(const char *term);
	bool ContainsCaseString(const char *term);
	bool RemoveString(int index);
	int RemoveString(const char *str);
	int RemoveCaseString(const char *str);
	int IndexOfString(const char *term);
	
	void SwapItems(int index1, int index2);
	void DumpContents();
	
	int32_t CountItems() const { return BList::CountItems(); }
	
	StringList &operator= (const StringList &other);
	bool operator== (const StringList &other) const;
	bool operator!= (const StringList &other) const;

private:
	class iterator
	{
		public:
			iterator(StringList *list, int startIndex) {
				this->list = list;
				index = startIndex;
			}
			
			iterator &operator++()
			{
				index++;
				return *this;
			}
			
			const char* operator*() const {
				return list->StringAt(index);
			}
			
			bool operator!= (const iterator &other) const {
				return index != other.index;
			}
		
		private:
			int index;
			StringList *list;
	};
	
public:
	iterator begin() {
		return iterator(this, 0);
	}
	
	iterator end() {
		return iterator(this, CountItems());
	}
};



#endif
