/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BE_LIST_H
#define _BE_LIST_H

#include <stdint.h>
#include <stddef.h>
//#include "SupportDefs.h"
//#include <functional>
#include <stdio.h>


class BList {
	public:
		BList(int32_t count = 20);
		BList(const BList& anotherList);
		virtual ~BList();
		
		virtual BList& operator =(const BList &);
		
		/* Adding and removing items. */
		bool AddItem(const void* item, int32_t index);
		bool AddItem(const void* item);
		bool AddList(const BList* list, int32_t index);
		bool AddList(const BList* list);
		bool RemoveItem(void* item);
		void* RemoveItem(int32_t index);
		bool RemoveItems(int32_t index, int32_t count);
		bool ReplaceItem(int32_t index, void* newItem);
		virtual void MakeEmpty();
		
		// Reorder items
		void SortItems(int (*compareFunc)(const void*, const void*));
		bool SwapItems(int32_t indexA, int32_t indexB);
		bool MoveItem(int32_t fromIndex, int32_t toIndex);
		
		// Retrieve items
		void* ItemAt(int32_t index) const;
		void* FirstItem() const;
		void* ItemAtFast(int32_t) const;
			// does not check the array bounds!
		
		void* LastItem() const;
		void* Items() const;
		
		// Query
		bool HasItem(void* item) const;
		int32_t IndexOf(void* item) const;
		int32_t CountItems() const;
		bool IsEmpty() const;
		
		// Iteration
		void DoForEach(bool (*func)(void* item));
		void DoForEach(bool (*func)(void* item, void* arg2), void *arg2);
	
	private:
		bool _ResizeArray(int32_t count);
		
		void**	fObjectList;
		int32_t	fPhysicalSize;
		int32_t	fItemCount;
		int32_t	fBlockSize;
		int32_t	fResizeThreshold;
		uint32_t	_reserved[1];
//#ifdef B_PRIVATE
};
//#else
//} __attribute__((__deprecated__));
//#endif

// TODO: redo this a bit better than this hack, so we can store -1 for example in the list.

/* TODO: the idea of this is stupid, deprecate this and just use std::vector<int> or Stack<int> */
class IntList : public BList {
public:
	int ItemAt(int index)
	{
		void *vValue = BList::ItemAt(index);
		if (vValue == 0) return -1;
		return ((int)(size_t)vValue) - 1;
	}
	
	int FirstItem()
	{
		return ItemAt(0);
	}
	
	int LastItem()
	{
		return ItemAt(CountItems() - 1);
	}
	
	void AddItem(int value)
	{
		BList::AddItem((void *)(size_t)(value + 1));
	}
} __attribute__((__deprecated__));


template<class T>
class List : BList		// make BList an inaccessible base or else you could e.g. AddItem(void *) and break type-safety
{
public:
	List<T>()
	{
		deleteContentsOnDestroy = false;
	}
	List<T>(List<T> &anotherList)
		: BList(anotherList)
	{
		deleteContentsOnDestroy = false;
	}
	~List<T>()
	{
		if (deleteContentsOnDestroy)
			DeleteContents();
	}
	
	inline void AddItem(T* item) {
		BList::AddItem((void*)item);
	}
	inline void AddItem(T* item, int32_t index) {
		BList::AddItem((void*)item, index);
	}
	
	inline void AddList(const List<T> *list) {
		BList::AddList(list);
	}
	inline void AddList(const List<T> *list, int32_t index) {
		BList::AddList(list, index);
	}
	
	inline T* ItemAt(int index) {
		return (T*)BList::ItemAt(index);
	}
	
	int32_t CountItems() const {
		return BList::CountItems();
	}
	
	void MakeEmpty() {
		BList::MakeEmpty();
	}
	
	bool IsEmpty() const {
		return BList::IsEmpty();
	}
	
	inline T* FirstItem() {
		return (T*)BList::FirstItem();
	}
	
	inline T* LastItem() {
		return (T*)BList::LastItem();
	}
	
	inline T* RemoveItem(int index) {
		return (T*)BList::RemoveItem(index);
	}
	
	inline bool RemoveItem(T* item) {
		return BList::RemoveItem((void *)item);
	}
	
	inline T* PopItem() {
		return RemoveItem(CountItems() - 1);
	}
	
	inline int FindItem(T* item) const {
		return BList::IndexOf(item);
	} __attribute__((__deprecated__));
	
	int32_t IndexOf(void* item) const {
		return BList::IndexOf(item);
	}
	inline bool HasItem(T* item) {
		return BList::HasItem(item);
	}
	
	inline void SortItems(int (*compareFunc)(T* a, T* b)) {
		currentCompareFunc = compareFunc;
		BList::SortItems([](const void *pa, const void *pb) -> int {
			return currentCompareFunc(*((T**)pa), *((T**)pb));
		});
	}
	inline bool SwapItems(int32_t indexA, int32_t indexB) {
		return SwapItems(indexA, indexB);
	}
	inline bool MoveItem(int32_t fromIndex, int32_t toIndex) {
		return BList::MoveItem(fromIndex, toIndex);
	}
	
	inline void DeleteContents(void)
	{
		Loop(); while(T *item = Next()) delete item;
		MakeEmpty();
	}
	
	// use DeleteContents instead
	void Clear(void)
	{
		DeleteContents();
	}__attribute__((__deprecated__));
	
	// use DeleteOnDestroy feature instead
	void Delete(void)
	{
		if (this)
		{
			for(int i=CountItems()-1;i>=0;i--)
				delete (T*)BList::ItemAt(i);
			
			delete this;
		}
	}__attribute__((__deprecated__));
	
	void FreeEntries(void)
	{
		for(int i=CountItems()-1;i>=0;i--)
			free(BList::ItemAt(i));
		
		MakeEmpty();
	}__attribute__((__deprecated__));
	
	inline void SetAutoDestroy(bool enable = true) {
		deleteContentsOnDestroy = enable;
	}
	inline void DeleteOnDestroy(bool on = true) {
		deleteContentsOnDestroy = on;
	}__attribute__((__deprecated__));
	
	inline T* operator[] (int index) { return ItemAt(index); }
	
	inline void Loop() { iter_idx = 0; }
	inline T* Next() { return ItemAt(iter_idx++); }
	
	int IndexOf(void *search, bool (*filterFunc)(void *search, T* item)) {
		for(int i=0;;i++)
		{
			T* item = ItemAt(i);
			if (!item) return -1;
			if (filterFunc(search, item)) return i;
		}
	}
	T* Find(void *search, bool (*filterFunc)(void *search, T* item)) {
		for(int i=0;;i++)
		{
			T* item = ItemAt(i);
			if (!item) return NULL;
			if (filterFunc(search, item)) return item;
		}
	}

public:
	bool deleteContentsOnDestroy;
	int /*__thread*/ iter_idx;
	static __thread int (*currentCompareFunc)(T* a, T* b);

	class iterator
	{
		public:
			iterator(List<T> *list, int startIndex) {
				this->list = list;
				index = startIndex;
			}
			
			iterator &operator++() {
				index++;
				return *this;
			}
			iterator &operator--() {
				index--;
				return *this;
			}
			iterator operator++(int) {
				iterator temp = *this;
				index++;
				return temp;
			}
			iterator operator--(int) {
				iterator temp = *this;
				index--;
				return temp;
			}
			
			bool operator== (const iterator &other) const {
				return index == other.index;
			}
			bool operator!= (const iterator &other) const {
				return index != other.index;
			}
			bool operator> (const iterator &other) const {
				return index > other.index;
			}
			bool operator< (const iterator &other) const {
				return index < other.index;
			}
			bool operator>= (const iterator &other) const {
				return index >= other.index;
			}
			bool operator<= (const iterator &other) const {
				return index <= other.index;
			}
			iterator operator= (const iterator &other) {
				list = other.list;
				index = other.index;
				return *this;
			}
			
			T* operator*() const {
				return list->ItemAt(index);
			}
		
		private:
			int index;
			List<T> *list;
	};
	
	iterator begin() {
		return iterator(this, 0);
	}
	iterator end() {
		return iterator(this, CountItems());
	}
};

template<typename T>
__thread int (*List<T>::currentCompareFunc)(T* a, T* b);

#endif // _BE_LIST_H

