/*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0.  If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.
*/

// This file was generated automatically through bootstrap.py.
// Do not edit this file directly.

#include "monetdb_config.h"

#include "17_temporal.sql.h"
#include "gdk.h"
#include "mal_client.h"
#include "sql_catalog.h"

#include <assert.h>

extern str SQLstatementIntern(Client c, str *expr, str nme, bit execute, bit output, res_table **result);

str
sql_install_17_temporal(Client c, char *buf, size_t bufsize)
{
	size_t pos = 0;

	pos += snprintf(buf, bufsize, " create function sys.\"epoch\"(sec BIGINT) returns TIMESTAMP external name timestamp.\"epoch\"; create function sys.\"epoch\"(sec INT) returns TIMESTAMP external name timestamp.\"epoch\"; create function sys.\"epoch\"(ts TIMESTAMP) returns INT external name timestamp.\"epoch\"; create function sys.\"epoch\"(ts TIMESTAMP WITH TIME ZONE) returns INT external name timestamp.\"epoch\"; grant execute on function sys.\"epoch\" (BIGINT) to public; grant execute on function sys.\"epoch\" (INT) to public; grant execute on function sys.\"epoch\" (TIMESTAMP) to public; grant execute on function sys.\"epoch\" (TIMESTAMP WITH TIME ZONE) to public;");

	pos += snprintf(buf + pos, bufsize - pos, "commit;\n");

	assert(pos < bufsize);
	printf("#Loading: 17_temporal.sql\n");
	return SQLstatementIntern(c, &buf, "install", 1, 0, NULL);
}
