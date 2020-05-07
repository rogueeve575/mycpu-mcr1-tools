
//#define DEBUG
#include "common.h"
#include <stdarg.h>     // va_list, va_start, va_arg, va_end
#include "controls.fdh"

ControlDefParser::ControlDefParser()
{
	controlList = NULL;
	t = NULL;
	pin_assignments = NULL;
	Clear();
}

ControlDefParser::~ControlDefParser()
{
	Clear();
}
	
void ControlDefParser::Clear(void)
{
	if (controlList)
	{
		controlList->Delete();
		controlList = NULL;
	}
	
	if (t)
	{
		delete t;
		t = NULL;
	}
	
	if (pin_assignments)
	{
		free(pin_assignments);
		pin_assignments = NULL;
	}
	
	max_pins = 0;
}

/*
void c------------------------------() {}
*/

ControlDefList *ControlDefParser::ParseFile(const char *fname, int _iotype, int _num_pins)
{
	stat("Parsing %s definition file %s", DescribeIOType(_iotype), fname);
	Clear();
	
	this->iotype = _iotype;
	//this->max_pins = (iotype == OUTPUT) ? NUM_OUTPUT_PINS : NUM_INPUT_PINS;
	this->max_pins = _num_pins;
	this->t = new Tokenizer();
	
	t->DecodePins(true);
	if (t->Tokenize_File(fname))
	{
		Clear();
		return NULL;
	}
	
	if (ParseTokens())
		return NULL;
	
	if (controlList == NULL)
	{
		staterr("NULL controlList after ParseTokens");
		return NULL;
	}
	
	stat("'%s': defined %d %s keyword%s",
		fname, controlList->CountItems(),
		DescribeIOType(_iotype),
		(controlList->CountItems() != 1) ? "s" : "");
	
	// create the slightly smarter ControlDefList object from the List<> of controls
	auto *cdList = new ControlDefList();
	if (cdList->Populate(controlList))
	{
		delete cdList;
		controlList->Delete();
		return NULL;
	}
	
	controlList = NULL;		// owned by cdList now
	Clear();				// free tokenizer
	
	return cdList;
}

/*
void c------------------------------() {}
*/

bool ControlDefParser::ParseTokens()
{
	if (!t)
	{
		staterr("Tokenizer not ready");
		return 1;
	}
	
	stat("ControlDefParser::ParseTokens(): parsing %d tokens, %d %s pins available",
		t->CountItems(), max_pins, DescribeIOType(iotype));
	
	if (controlList) { controlList->Clear(); delete controlList; }
	controlList = new List<ControlDef>();
	
	if (pin_assignments) free(pin_assignments);
	pin_assignments = (ControlDef **)malloc(max_pins * sizeof(ControlDef **));
	memset(pin_assignments, 0, max_pins * sizeof(ControlDef **));
	
	for(int i=0;;i++)
	{
		Token *tkn = t->next_token_except_eol();
		dstat("controldef main read '%s'", tkn->Describe());
		if (tkn->type == TKN_EOF) break;
		
		// handle tokens that kick off subparsers
		bool processed_token = false;
		
		if (tkn->type == TKN_WORD)
		{
			processed_token = true;		// assume
			
			if (!strcasecmp(tkn->text, "define"))
			{
				if (parse_control_definition(false))
					return 1;
			}
			else if (!strcasecmp(tkn->text, "multiplex"))
			{
				if (parse_control_definition(true))
					return 1;
			}
			else
			{
				processed_token = false;
			}
		}
		
		if (!processed_token)
		{
			// get token that came before the unexpected one
			Token *prevToken = (t->back_token(), t->back_token());
			t->next_token(); t->next_token();
			
			char prevText[128];
			if (prevToken->type != TKN_EOF)
				snprintf(prevText, sizeof(prevText) - 1,
					" after '%s'", prevToken->Describe());
			else
				prevText[0] = 0;
			
			staterr("unexpected '%s'%s on line %d",
				tkn->Describe(), prevText,
				tkn->line);
			
			controlList->Clear();
			delete controlList;
			controlList = NULL;
			
			return 1;
		}
	}
	
	// warn if any pins were unused
	DString unusedPins;
	int numUnusedPins = 0;
	for(int i=0;i<max_pins;i++)
	{
		if (!pin_assignments[i])
		{
			if (unusedPins.Length()) unusedPins.AppendString(", ");
			unusedPins.AppendString(stprintf("%d", i));
			numUnusedPins++;
		}
	}
	
	if (numUnusedPins)
	{
		if (numUnusedPins == 1)
			staterr("warning: unused %s pin %s", DescribeIOType(iotype), unusedPins.String());
		else
			staterr("warning: unused %s pins [%s]", DescribeIOType(iotype), unusedPins.String());
	}
	
	return 0;
}

bool ControlDefParser::parse_control_definition(bool is_multiplex)
{
Token *tkn;

	//staterr("parsing %s control definition", is_multiplex ? "multiplex" : "single pin");
	
	// read name of output
	Token *tknName = t->expect_token(TKN_WORD);
	if (!tknName) return 1;
	
	//staterr("defining %s '%s'", DescribeIOType(iotype), tknName->text);
	
	// read the =
	if (!t->expect_token(TKN_EQUALS))
		return 1;
	
	ControlDef *def = new ControlDef(tknName->text, max_pins);
	if (is_multiplex) def->SetFlag(FlagMultiplex, true);
	def->iotype = iotype;
	
	// read the list of pins
	if (parse_pin_list(def))
	{
		delete def;
		return 1;
	}
	
	// it's valid to specify one of the names in the enum as the value,
	// but we haven't read the enums yet. This lets us defer setting defaultValue.
	Token *defaultValueToken = NULL;
	bool seenDefaultKeyword = false;
	
	// read any options: DEFAULT, ACTIVE_LOW, etc until EOL
	for(;;)
	{
		tkn = t->next_token();
		if (tkn->type == TKN_EOL || tkn->type == TKN_EOF) break;
		
		bool parsed = false;
		
		if (tkn->type == TKN_WORD)
		{
			parsed = true;	// assume
			
			if (iotype == OUTPUT)
			{
				if (!strcasecmp(tkn->text, "DEFAULT"))
				{
					if (seenDefaultKeyword)
					{
						staterr("Multiple DEFAULT parameters in definition of '%s' on line %d",
							def->name,
							tkn->line);
						delete def;
						return 1;
					}
					else
					{
						seenDefaultKeyword = true;
					}
					
					if (!t->expect_token(TKN_EQUALS))
					{
						delete def;
						return 1;
					}
					
					auto *tknValue = t->next_token();
					
					if (!is_multiplex && tknValue->type != TKN_NUMBER)
					{
						staterr("Invalid default value for single-pin definition of '%s' on line %d: ( %s )",
							def->name, tkn->line,
							tknValue->Describe());
						return 1;
					}
					
					if (tknValue->type == TKN_NUMBER)
					{
						if (tknValue->value < 0 || tknValue->value > def->maxValue())
						{
							staterr("default value %d out of range for '%s' on line %d",
								tknValue->value,
								def->name,
								tknValue->line);
							delete def;
							return 1;
						}
						
						def->defaultValue = tknValue->value;
					}
					else if (tknValue->type == TKN_WORD)
					{
						// defer until after parsing enums
						defaultValueToken = tknValue;
					}
					else
					{
						staterr("expected number or enum value after 'DEFAULT =' while defining '%s' on line %d",
							def->name, tknValue->line);
						delete def;
						return 1;
					}
				}
				else if (!strcasecmp(tkn->text, "ACTIVE_LOW"))
				{
					def->SetFlag(FlagActiveLow, true);
				}
				else
				{
					parsed = false;
				}
			}
			else if (iotype == INPUT)
			{
				if (!strcasecmp(tkn->text, "conditional"))
				{
					def->SetFlag(FlagConditional, true);
				}
				else
				{
					parsed = false;
				}
			}
			else
			{
				staterr("unknown iotype %d", iotype);
				return 1;
			}
		}
		
		if (!parsed)
		{
			staterr("unexpected %s at line %d while parsing definition for '%s'", \
				tkn->Describe(), tkn->line,
				def->name);
			delete def;
			return 1;
		}
	}
	
	// if multiplex, now we must read in enums.
	if (is_multiplex)
	{
		if (read_enums(def))
		{
			delete def;
			return 1;
		}
	}
	
	// if we had a default which was referring to an enum, try to resolve it now
	if (seenDefaultKeyword)
	{
		if (defaultValueToken)
		{
			int value = def->enumEntries.LookupValue(defaultValueToken->text);
			if (value < 0)
			{
				staterr("when parsing control '%s': default value '%s' is not listed in enum list",
					def->name,
					defaultValueToken->text);
				delete def;
				return 1;
			}
			
			def->defaultValue = value;
		}
	}
	else	// else apply the default default
	{
		if (is_multiplex || def->HasFlag(FlagActiveLow))
			def->defaultValue = def->maxValue();
		else
			def->defaultValue = 0;
	}
	
	stat("Adding %s: \e[1m<\e[34m %s \e[0m\e[1m>\e[0m", DescribeIOType(iotype), def->Describe());
	controlList->AddItem(def);
	return 0;
}

bool ControlDefParser::parse_pin_list(ControlDef *def)
{
	def->numPins = 0;
	
	for(;;)
	{
		Token *tkn = t->next_token();
		//stat("parse_pin_list read %s", tkn->Describe());
		
		if (!t->expect_token(tkn, TKN_PIN))
			return 1;
		
		if (tkn->value < 0 || tkn->value >= def->max_pins)
		{
			staterr("pin '%s' invalid while defining %s: only %d %s pins are available on line %d",
				tkn->text,
				def->name,
				def->max_pins,
				DescribeIOType(iotype),
				tkn->line);
			return 1;
		}
		else if (pin_assignments[tkn->value] != NULL)
		{
			if (pin_assignments[tkn->value] == def)
			{
				staterr("pin '%s' used multiple times when defining '%s' on line %d",
					tkn->text,
					def->name,
					tkn->line);
			}
			else
			{
				staterr("error assigning pin '%s' to '%s': pin already in use by '%s' on line %d",
					tkn->text,
					def->name,
					pin_assignments[tkn->value]->name,
					tkn->line);
			}
			
			return 1;
		}
		
		pin_assignments[tkn->value] = def;
		def->pins[def->numPins] = tkn->value;
		def->numPins++;
		
		// now check for comma indicating more list
		if (t->next_token()->type != TKN_COMMA)
		{
			t->back_token();
			break;
		}
	}
	
	return 0;
}

bool ControlDefParser::read_enums(ControlDef *def)
{
	//staterr("reading enums for multiplexed output '%s'", def->name);
	
	if (!t->expect_token(t->next_token_except_eol(), TKN_OPEN_CURLY_BRACE))
		return 1;
	
	def->enumEntries.Clear();
	int value = 1;
	int used_values[def->maxValue() + 1] = { 0 };
	
	for(;;)
	{
		Token *tkn = t->next_token_except_eol();
		//staterr("read '%s'", tkn->Describe());
		
		if (tkn->type == TKN_WORD)
		{
			Token *enumName = tkn;
			Token *next = t->next_token_except_eol();
			switch(next->type)
			{
				case TKN_EQUALS:
				{
					Token *enumValue = t->next_token();
					if (enumValue->type == TKN_NUMBER)
						value = enumValue->value;
					else
					{
						staterr("Expected number after '=' while processing enum '%s' for control '%s' on line %d (got '%s')",
							enumName->text,
							def->name,
							enumName->line,
							enumValue->Describe());
						return 1;
					}
				}
				break;
				
				case TKN_COMMA:
				case TKN_CLOSE_CURLY_BRACE:
					t->back_token();	// let it be processed below
				break;
				
				default:
				{
					staterr("Expected '=' or ',' after enum '%s' for control '%s' on line %d ( got '%s' )",
						enumName->text,
						def->name,
						next->line,
						next->Describe());
					return 1;
				}
				break;
			}
			
			// check value
			if (value < 0 || value > def->maxValue())
			{
				staterr("enum '%s' of control '%s': value %d is out of range for this control (max %d) near line %d",
					enumName->text, def->name,
					value, def->maxValue(),
					enumName->line);
				return 1;
			}
			
			// add the enum name and value to the hashtable
			if (def->enumEntries.LookupPair(enumName->text))
			{
				stat("name '%s' used multiple times in enum for control '%s'", enumName->text, def->name);
				return 1;
			}
			else if (used_values[value])
			{
				staterr("warning: value %d used multiple times in enum for %s '%s'",
					value, DescribeIOType(iotype), def->name);
			}
			
			used_values[value] = true;
			
			//stat("adding enum '%s' = %d to control '%s'", enumName->text, value, def->name);
			def->enumEntries.AddValue(enumName->text, value);
			value++;
			
			// check for comma
			Token *comma = t->next_token_except_eol();
			switch(comma->type)
			{
				case TKN_COMMA: break;
				case TKN_CLOSE_CURLY_BRACE: t->back_token(); break;		// let it be processed by main loop
				default:
				{
					staterr("expected: ',' or '}' after enum %s for control '%s' (got '%s')",
						enumName->text,
						def->name,
						comma->Describe());
					return 1;
				}
				break;
			}
		}
		else if (tkn->type == TKN_CLOSE_CURLY_BRACE)
		{
			//stat("Enum closed.");
			break;
		}
		else
		{
			staterr("expected: word or '}' while processing enums on line %d (got %s)",
				tkn->line, tkn->Describe());
			return 1;
		}
	}
	
	// warn if enum doesn't use all potential values
	/*for(int i=0;i<def->maxValue();i++)
	{
		if (!used_values[i])
		{
			warning("value %d unused in %s '%s'",
				i, DescribeIOType(iotype), def->name);
		}
	}*/
	
	return 0;
}

/*
void c------------------------------() {}
*/

ControlDef::ControlDef(const char *__controlName, int __maxPins)
{
	name = strdup(__controlName);
	max_pins = __maxPins;
	
	pins = (uint8_t *)malloc(max_pins * sizeof(uint8_t *));
	numPins = 0;
	
	iotype = -1;
	flags = 0;
	defaultValue = 0;
}

ControlDef::~ControlDef()
{
	if (name) free(name);
	if (pins) free(pins);
	enumEntries.Clear();
	numPins = 0;
}

int ControlDef::LookupEnumValueByName(const char *name)
{
	return enumEntries.LookupValue(name);
}

bool ControlDef::haspin(uint8_t no)
{
	for(int i=0;i<numPins;i++)
	{
		if (pins[i] == no)
			return true;
	}
	
	return false;
}

romword_t ControlDef::SetValue(romword_t inputWord, int value)
{
	if (value > maxValue())
	{	// this should have been checked by our caller, which can handle the error more gracefully
		staterr("internal error: value %d out of range for control '%s': max value %d",
			value, name, (uint32_t)maxValue());
		exit(1);
	}
	
	for(int i=0;i<numPins;i++)
	{
		if (value & (1 << i))
			inputWord |= (1 << pins[i]);
		else
			inputWord &= ~(1 << pins[i]);
	}
	
	return inputWord;
}

romword_t ControlDef::GetValue(romword_t inputWord)
{
	if (numPins <= 0)
	{
		staterr("internal error: '%s' has no pins while processing input word 0x%06x", name, inputWord);
		exit(1);
	}
	
	romword_t outvalue = 0;
	for(int i=0;i<numPins;i++)
	{
		if (inputWord & (1 << pins[i]))
			outvalue |= (1 << i);
	}
	
	return outvalue;
}

const char *ControlDef::Describe()
{
	DString str;
	
	str.AppendString(DescribeIOType(iotype, true));
	str.AppendString(": ");
	
	str.AppendString(name);
	
	if (HasFlag(FlagMultiplex))
		str.AppendString("()");
	
	if (HasFlag(FlagActiveLow))
		str.AppendString(" ACTIVE_LOW");
	
	if (HasFlag(FlagConditional))
		str.AppendString(" CONDITIONAL");
	
	if (iotype == OUTPUT)
		str.AppendString(stprintf(" DEFAULT=%d", defaultValue));
	
	str.AppendString(" pins:[");
	for(int i=0;i<numPins;i++)
	{
		if (i) str.AppendChar(' ');
		str.AppendString(stprintf("%d", pins[i]));
	}
	
	if (!numPins) str.AppendString("(none)");
	str.AppendChar(']');
	
	if (enumEntries.CountItems())
	{
		str.AppendString(" enum [");
		for(int i=0;;i++)
		{
			KeyValuePair *kv = enumEntries.ItemAt(i);
			if (!kv) break;
			
			if (i) str.AppendString(", ");
			str.AppendString(stprintf("%s=%d", kv->keyname, kv->value));
		}
		str.AppendString("]");
	}
	
	return str.StaticString();
}

/*
void c------------------------------() {}
*/

ControlDefList::ControlDefList()
{
}

ControlDefList::~ControlDefList()
{
	Clear();
}

void ControlDefList::Clear()
{
	for(int i=0;;i++)
	{
		ControlDef *def = list.ItemAt(i);
		if (!def) break;
		
		delete def;
	}
	
	list.MakeEmpty();
	namesHash.Clear();
}

bool ControlDefList::Populate(List<ControlDef> *_list)
{
	for(int i=0;;i++)
	{
		ControlDef *def = _list->ItemAt(i);
		if (!def) break;
		
		if (namesHash.LookupPair(def->name))
		{
			staterr("a control named '%s' already exists in this list", def->name);
			return 1;
		}
		
		list.AddItem(def);
		namesHash.AddValue(def->name, i);
	}
	
	_list->MakeEmpty();
	delete _list;
	return 0;
}

ControlDef *ControlDefList::LookupByName(const char *name)
{
	int index = namesHash.LookupValue(name);
	if (index < 0) return NULL;
	
	ControlDef *def = list.ItemAt(index);
	if (!def) return NULL;
	
	if (!strcasecmp(def->name, name))
		return def;
	else
		staterr("internal error: name '%s' in hashtable as index %d, but control found there was named '%s'",
			name, index, def->name);
	
	return NULL;
}

bool ControlDefList::MapToVars(const char *name1, ControlDef **var1, ...)
{
List<const char> names;
List<ControlDef *> vars;

	stat("Mapping controls to variables...");
	names.AddItem(name1);
	vars.AddItem(var1);
	
	va_list vl;
	va_start(vl, var1);
	for(int i=0;;i++)
	{
		const char *name = va_arg(vl, const char *);
		if (!name) break;
		ControlDef **var = va_arg(vl, ControlDef **);
		if (!var)
		{
			staterr("even number of parameters required");
			return 1;
		}
		
		names.AddItem(name);
		vars.AddItem(var);
	}
	va_end(vl);
	
	for(int i=0;;i++)
	{
		const char *name = names.ItemAt(i);
		if (!name) break;
		ControlDef **var = vars.ItemAt(i);
		
		ControlDef *ctrl = LookupByName(name);
		if (!ctrl)
		{
			staterr("required control not found: '%s'", name);
			return 1;
		}
		
		{
			DString str;
			str.AppendString("\e[1m");
			str.AppendString(name);
			str.AppendString("\e[0m");
			while(str.Length() < 8+14) str.AppendChar(' ');
			str.AppendString(stprintf(" -> %p", var));
			stat("  %s", str.String());
		}
		
		*var = ctrl;
	}
	
	return 0;
}


/*
void c------------------------------() {}
*/

const char *DescribeIOType(int iotype, bool uppercase)
{
	if (iotype == INPUT) return uppercase ? "INPUT" : "input";
	if (iotype == OUTPUT) return uppercase ? "OUTPUT" : "output";
	return stprintf("<invalid iotype %d>", iotype);
}
