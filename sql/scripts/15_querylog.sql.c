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

#include "15_querylog.sql.h"
#include "gdk.h"
#include "mal_client.h"
#include "sql_catalog.h"

#include <assert.h>

extern str SQLstatementIntern(Client c, str *expr, str nme, bit execute, bit output, res_table **result);

str
sql_install_15_querylog(Client c)
{
	size_t bufsize = 16384, pos = 0;
	char *buf = GDKmalloc(bufsize), *err = NULL;

	if (buf == NULL)
		throw(SQL, "sql.install_15_querylog", SQLSTATE(HY001) MAL_MALLOC_FAIL);
	pos += snprintf(buf + pos, bufsize - pos, "  create function sys.querylog_catalog() returns table( id oid, owner string, defined timestamp, query string, pipe string, \"plan\" string,		mal int,		optimize bigint	) external name sql.querylog_catalog;  create function sys.querylog_calls() returns table( id oid,		\"start\" timestamp,		\"stop\" timestamp,		arguments string,		tuples bigint,		run bigint,		ship bigint,		cpu int,		io int	) external name sql.querylog_calls; create view sys.querylog_catalog as select * from sys.querylog_catalog(); create view sys.querylog_calls as select * from sys.querylog_calls(); create view sys.querylog_history as select qd.*, ql.\"start\",ql.\"stop\", ql.arguments, ql.tuples, ql.run, ql.ship, ql.cpu, ql.io from sys.querylog_catalog() qd, sys.querylog_calls() ql where qd.id = ql.id and qd.owner = user; create procedure sys.querylog_empty() external name sql.querylog_empty; create procedure sys.querylog_enable() external name sql.querylog_enable; create procedure sys.querylog_enable(threshold smallint) external name sql.querylog_enable_threshold; create procedure sys.querylog_disable() external name sql.querylog_disable;");

	pos += snprintf(buf + pos, bufsize - pos, "commit;\n");

	assert(pos < bufsize);
	printf("#Loading: 15_querylog.sql\n");
	err = SQLstatementIntern(c, &buf, "install", 1, 0, NULL);
	GDKfree(buf);
	return err;
}
