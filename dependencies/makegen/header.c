
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "makegen.h"

static ml_header_section *addsection(char *name, ml_header *hdr);
static char AssignValue(char *assign, char *value, ml_header_section *h);
static char AssignGlobalValue(char *assign, char *value, ml_header_section *h);
static char FinishSection(ml_header_section *h, ml_header *hdr);
static char try_assign(token **dest, char *assignlook, char *assign, char *value);


// reads the header of a .ml file and returns a header struct
ml_header *ML_ReadHeader(FILE *fp)
{
ml_header *hdr;
ml_header_section *cursec;
char line[MAXLEN];
char ok;

	//printf("\n\nreading header.\n");
	
	hdr = malloc(sizeof(ml_header));
	if (!hdr)
	{
		printf("ML_ReadHeader: out of memory!");
		return NULL;
	}
	
	hdr->firstsec = NULL;
	hdr->defsec = cursec = addsection("DEFAULT", hdr);
	if (!cursec) goto error;
	
	h_ext[0] = 0;
	fdh_ext_dot[0] = 0;
	
	while(1)
	{
		if (feof(fp))
		{
			printf("ML_ReadHeader: unexpected end of file while parsing header.\n");
			goto error;
		}
		
		fgetline(fp, line, sizeof(line));
		if (!line[0]) continue;
		if (line[0] == '#' || line[0] == ';') continue;
		
		//printf("'%s': '%s'\n", cursec->name, line);
		
		
		if (line[0]=='<' || line[0] ==':')			// start of new header section
		{
			if (!(cursec = Hdr_FindSection(hdr, &line[1])))
			{
				cursec = addsection(&line[1], hdr);
				if (!cursec) goto error;
			}
		}
		else if (line[0]=='>')		// end of header
		{
			break;
		}
		else
		{	// split up assignment at '=' sign
			char *assign, *valptr, *value;
			
			valptr = strchr(line, '=');
			if (!valptr)
			{
				printf("header parse error: no '=' on line '%s'\n", line);
				goto error;
			}
			
			*valptr = 0; valptr++;
			assign = line;
			
			if (!(value = RemoveQuotes(valptr))) goto error;
			
			if ((ok = AssignValue(assign, value, cursec)) != 1)
			{
				if (ok==-1) goto error;
				
				if ((ok = AssignGlobalValue(assign, value, cursec)) != 1)
				{
					if (ok==-1) goto error;
					printf(" ** unknown header assignment '%s' = '%s' in file '%s'\n", assign, value, mlname);
					goto error;
				}
				else if (cursec != hdr->defsec)
				{
					printf(" ** header variable '%s' has no effect outside default section\n", assign);
					goto error;
				}
			}
		}
	}
	
	// assign defaults to values that didn't get set
	if (hdr->defsec->Link_Usefiles && !hdr->defsec->LinkInvoke)
	{
		printf(" * error: LINK_USEFILES=1 but no LINK_INVOKE string\n");
		goto error;
	}
	
	if (!hdr->defsec->LinkPrefix)
	{
		if (!(hdr->defsec->LinkPrefix = tokenize_const("gcc -o %EXEFILE%"))) goto error;
	}
	
	if (!hdr->defsec->Compile)
	{
		if (!(hdr->defsec->Compile = tokenize_const("gcc -c %SRCFILE% -o %MODULE%%OBJ_EXT%"))) goto error;
	}
	
	if (hdr->defsec->Compile_Usefiles==-1)
		hdr->defsec->Compile_Usefiles = 0;
	
	if (!hdr->defsec->exe[0])
		strcpy(hdr->defsec->exe, "a.out");

	if (!h_ext[0]) strcpy(h_ext, "h");
	if (!fdh_ext_dot[0])
	{
		strcpy(fdh_ext_dot, ".fdh");
	}
	
//printf("==>'%s'\n",hdr->defsec->cc);
	fdh_ext = &fdh_ext_dot[1];
	
	// do finishing processing on all sections
	cursec = hdr->firstsec;
	while(cursec)
	{
		if (FinishSection(cursec, hdr)) goto error;
		cursec = cursec->next;
	}
	
	return hdr;

error: ;
	printf("ML_ReadHeader: operation failed\n");
	ML_FreeHeader(hdr);
	return NULL;
}



static char AssignValue(char *assign, char *value, ml_header_section *h)
{
char ok;

	ok = try_assign(&h->Compile, "COMPILE", assign, value);
	if (ok) return ok;
	
	ok = try_assign(&h->CompileInvoke, "COMPILE_INVOKE", assign, value);
	if (ok) return ok;
	
	
	if (!strcasecmp(assign, "COMPILE_USEFILES"))
	{
		h->Compile_Usefiles = (value[0] != '0') ? 1:0;
	}
	else if (!strcasecmp(assign, "NOPARSE"))
	{
		h->noparse = (value[0] != '0') ? 1:0;
	}
	else if (!strcasecmp(assign, "FORCE"))
	{
		h->forcebuild = (value[0] != '0') ? 1:0;
	}
	else if (!strcasecmp(assign, "DO_NOT_LINK"))
	{
		h->do_not_link = (value[0] != '0') ? 1:0;
	}
	else if (!strcasecmp(assign, "obj_ext"))
	{
		validate_ext(value);
		strcpy(h->obj_ext, &value[1]);		// do NOT include the dot
	}
	else if (!strcasecmp(assign, "CC"))
	{
		strcpy(h->cc, value);
	}
	else if (!strcasecmp(assign, "ARCH_AWARE") ||
		!strcasecmp(assign, "HOST_AWARE"))
	{
		h->arch_aware = (value[0] != '0') ? 1:0;
	}
	else
	{
		return 0;
	}
	
	return 1;
}


const char *get_architecture_name(void)
{
static char tempbuffer[1024];

	FILE *fp = popen("uname -m", "r");
	if (!fp)
	{
		fprintf(stderr, "Could not determine system architecture!\n");
		return NULL;
	}
	
	fgetline(fp, tempbuffer, sizeof(tempbuffer) - 1);
	pclose(fp);
	
	return tempbuffer;
}

// resolve any format specifiers in the OUTPUT= header directive
static char *resolve_OUTPUT_format_specifiers(const char *in)
{
	//printf("resolve_OUTPUT_format_specifiers('%s')\n", in);
	char *value = strdup(in);
	
	// support the %p specifier, which will replace itself with the CPU architecture of the system
	char *ptr = strstr(value, "%p");
	if (ptr)
	{
		// get architecture name
		const char *arch = get_architecture_name();
		if (arch)
		{
			int index = (ptr - value);
			char *result = malloc(strlen(arch) + strlen(value) + 1);
			
			int j = 0;
			memcpy(&result[j], value, index); j += index;
			memcpy(&result[j], arch, strlen(arch)); j += strlen(arch);
			strcpy(&result[j], ptr+2);
			
			free(value);
			value = result;
		}
	}
	
	//printf("** resolve_OUTPUT_format_specifiers('%s'): returning '%s' ********\n", in, value);
	return value;
}


static char AssignGlobalValue(char *assign, char *value, ml_header_section *h)
{
char ok;

	ok = try_assign(&h->LinkPrefix, "LPREFIX", assign, value);
	if (ok) return ok;
	
	ok = try_assign(&h->LinkSuffix, "LSUFFIX", assign, value);
	if (ok) return ok;
	
	ok = try_assign(&h->LinkInvoke, "LINK_INVOKE", assign, value);
	if (ok) return ok;
	
	
	if (/*!h->exe[0] &&*/ !strcasecmp(assign, "OUTPUT"))
	{
		char *ptr, *val = resolve_OUTPUT_format_specifiers(value);
		strcpy(h->exe, val);
		if ((ptr = strrchr(val, '.')))	// strip .exe extension if present
		{
			*ptr = 0;
		}
		strcpy(h->exemodule, val);
		free(val);
	}
	else if (!strcasecmp(assign, "LINK_USEFILES"))
	{
		h->Link_Usefiles = (value[0] != '0');
	}
	else if (!strcasecmp(assign, "ARCH_AWARE") ||
		!strcasecmp(assign, "HOST_AWARE"))
	{
		h->arch_aware = (value[0] != '0');
	}
	else if (!strcasecmp(assign, "NO_STATERR"))
	{
		No_Staterr = (value[0] != '0');
	}
/*	else if (!strcasecmp(assign, "IGNOREFILEPATHS"))
	{
		h->IgnoreFilePaths = (value[0] != '0') ? 1:0;
	}*/
	else if (!strcasecmp(assign, "INCLUDE_EXT"))
	{
		validate_ext(value);
		strcpy(h_ext, &value[1]);		// h_ext does NOT include a dot
	}
	else if (!strcasecmp(assign, "fdh_ext_dot"))
	{
		validate_ext(value);
		strcpy(fdh_ext_dot, value);
	}
	else if (!strcasecmp(assign, "BUILDSTAMP"))
	{
		// get timestamp
		static char timestamp[1024];
		time_t ts = time(NULL);
		struct tm *tm = localtime(&ts);
		strftime(timestamp, sizeof(timestamp), "%m/%d/%Y %I:%M:%S%p", tm);
		
		printf("Updating buildstamp file %s: \"%s\"\n", value, timestamp);
		FILE *fp = fopen(value, "wb");
		if (!fp)
		{
			printf("failed to open buildstamp file %s\n", value);
			return 1;
		}
		
		fprintf(fp, "\nconst char *BUILDSTAMP = \"%s\";\n\n", timestamp);
		fclose(fp);
	}
	else
	{
		return 0;
	}
	
	return 1;
}


static char try_assign(token **dest, char *assignlook, char *assign, char *value)
{
	if (!*dest && !strcasecmp(assign, assignlook))
	{
		*dest = tokenize(value);
		if (!*dest) return -1;
		return 1;
	}
	return 0;
}


ml_header_section *addsection(char *name, ml_header *hdr)
{
ml_header_section *h;

	h = malloc(sizeof(ml_header_section));
	if (!h)
	{
		printf("addsection: out of memory adding header section '%s'!\n", name);
		return NULL;
	}
	
	memset(h, 0, sizeof(ml_header_section));
	strcpy(h->name, name);
	
	h->Compile_Usefiles = -1;
	h->Link_Usefiles = 0;
	//h->IgnoreFilePaths = 0;
	h->noparse = 0;
	h->forcebuild = 0;
	h->do_not_link = 0;
	h->arch_aware = 0;
	
	h->exe[0] = 0;
        strcpy(h->obj_ext, "o");

	{
		char host[1024];
		gethostname(host, sizeof(host));

		if (!strcmp(host, "frubbles"))
			strcpy(h->cc, "clang++");
		else
			strcpy(h->cc, "gcc");

		h->hostflags[0] = 0;
		if (!strcmp(host, "vinnie")) strcpy(h->hostflags, "-Wno-psabi");
	}

	h->Compile = NULL;
	h->CompileInvoke = NULL;
	h->LinkPrefix = NULL;
	h->LinkSuffix = NULL;
	
	h->next = hdr->firstsec;
	hdr->firstsec = h;
	return h;
}


static char FinishSection(ml_header_section *h, ml_header *hdr)
{
	//printf("Finishing '%s'\n", h->name);
	
	// inherit unset values from default
	if (!h->Compile) h->Compile = hdr->defsec->Compile;
	if (!h->CompileInvoke) h->CompileInvoke = hdr->defsec->CompileInvoke;
	
	if (h->Compile_Usefiles == -1)
		h->Compile_Usefiles = hdr->defsec->Compile_Usefiles;
	
	if (!h->obj_ext[0])
		strcpy(h->obj_ext, hdr->defsec->obj_ext);
	
	if (h->Compile_Usefiles && !h->CompileInvoke)
	{
		printf(" * error: COMPILE_USEFILES=1 but no COMPILE_INVOKE string\n");
		return 1;
	}
	
	return 0;
}


void ML_FreeHeader(ml_header *hdr)
{
ml_header_section *h = hdr->firstsec;
ml_header_section *next;

	while(h)
	{
		next = h->next;
		free(h);
		h = next;
	}
	free(hdr);
}


ml_header_section *Hdr_FindSection(ml_header *hdr, char *name)
{
ml_header_section *h = hdr->firstsec;

	while(h)
	{
		if (!strcasecmp(h->name, name)) break;
		h = h->next;
	}
	return h;
}

