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

#include "23_skyserver.sql.h"
#include "gdk.h"
#include "mal_client.h"
#include "sql_catalog.h"

#include <assert.h>

extern str SQLstatementIntern(Client c, str *expr, str nme, bit execute, bit output, res_table **result);

str
sql_install_23_skyserver(Client c, char *buf, size_t bufsize)
{
	size_t pos = 0;

	pos += snprintf(buf, bufsize, " CREATE FUNCTION MS_STUFF( s1 varchar(32), st int, len int, s3 varchar(32)) RETURNS varchar(32) BEGIN DECLARE res varchar(32), aux varchar(32); DECLARE ofset int; IF ( st < 0 or st > LENGTH(s1)) THEN RETURN ''; END IF; SET ofset = 1; SET res = SUBSTRING(s1,ofset,st-1); SET res = res || s3; SET ofset = st + len; SET aux = SUBSTRING(s1,ofset,LENGTH(s1)-ofset+1); SET res = res || aux; RETURN res; END; grant execute on function MS_STUFF to public; CREATE FUNCTION MS_TRUNC(num double, prc int) RETURNS double external name sql.ms_trunc; grant execute on function MS_TRUNC to public; CREATE FUNCTION MS_ROUND(num double, prc int, truncat int) RETURNS double BEGIN IF (truncat = 0) THEN RETURN ROUND(num, prc); ELSE RETURN MS_TRUNC(num, prc); END IF; END; grant execute on function MS_ROUND to public; CREATE FUNCTION MS_STR(num float, prc int, truncat int) RETURNS string BEGIN RETURN CAST(num as string); END; grant execute on function MS_STR to public; CREATE FUNCTION alpha(pdec double, pradius double) RETURNS double EXTERNAL NAME sql.alpha; grant execute on function alpha to public;");

	pos += snprintf(buf + pos, bufsize - pos, "commit;\n");

	assert(pos < bufsize);
	printf("#Loading: 23_skyserver.sql\n");
	return SQLstatementIntern(c, &buf, "install", 1, 0, NULL);
}
