#ifndef _EMAIL_H
#define _EMAIL_H

bool send_email(const char *listname, const char *subj, const char *body = NULL);
bool send_email2(const char *listname, const char *subj, const char *body = NULL, const char *through_hostname = NULL);

#endif
