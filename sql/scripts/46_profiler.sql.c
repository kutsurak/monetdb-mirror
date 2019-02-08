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

#include "46_profiler.sql.h"
#include "gdk.h"
#include "mal_client.h"
#include "sql_catalog.h"

#include <assert.h>

extern str SQLstatementIntern(Client c, str *expr, str nme, bit execute, bit output, res_table **result);

str
sql_install_46_profiler(Client c, char *buf, size_t bufsize)
{
	size_t pos = 0;

	pos += snprintf(buf, bufsize, " create schema profiler; create procedure profiler.start() external name profiler.\"start\"; create procedure profiler.stop() external name profiler.stop; create procedure profiler.setheartbeat(beat int) external name profiler.setheartbeat; create function profiler.getlimit() returns integer external name profiler.getlimit; create procedure profiler.setlimit(lim integer) external name profiler.setlimit;");

	pos += snprintf(buf + pos, bufsize - pos, "commit;\n");

	assert(pos < bufsize);
	printf("#Loading: 46_profiler.sql\n");
	return SQLstatementIntern(c, &buf, "install", 1, 0, NULL);
}
