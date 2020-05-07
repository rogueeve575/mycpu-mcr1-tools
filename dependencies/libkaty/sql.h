
#include "main.h"
#include <mysql.h>
#include "sql.fdh"

static MYSQL *sql;

bool sql_init(void)
{
	stat("Opening MySQL connection...");
	sql = mysql_init(NULL);
	if (!sql)
	{
		staterr("mysql_init failed: %s", mysql_error(NULL));
		return 1;
	}
	
	if (!mysql_real_connect(sql, settings.sql_host, settings.sql_user, \
						settings.sql_pass, NULL, 0, NULL, 0))
	{
		staterr("mysql_real_connect failed: %s", mysql_error(sql));
		return 1;
	}
	
	if (mysql_select_db(sql, settings.sql_db))
	{
		staterr("mysql_select_db failed: %s", mysql_error(sql));
		return 1;
	}
	
	stat("sql_init: connected to %s as %s", settings.sql_host, settings.sql_user);
	return 0;
}

void sql_close(void)
{
	if (sql)
	{
		mysql_close(sql);
		sql = NULL;
	}
}

