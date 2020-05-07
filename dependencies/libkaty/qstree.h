
#ifndef _QSTREE_H
#define _QSTREE_H

struct QSNode
{
	QSNode *children[256];		// pointer to all subletters
	uint8_t numchildren;		// counts non-null children in children[]: 0 = leaf
	char *str;					// if a leaf, pointer to rest of string
	void *answer;				// if an answer is at this string, ptr to it
	int strLen;					// length of str in string
	#ifdef DEBUG_QSTREE
	int nodeid;					// debug
	#endif
};


class QSTree
{
public:
	QSTree();
	~QSTree();
	
	void AddMapping(const char *str, void *answer);
	void AddMapping(const char *str, int32_t answer);	// deprecated: back-compat
	void AddIntMapping(const char *str, int32_t answer);
	
	void *Lookup(const char *str);
	int32_t LookupInt(const char *str);
	
	void Delete(const char *str);
	void MakeEmpty();
	
	void DumpTree();

private:
	QSNode root;
};


template<typename T> class QSTreeFor {
public:
	QSTree tree;
	
	inline void AddMapping(const char *str, T *answer) {
		tree.AddMapping(str, (void *)answer);
	}
	
	inline T* Lookup(const char *str) {
		return (T*)tree.Lookup(str);
	}
	
	inline void Delete(const char *str) {
		tree.Delete(str);
	}
	
	inline void MakeEmpty() {
		tree.MakeEmpty();
	}
	
	inline void DumpTree() {
		tree.DumpTree();
	}
};



#endif
