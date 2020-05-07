
#ifndef _CONTROLS_H
#define _CONTROLS_H

struct EnumEntry;
struct ControlDef;
class ControlDefList;

enum 	// control line types/parse modes
{
	INPUT = 0,
	OUTPUT = 1
};

enum {		// masks for ControlDef flags
	FlagMultiplex = 0x01,
	FlagActiveLow = 0x02,
	FlagConditional = 0x04
};

// parses inputs.def and outputs.def
class ControlDefParser
{
public:
	ControlDefParser();
	~ControlDefParser();
	
	ControlDefList *ParseFile(const char *fname, int _iotype, int _num_pins);
	List<ControlDef> *GetList();
	
	void Clear();
	
private:
	bool ParseTokens();
	bool parse_control_definition(bool is_multiplex);
	bool parse_pin_list(ControlDef *def);
	bool read_enums(ControlDef *def);

private:
	List<ControlDef> *controlList;
	Tokenizer *t;
	int iotype;
	
	int max_pins;
	ControlDef **pin_assignments;
};


// defines either an input or output pin (or logical group of pins)
// going in/out of the control rom.
struct ControlDef
{
public:
	ControlDef(const char *__controlName, int __maxPins);
	~ControlDef();
	
	int LookupEnumValueByName(const char *name);
	
	inline romword_t maxValue() { return (1 << numPins) - 1; }
	bool haspin(uint8_t no);
	
	romword_t SetValue(romword_t inputWord, int value);	// given a starting word, set this control to the given value, return new word
	romword_t GetValue(romword_t inputWord);		// given a micro-op word or portion of one, read the value of this control
	
	inline void setName(const char *newName)
	{
		if (name) free(name);
		name = strdup(newName);
	}
	
	inline void SetFlag(uint8_t flag, bool value = true)
	{
		if (value) flags |= flag;
		else flags &= ~flags;
	}
	
	inline bool HasFlag(uint8_t flag)
	{
		return (flags & flag) ? 1 : 0;
	}
	
	inline bool IsActive(romword_t inputWord)
	{
		romword_t value = GetValue(inputWord);
		return (flags & FlagActiveLow) ? (value == 0) : (value != 0);
	}
	
	const char *Describe();
	
public:
	uint8_t *pins;			// mapping of each bit of a word to a pin number, LSB first
	int numPins;			// how many pins have been assigned to this device
	int max_pins;			// the maximum number of pins that are available (i.e. number of inputs/outputs of the MC roms)
	
	char *name;
	int iotype;
	uint8_t flags;
	
	// used with output controls
	romword_t defaultValue;
	HashTable enumEntries;
};


class ControlDefList
{
public:
	ControlDefList();
	~ControlDefList();
	
	bool Populate(List<ControlDef> *_list);
	void Clear();
	
	ControlDef *LookupByName(const char *name);
	inline int CountItems() { return list.CountItems(); }
	
	bool MapToVars(const char *name1, ControlDef **var1, ...);
	
public:
	List<ControlDef> list;
	
private:
	HashTable namesHash;
};


const char *DescribeIOType(int iotype, bool uppercase = false);


#endif
