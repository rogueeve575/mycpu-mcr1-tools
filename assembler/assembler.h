
#ifndef _ASSEMBLER_H
#define _ASSEMBLER_H

class Label;
class DeferredLine;

class Assembler
{
public:
	Assembler();
	~Assembler();
	
	bool Assemble_File(const char *fname);
	
	bool SaveOutput(const char *fname);
	uint8_t *GetOutput(int *length_out)
	{
		if (length_out) *length_out = output.Length();
		return output.Data();
	}
	
private:
	bool DoAssembly();			// umbrella function
	
	bool FindLabelNames();		// pass 1
	bool AssembleLines();		// pass 2
	bool ResolveLabels();		// pass 3
	
	inline int GetFilePos() { return output.Length(); }						// get the current offset within the output file
	inline int GetRunfromPos() { return output.Length() + runfrom_delta; }	// get the current address that the code will be run from
	
	Label *FindLabel(const char *name, const char *scope);
	bool IsLabelDefinition(List<Token> *line);
	
	bool AssembleLine(List<Token> *line);
	bool HandleDB(List<Token> *line, int wordLength);
	bool HandleAssemblerDirective(List<Token> *line, bool wasDeferred);
	bool HandleLabel(const char *labelName);
	List<Token> *DeferLabels(List<Token> *line, List<DeferredLine> *deferred_out);
	bool HaveDeferredLineForOffset(int offset);

private:
	Tokenizer *t;
	ASMCore *asmcore;
	DBuffer output;
	int lineNum;
	
	// difference between output.length() and address that that position will be at run-time (.runfrom assembler directive)
	// to get runfrom pos, runfrom_pos = (output.Length() + runfrom_delta).
	int runfrom_delta;
	
	List<Label> labels;						// list of all labels in file
	DString currentScope;					// current label scope we're in (for '.label' labels)
	
	// list of instructions which referenced one or more labels,
	// which need reassembled once all label addresses are known (in pass 3)
	List<DeferredLine> deferredLines;
};


class Label
{
public:
	Label() { address = -1; offset = -1; }
	Label(const char *_name, const char *_scope)
	{
		name.SetTo(_name);
		scope.SetTo(_scope);
		address = -1;
		offset = -1;
	}
	
	const char *Describe();
	
	DString name;		// name of label
	DString scope;		// used for "local" labels beginning with a '.'. If empty string, we are in global scope.
	int address;		// address of the label at run-time. if -1, address is not yet known.
	int offset;			// location of the label within the output buffer. if -1, offset is not yet known.
};

// holds information about a line which contained a label during main assembly whose address
// was not yet known at the time the instruction or directive was assembled.
// These instructions will be initially assembled with a 16-bit dummy value replacing the label
// and will be reassembled once the address of all labels are known.
class DeferredLine
{
public:
	DeferredLine()
	{
		offset = -1;
		line = NULL;
	}
	
	~DeferredLine()
	{
		if (line)
			line->Delete();
	}
	
	List<Token> *line;		// original source of the line that needs to be reassembled once the labels in it are known
	int offset;				// offset within output buffer that the re-assembly should go
	int predictedLength;	// how much space was reserved in the "dummy" pass, so we can check that there's room
};

#endif
