#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// read data from a file until CR
void fgetline(FILE *fp, char *str, int maxlen)
{
	str[0] = 0;
	fgets(str, maxlen - 1, fp);
	
	// trim the CRLF that fgets appends
	for(int k=strlen(str)-1;k>=0;k--)
	{
		if (str[k] != 13 && str[k] != 10) break;
		str[k] = 0;
	}
}

void remove_str_from_str(const char *in, char *out, const char *str)
{
	const char *ptr = strstr(in, str);
	if (!ptr)
	{
		strcpy(out, in);
		return;
	}

	int strIndex = (ptr - in);
	int strLength = strlen(str);

	strcpy(out, in);
	strcpy(out + strIndex, in + strIndex + strLength);
}

void add_str_to_str(const char *in, char *out, int index, const char *str)
{
	strcpy(out, in);
	strcpy(out+index, str);
	strcpy(out+index+strlen(str), in+index);
}

bool set_uses_fpic(const char *fname, bool uses_fpic)
{
	FILE *fp = fopen(fname, "rb");
	if (!fp)
	{
		fprintf(stderr, "cannot open %s\n", fname);
		return 1;
	}

	FILE *fpo = fopen("makelist.ml.tmp", "wb");
	if (!fpo)
	{
		fprintf(stderr, "cannot open temp file\n");
		return 1;
	}

	char line[1024];
	while(!feof(fp))
	{
		fgetline(fp, line, sizeof(line));
		
		if (strstr(line, "COMPILE=") == line)
		{
			char without[1024];
			remove_str_from_str(line, without, "-fPIC ");

			char final[1024];
			if (!uses_fpic)
				strcpy(final, without);
			else
			{
				char *ptr = strstr(without, "g++ ");
				if (!ptr) { fprintf(stderr, "missing g++ token for adding fpic\n"); return 1; }
				int index = (ptr - without) + 4;
				add_str_to_str(without, final, index, "-fPIC ");
			}
			
			fprintf(fpo, "%s\n", final);
		}
		else
		{
			fprintf(fpo, "%s\n", line);
		}
	}

	fclose(fp);
	fclose(fpo);
	rename("makelist.ml.tmp", fname);
	return 0;
}

int main(int argc, char **argv)
{
	const char *appname = argv[0];
	const char *ptr = strrchr(appname, '/');
	if (ptr) appname = ptr + 1;

	// get processor architecture
	static const char *cmd = "uname -m";
	FILE *fp = popen(cmd, "r");
	if (!fp)
	{
		fprintf(stderr, "failed to run '%s'\n", cmd);
		return 1;
	}

	char arch[1024]; arch[0] = 0;
	fgetline(fp, arch, sizeof(arch));
	pclose(fp);

	if (arch[0] == 0)
	{
		fprintf(stderr, "%s: failed to retrieve system architecture\n", appname);
	}

	// check if architecture is one that doesn't need -fPIC for shared libs
	static const char *fpic_unnecessary_processors[] = { "i686", "i586", "i386", NULL };
	bool use_fpic = true;
	for(int i=0;;i++)
	{
		const char *proc = fpic_unnecessary_processors[i];
		if (!proc) break;

		if (strstr(arch, proc))
		{
			printf("%s: found architecture %s, which doesn't need -fPIC\n", appname, proc);
			use_fpic = false;
			break;
		}
	}

	if (use_fpic)
		printf("%s: architecture %s may need -fPIC\n", appname, arch);

	printf("%s: configuring makelist.ml to \e[1;36m%s\e[0m -fPIC\n", appname, use_fpic ? "include" : "omit");
	set_uses_fpic("makelist.ml", use_fpic);

	printf("\n");
	return 0;
}

