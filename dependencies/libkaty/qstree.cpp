
//#define DEBUG_QSTREE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include "stat.h"
#include "qstree.h"
#include "qstree.fdh"

#ifdef DEBUG_QSTREE
int nodeid = 1;
#define DEBUG(...)	stat(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

QSTree::QSTree()
{
	memset(&root, 0, sizeof(QSNode));
}

QSTree::~QSTree()
{
	MakeEmpty();
}

/*
void c------------------------------() {}
*/

/*
	ADD MAPPING
	
	start at the root node.
	
	if we are at the end of the string
		put the answer into the current node and exit.
	
	check if there is a child for the next letter
		if yes
			enter into child
		if no
			create a child for the next letter we need
			make that child a leaf and give it our answer
			
			if this node has a str and the str has more child letters,
				remove the answer from this node
				recursively add current mapping back in starting at this node
			
			exit
*/

void QSTree::AddMapping(const char *str, void *answer)
{
	char recursestr[1024];
	int strLen = strlen(str);
	int strDepth = 0;
	QSNode *node = &root;
top: ;
	DEBUG("AddMapping('%s', '%08x') to node %d, strLen = %d, strDepth = %d",
		str, answer, node->nodeid, strLen, strDepth);
	for(;;)
	{
		uint8_t ch = str[strDepth];
		
		if (!ch)
		{
			DEBUG("reached end of str, dump here");
			if (node->str)
			{
				if (node->strLen == strLen)		// changing a mapping's answer
					free(node->str);
				else
				{	// example: add "so" then "soa"
					DEBUG("dump str has old answer");
					char *oldstr = node->str;
					int oldStrLen = node->strLen;
					void *oldanswer = node->answer;
					
					node->str = strdup(str);
					node->strLen = strLen;
					node->answer = answer;
					
					DEBUG("recurse back to add old dump answer");
					
					maxcpy(recursestr, oldstr, sizeof(recursestr));
					free(oldstr);
					str = recursestr;
					answer = oldanswer;
					strLen = oldStrLen;
					goto top;
				}
			}
			
			node->str = strdup(str);
			node->strLen = strLen;
			node->answer = answer;
			return;
		}
		
		if (node->children[ch])
		{
			node = node->children[ch];
			DEBUG("enter into child %d for '%c'", node->nodeid, ch);
		}
		else
		{
			QSNode *newchild = new QSNode;
			memset(newchild->children, 0, sizeof(newchild->children));
			newchild->numchildren = 0;
			newchild->str = strdup(str);
			newchild->strLen = strLen;
			newchild->answer = answer;
			#ifdef DEBUG_QSTREE
			newchild->nodeid = nodeid++;
			#endif
			DEBUG("no child for '%c', creating leaf %d", ch, newchild->nodeid);
			
			node->children[ch] = newchild;
			node->numchildren++;
			
			//F
			//--O
			//----O	foo strLen = 3  strDepth = 2
			
			// check if this node had a str and the str has more letters
			// to go now that this isn't a leaf
			if (node->str)
			{
				DEBUG("node %d has an existing str: '%s', strLen = %d, strDepth = %d", \
					node->nodeid, node->str, node->strLen, strDepth);
			}
			
			if (node->str && node->strLen > strDepth)
			{
				DEBUG("Recurse to add back old answer");
				
				maxcpy(recursestr, node->str, sizeof(recursestr));
				str = recursestr;
				answer = node->answer;
				strLen = node->strLen;
				
				free(node->str);
				node->str = NULL;
				node->answer = NULL;
				node->strLen = 0;
				
				goto top;
			}
			else DEBUG("not recursing because it's already at the end");
			
			return;
		}
		
		strDepth++;
	}
}

void QSTree::DumpTree()
{
	stat("");
	stat("-- TREE DUMP --");
	DumpTree_r(&root, 0);
	stat("");
}

static void DumpTree_r(QSNode *node, int depth)
{
char spac[120];
int i;

	int sl = depth * 2;
	memset(spac, ' ', sl);
	spac[sl] = 0;
	
	#ifdef DEBUG_QSTREE
		stat("%snode %d", spac, node->nodeid);
	#else
		stat("%snode %x", spac, node);
	#endif
	
	stat("%snumchildren %d", spac, node->numchildren);
	if (node->str || node->answer)
	{
		stat("%sstr '%s'", spac, node->str);
		stat("%sanswer 0x%08x", spac, node->answer);
		stat("%sstrLen %d", spac, node->strLen);
	}
	
	for(i=0;i<256;i++)
	{
		if (node->children[i])
		{
			stat("%s'%c'", spac, i);
			DumpTree_r(node->children[i], depth+1);
		}
	}
}

/*
	LOOKUP MAPPING
	
	start at the root node.
	
	if we are at the end of the string, check if this node has an answer.
	if so, return said answer, else return NULL.
	
	if the current node is a leaf
		check if it has an answer (i think it always should?)
		check the remaining portion of the string against the
		remaining portion of the answer
		if match, return the answer, else NULL
	
	if the current node is a node
		check if the child corresponding to next letter exists
		if so, descend into next child
		else return NULL
*/

void *QSTree::Lookup(const char *str)
{
	QSNode *node = &root;
	int strDepth = 0;
	for(;;)
	{
		uint8_t ch = str[strDepth];
		DEBUG("at strDepth %d, node %d, letter '%c'", strDepth, node->nodeid, ch ? ch : '*');
		if (!ch)
			return (node->str && node->str[strDepth] == 0) ? node->answer : NULL;
		
		if (node->numchildren == 0)
		{
			if (!node->str)
				return NULL;
			
			const char *s1 = &str[strDepth];
			const char *s2 = &node->str[strDepth];
			DEBUG("compare at index %d: my '%s' with nodes '%s'", strDepth, s1, s2);
			if (!strcmp(s1, s2))
				return node->answer;
			else
				return NULL;
		}
		else
		{
			node = node->children[ch];
			if (!node) { DEBUG("no child '%c', return NULL", ch);
				return NULL; }
			DEBUG("enter into child '%c'", ch);
		}
		
		strDepth++;
	}
}

/*
void c------------------------------() {}
*/

int32_t QSTree::LookupInt(const char *str)
{
	return ((size_t)Lookup(str)) - 1;
}

void QSTree::AddMapping(const char *str, int32_t answer)
{
	AddMapping(str, (void *)(((size_t)answer) + 1));
}

void QSTree::AddIntMapping(const char *str, int32_t answer)
{
	AddMapping(str, (void *)(((size_t)answer) + 1));
}

/*
void c------------------------------() {}
*/

/*
	DELETE
	
	recurse down tree until we enter a leaf or reach end of str.
	
	if we entered a leaf, check rest of string to see if it matches.
	if we reached end of str, check that node->str[strDepth] is 0
	if test fails string does not exist in tree, abort
	
	set str/answer to NULL
	if numchildren is 0 and has no str
		delete this node
		move up to parent
		decrement parent's numchildren
		loop
*/
void QSTree::Delete(const char *str)
{
QSNode *path[strlen(str)];

	DEBUG("Delete '%s'", str);
	int pathp = 0;
	int strDepth = 0;
	QSNode *node = &root;
	
	for(;;)
	{
		path[pathp++] = node;
		uint8_t ch = str[strDepth];
		DEBUG("at strDepth %d, node %d, letter '%c'", strDepth, node->nodeid, str[strDepth] ? str[strDepth] : '*');
		
		if (!ch)
		{
			if (!node->str || node->str[strDepth])
			{
				DEBUG("doesn't exist(1)");
				return;	// entry doesn't exist in tree
			}
			
			break;
		}
		
		if (!node->numchildren)
		{
			if (!node->str)		// entry doesn't exist in tree
				return;			// can happen with empty tree
			
			const char *s1 = &str[strDepth];
			const char *s2 = &node->str[strDepth];
			DEBUG("in leaf, compare at index %d: my '%s' with nodes '%s'", strDepth, s1, s2);
			if (strcmp(s1, s2) != 0)	// entry doesn't exist in tree
				return;
			
			DEBUG("Match.");
			break;
		}
		
		if (!node->children[ch])
			return;		// entry doesn't exist in tree
		
		node = node->children[ch];
		strDepth++;
	}
	
	// boom, it's gone
	DEBUG("Boom, it's gone.");
	free(node->str);
	node->str = NULL;
	node->answer = NULL;
	node->strLen = 0;
	
	#if 0
	DumpTree();
	
	if (node->numchildren)
	{	// deleted from a non-leaf node: example: add "foo"+"foobar", delete "foo"
		DEBUG("deleted from a non-leaf node");
		
		// walk down until we find something that has an answer or other children
		for(;;)
		{
			DEBUG("walkdown: in node %d", node->nodeid);
			if (node->str || node->numchildren != 1)
				break;
			
			// find which child it is that it has
			for(int i=0;i<256;i++)
				if (node->children[i]) break;
			
			DEBUG("walkdown: entering into child '%c'", i);
			node = node->children[i];
			path[pathp++] = node;
		}
		
		QSNode *reattach_node = node;
		
		// walk back up deleting nodes until we find something that has a str
		// or other children or we reach the root node.
		pathp--;
		while(pathp > 0)
		{
			node = path[--pathp];
			DEBUG("walkup: up to node %d", pathp, node->nodeid);
			if (node->str || node->numchildren != 1)
				break;
			
			// delete this node
			if (pathp)
				delete node;
		}
		
		node->numchildren--;		// fixup top
		
		// reattach the node here
		// if the node has other children, attach as a child
		// if this was the node's only child, make it into a leaf
		if (!node->str)
		{	// make into a leaf
			DEBUG("make into leaf");
			memset(node->children, 0, sizeof(node->children));
			node->str = reattach_node->str;
			node->answer = reattach_node->answer;
			node->strLen = reattach_node->strLen;
			node->nodeid = reattach_node->nodeid;
			delete reattach_node;
		}
		else
		{	// attach node here
		DEBUG("reattach node");
		DEBUG("pathp = %d", pathp);
		DEBUG("child letter = '%c'", reattach_node->str[pathp]);
			node->children[reattach_node->str[pathp]] = reattach_node;
		}
	}
	else
	{	// deleting from a leaf node
		// collapse unnecessary leaves
		--pathp;
		for(;;)
		{
			DEBUG("In node %d, numchildren = %d, str = '%s'",
				node->nodeid, node->numchildren, node->str);
			
			if (node->numchildren == 0 && !node->str) {}
			else
			{
				break;
			}
			
			delete node;
			if (--pathp < 0) break;
			DEBUG("move up to pathp %d", pathp);
			node = path[pathp];	// move up to parent
			DEBUG("new node %x", node->nodeid);
			node->numchildren--;
		}
	}
	#endif
}

void QSTree::MakeEmpty()
{
	MakeEmpty_r(&root, false);
	memset(&root, 0, sizeof(QSTree));
}

static void MakeEmpty_r(QSNode *node, bool del)
{
int i;

	if (node->numchildren)
	{
		for(i=0;i<256;i++)
		{
			if (node->children[i])
				MakeEmpty_r(node->children[i], true);
		}
	}
	
	if (node->str)
		free(node->str);
	
	if (del)
		delete node;
}

/*
struct QSNode
{
	QSNode *children[256];		// pointer to all subletters
	bool leaf;					// set 1 if all children are NULL
	char *str;					// if a leaf, pointer to rest of string
	void *answer;				// if an answer is at this string, ptr to it
};


class QSTree
{
public:
	QSTree();
	~QSTree();
	
	void AddMapping(const char *str, void *answer);
	void AddMapping(const char *str, int32_t answer);
	
	void *Lookup(const char *str);
	int32_t LookupInt(const char *str);
	
	void Delete(const char *str);
	void MakeEmpty();

private:
	QSNode root;
}

*/


