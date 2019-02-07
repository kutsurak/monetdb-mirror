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

#include "39_analytics_hge.sql.h"
#include "gdk.h"
#include "mal_client.h"
#include "sql_catalog.h"

#include <assert.h>

extern str SQLstatementIntern(Client c, str *expr, str nme, bit execute, bit output, res_table **result);

str
sql_install_39_analytics_hge(Client c)
{
	size_t bufsize = 16384, pos = 0;
	char *buf = GDKmalloc(bufsize), *err = NULL;

	if (buf == NULL)
		throw(SQL, "sql.install_39_analytics_hge", SQLSTATE(HY001) MAL_MALLOC_FAIL);
	pos += snprintf(buf + pos, bufsize - pos, " create aggregate stddev_samp(val HUGEINT) returns DOUBLE external name \"aggr\".\"stdev\"; GRANT EXECUTE ON AGGREGATE stddev_samp(HUGEINT) TO PUBLIC; create aggregate stddev_pop(val HUGEINT) returns DOUBLE external name \"aggr\".\"stdevp\"; GRANT EXECUTE ON AGGREGATE stddev_pop(HUGEINT) TO PUBLIC; create aggregate var_samp(val HUGEINT) returns DOUBLE external name \"aggr\".\"variance\"; GRANT EXECUTE ON AGGREGATE var_samp(HUGEINT) TO PUBLIC; create aggregate var_pop(val HUGEINT) returns DOUBLE external name \"aggr\".\"variancep\"; GRANT EXECUTE ON AGGREGATE var_pop(HUGEINT) TO PUBLIC; create aggregate median(val HUGEINT) returns HUGEINT external name \"aggr\".\"median\"; GRANT EXECUTE ON AGGREGATE median(HUGEINT) TO PUBLIC; create aggregate quantile(val HUGEINT, q DOUBLE) returns HUGEINT external name \"aggr\".\"quantile\"; GRANT EXECUTE ON AGGREGATE quantile(HUGEINT, DOUBLE) TO PUBLIC; create aggregate corr(e1 HUGEINT, e2 HUGEINT) returns DOUBLE external name \"aggr\".\"corr\"; GRANT EXECUTE ON AGGREGATE corr(HUGEINT, HUGEINT) TO PUBLIC;");

	pos += snprintf(buf + pos, bufsize - pos, "commit;\n");

	assert(pos < bufsize);
	printf("#Loading: 39_analytics_hge.sql\n");
	err = SQLstatementIntern(c, &buf, "install", 1, 0, NULL);
	GDKfree(buf);
	return err;
}
