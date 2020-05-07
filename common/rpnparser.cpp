
#include "common.h"
#include "exception.h"
#include "rpnparser.fdh"

class IntStack
{
public:
	void push(int value);
	int pop(void);
	inline int CountItems() { return list.CountItems(); }
	
private:
	IntList list;
};

// parses a reverse polish notation expression contained in the given tokenList,
// attempting to resolve any TKN_WORDs or TKN_VARIABLEs by looking them up in variableList.
//
// inputs:
//		expr: List of tokens containing expression to resolve
//		variableList: Hashtable of all potential variables that might be encountered in expression and their current values
//
// returns:
//		integer value of resolved expression if successful.
//		RPN_PARSE_FAILURE if the expression could not be resolved.
//
int rpn_parse(List<Token> *expr, HashTable *variableList)
{
IntStack stack;
Token *tkn;
int index;

	try
	{
		if (expr->CountItems() == 0)
			throw_exception("empty expression");
		
		for(int index=0;;index++)
		{
			tkn = expr->ItemAt(index);
			if (!tkn) break;
			//staterr("read: '%s'", tkn->Describe());
			
			switch(tkn->type)
			{
				case TKN_WORD:
				case TKN_VARIABLE:
				{
					KeyValuePair *kv = NULL;
					if (variableList) kv = variableList->LookupPair(tkn->text);
					if (!kv)
					{
						throw_exception(stprintf("'%s' is not a known variable or keyword", tkn->text));
					}
					
					stack.push(kv->value);
				}
				break;
				
				case TKN_NUMBER:
					stack.push(tkn->value);
				break;
				
				case TKN_PLUS:
					stack.push(stack.pop() + stack.pop());
				break;
				
				case TKN_MINUS:
					stack.push(stack.pop() - stack.pop());
				break;
				
				case TKN_BITWISE_AND:
					stack.push(stack.pop() & stack.pop());
				break;
				
				case TKN_BITWISE_OR:
					stack.push(stack.pop() | stack.pop());
				break;
				
				case TKN_LOGICAL_AND:
				{
					int valueA = stack.pop();
					int valueB = stack.pop();
					stack.push((valueA != 0 && valueB != 0) ? 1 : 0);
				}
				break;
				
				case TKN_LOGICAL_OR:
				{
					int valueA = stack.pop();
					int valueB = stack.pop();
					stack.push((valueA != 0 || valueB != 0) ? 1 : 0);
				}
				break;
				
				case TKN_EQUALS:
				{
					int valueA = stack.pop();
					int valueB = stack.pop();
					stack.push((valueA == valueB) ? 1 : 0);
				}
				break;
				
				case TKN_NOTEQUALS:
				{
					int valueA = stack.pop();
					int valueB = stack.pop();
					stack.push((valueA != valueB) ? 1 : 0);
				}
				break;
				
				case TKN_EXCLAMATION:	// negate
				{
					stack.push((stack.pop() == 0) ? 1 : 0);
				}
				break;
				
				default:
				{
					staterr("unexpected token ( %s ) in expression", tkn->Describe());
				}
				break;
			}
		}
		
		if (stack.CountItems() != 1)
		{
			if (stack.CountItems() == 0)
				throw_exception("No items left on stack at end of expression");
			else
				throw_exception(stprintf("Extra items left on stack at end of expression; %d items should be only 1", stack.CountItems()));
		}
		
		return stack.pop();
	}
	catch(MyException &e)
	{
		stat("\e[1;35mError parsing expression [%s] at index %d: \e[0m%s", dump_token_list(expr), index, e.Describe());
		return RPN_PARSE_FAILURE;
	}
}

/*
void c------------------------------() {}
*/

void IntStack::push(int value)
{
	list.AddItem(value);
}

int IntStack::pop(void)
{
	int count = list.CountItems();
	if (count <= 0)
		throw_exception("stack underflow");
	
	int result = list.ItemAt(count - 1);
	list.RemoveItem(count - 1);
	return result;
}
