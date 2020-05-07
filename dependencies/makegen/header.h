
#ifndef _HEADER_H
#define _HEADER_H

typedef struct ml_header_section
{
	char name[MAXLEN];
	
	char exe[MAXLEN];					// e.g. OUTPUT.EXE
	char exemodule[MAXLEN];				// e.g. OUTPUT
	char obj_ext[MAXLEN];
	char cc[MAXLEN];
	char hostflags[MAXLEN];
	
	char strCompile[MAXLEN];
	char strCompileInvoke[MAXLEN];		// only relevant if Compile_Usefiles=1
	char strLinkInvoke[MAXLEN];			// ditto
	char strLinkPrefix[MAXLEN];
	char strLinkSuffix[MAXLEN];
	
	char Compile_Usefiles, Link_Usefiles;
	//char IgnoreFilePaths;
	char noparse;
	char forcebuild;
	char do_not_link;
	char arch_aware;
	
	token *Compile;
	token *CompileInvoke;
	token *LinkPrefix;
	token *LinkSuffix;
	token *LinkInvoke;
	
	struct ml_header_section *next;
} ml_header_section;


typedef struct
{
	ml_header_section *firstsec;
	ml_header_section *defsec;
} ml_header;


ml_header *ML_ReadHeader(FILE *fp);
ml_header_section *Hdr_FindSection(ml_header *hdr, char *name);
void ML_FreeHeader(ml_header *hdr);
const char *get_architecture_name(void);

#endif
