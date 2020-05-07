#include "katy.h"
#include "email.fdh"

bool send_email(const char *listname, const char *subj, const char *body)
{
	return send_email2(listname, subj, body, NULL);
}

bool send_email2(const char *listname, const char *subj, const char *body,
		const char *through_hostname)
{
DString body2;
char *recps = NULL;

	if (listname == NULL)
		recps = get_recipient_list("default");
	else if (strchr(listname, '@'))
		recps = strdup(listname);
	else
		recps = get_recipient_list(listname);

	if (!recps)
	{
		staterr("no recipients found");
		return 1;
	}

	if (!body)
	{
		body2.AppendString("As of ");
		body2.AppendString(GetTimestamp());
		body2.AppendString(":\r\n");
		body2.AppendString(subj);

		body = body2.String();
	}

	stat("[email]: <%s>", subj);

	static int x = randrange(0, 65535);
	static int z = 0;
	char tempfname[1024];
	sprintf(tempfname, "/dev/shm/mail-%d-%d-%d", randrange(0, 65535), x, z++);
	__sync_fetch_and_add(&z, 1);

	FILE *fp = fopen(tempfname, "wb");
	if (!fp)
	{
		staterr("failed to open %s", tempfname);
		return 1;
	}

	fprintf(fp, "%s", body);
	fclose(fp);

	DString cmd;
	if (through_hostname)
	{
		cmd.AppendString("ssh ");
		cmd.AppendString(through_hostname);
		cmd.AppendChar(' ');
	}

	cmd.AppendString(stprintf("cat %s | mailx -s \"%s\" %s",
		tempfname, subj, recps));
	staterr("executing: %s", cmd.String());
	int result = system(cmd.String());

	remove(tempfname);
	free(recps);
	return result;
}

static char *get_recipient_list(const char *listname)
{
	char *list = do_get_recipient_list(listname);
	if (!list)
	{
		staterr("failed to load email recipient list '%s'", listname);
		return NULL;
	}

	return list;
}

static char *do_get_recipient_list(const char *listname)
{
DString recps;

	FILE *fp = fopen("/etc/emails", "rb");
	if (!fp)
	{
		staterr("%s: not found", "/etc/emails");
		return NULL;
	}

	bool read_on = 0;
	char line[1024];
	while(!feof(fp))
	{
		fgetline(fp, line, sizeof(line));
		if (!line[0]) continue;
		if (line[0] == '#') continue;

		if (line[0] == '[')
		{
			if (read_on) break;
			if (!strcmp(line, stprintf("[%s]", listname)))
			{
				read_on = true;
			}
		}
		else if (read_on && strchr(line, '@'))
		{
			if (recps.Length()) recps.AppendString(" ");
			recps.AppendString(line);
		}
	}

	fclose(fp);

	if (recps.Length() == 0)
	{
		staterr("not-found or empty recipient list: '%s'", listname);
		return NULL;
	}

	return recps.TakeString();
}
