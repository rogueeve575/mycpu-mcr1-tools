
#ifndef _RPNPARSER_H
#define _RPNPARSER_H

#define RPN_PARSE_FAILURE		(INT_MIN+1)

int rpn_parse(List<Token> *expr, HashTable *variableList = NULL);

#endif
