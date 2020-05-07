
#ifndef _LLIST_H
#define _LLIST_H

// this file provides macros which implement common operations on linked lists.
// this saves a little bit of thinking and helps prevent bugs caused by
// forgetting one of the steps.

// call them giving them the node to work on and the names of the fields
// for next, prev, first and last nodes.

// add a node to the end of a linked list
#define LL_ADD_END_ETC(O, FIRST, LAST, NEXT, PREV)	\
do {	\
	if (LAST)	\
		(LAST)->NEXT = O;	\
	else	\
		(FIRST) = O;	\
	\
	O->PREV = (LAST);	\
	O->NEXT = NULL; \
	(LAST) = O;	\
} while(0)

// add a node at the start of a linked list
#define LL_ADD_BEGIN_ETC(O, FIRST, LAST, NEXT, PREV)	\
do {	\
	O->NEXT = (FIRST);	\
	O->PREV = NULL;	\
	\
	if (FIRST)	\
		(FIRST)->PREV = O;	\
	else	\
		(LAST) = O;	\
	(FIRST) = O;	\
} while(0)

// insert a node just before another node
#define LL_INSERT_BEFORE(O, BEHIND, FIRST, LAST)	\
do {	\
	if ((BEHIND) == (FIRST))	\
		(FIRST) = O;	\
	else	\
		(BEHIND)->prev->next = O;	\
	\
	O->next = (BEHIND);	\
	O->prev = (BEHIND)->prev;	\
	(BEHIND)->prev = O;	\
} while(0)

// insert a node just after another node
#define LL_INSERT_AFTER(O, AFTER, FIRST, LAST)	\
do {	\
	if ((AFTER) == (LAST))	\
		(LAST) = O;	\
	else	\
		(AFTER)->next->prev = O;	\
	\
	O->next = (AFTER)->next;	\
	O->prev = (AFTER);	\
	(AFTER)->next = O;	\
} while(0)

// remove a node from a linked list
#define LL_REMOVE_ETC(O, FIRST, LAST, NEXT, PREV)	\
do {	\
	if (O == (FIRST))	\
		(FIRST) = (FIRST)->NEXT;	\
	else if (O->PREV)	\
		O->PREV->NEXT = O->NEXT;	\
	\
	if (O == (LAST))	\
		(LAST) = (LAST)->PREV;	\
	else \
		O->NEXT->PREV = O->PREV;	\
} while(0)

// debug function
#define LL_DUMP_LIST(START, NODETYPE)	\
do {	\
	stat("LL_DUMP_LIST");	\
	\
	NODETYPE *n = (START);	\
	int iter = 0;	\
	while(n)	\
	{	\
		stat("%d: %08x   P:%08x  N:%08x", iter++, n, n->prev, n->next);	\
		n = n->next;	\
	}	\
} while(0)

#define LL_ADD_END(O, FIRST, LAST)		LL_ADD_END_ETC(O, FIRST, LAST, next, prev)
#define LL_ADD_BEGIN(O, FIRST, LAST)	LL_ADD_BEGIN_ETC(O, FIRST, LAST, next, prev)
#define LL_REMOVE(O, FIRST, LAST)		LL_REMOVE_ETC(O, FIRST, LAST, next, prev)

template <typename T>
void LL_SwapEntries(T *t1, T *t2, T* &first, T* &last)
{
T *temp;

	// first, swap first and last pointers if one or the other is first or last
	if (first == t1)		first = t2;
	else if (first == t2)	first = t1;
	
	if (last == t1)			last = t2;
	else if (last == t2)	last = t1;
	
	// ok, hang on...
	if (t2->next==t1)
	{
		// next pointers
		if (t2->prev) t2->prev->next = t1;
		t2->next = t1->next;
		t1->next = t2;
		
		// prev pointers
		t1->prev = t2->prev;
		t2->prev = t1;
		if (t2->next) t2->next->prev = t2;
	}
	else if (t1->next==t2)
	{
		// next pointers
		if (t1->prev) t1->prev->next = t2;		// Barnie's previous, Alfred, points next to Charlie
		t1->next = t2->next;					// Barnie's next is now Dennis, Charlie's old next.
		t2->next = t1;							// Charlie's next is now Barnie
		
		// prev pointers
		t2->prev = t1->prev;					// Charlie points to Alfred, Barnie's prev.
		t1->prev = t2;							// Barnie points to Charlie.
		if (t1->next) t1->next->prev = t1;		// Dennis, Charlie's old Next, now points to Barnie
	}
	else		// tabs are not neighbors at all
	{
		// next pointers
		if (t1->prev) t1->prev->next = t2;		// Barnie's prev Alfred points to Dennis now
		if (t2->prev) t2->prev->next = t1;		// Dennis's prev Charlie points to Barnie now.
		
		temp = t1->next;
		t1->next = t2->next;
		t2->next = temp;
		
		// prev pointers
		temp = t1->prev;
		t1->prev = t2->prev;
		t2->prev = temp;
		
		// think t1_next and t2_next...since they're swapped now
		// t2 is actually t1 and vice versa
		if (t2->next) t2->next->prev = t2;		// Barnie's old Next Charlie, points at Dennis.
		if (t1->next) t1->next->prev = t1;		// Dennis's old Next Eager points to Barnie.
	}
}




#endif


