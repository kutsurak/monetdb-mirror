# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 1997 - July 2008 CWI, August 2008 - 2020 MonetDB B.V.

sed '/^$/q' $0			# copy copyright from this file

cat <<EOF
# This file was generated by using the script ${0##*/}.

module sql;

EOF

cat <<EOF
pattern sql.window_bound(b:any_1, unit:int, bound:int, excl:int, start:hge) :lng
address SQLwindow_bound
comment "computes window ranges for each row";

pattern batsql.window_bound(b:bat[:any_1], unit:int, bound:int, excl:int, start:hge) :bat[:lng]
address SQLwindow_bound
comment "computes window ranges for each row";

pattern sql.window_bound(p:bit, b:any_1, unit:int, bound:int, excl:int, start:hge) :lng
address SQLwindow_bound
comment "computes window ranges for each row";

pattern batsql.window_bound(p:bat[:bit], b:bat[:any_1], unit:int, bound:int, excl:int, start:hge) :bat[:lng]
address SQLwindow_bound
comment "computes window ranges for each row";

pattern batsql.window_bound(b:bat[:any_1], unit:int, bound:int, excl:int, start:bat[:hge]) :bat[:lng]
address SQLwindow_bound
comment "computes window ranges for each row";

pattern batsql.window_bound(p:bat[:bit], b:bat[:any_1], unit:int, bound:int, excl:int, start:bat[:hge]) :bat[:lng]
address SQLwindow_bound
comment "computes window ranges for each row";


EOF

for tp1 in 1:bte 2:sht 4:int 8:lng 16:hge; do
    for tp2 in 16:hge; do
	if [ ${tp1%:*} -le ${tp2%:*} -o ${tp1#*:} = ${tp2#*:} ]; then
	    cat <<EOF
pattern sql.sum(b:${tp1#*:}, s:lng, e:lng) :${tp2#*:}
address SQLsum
comment "return the sum of groups";

pattern batsql.sum(b:bat[:${tp1#*:}], s:bat[:lng], e:bat[:lng]) :bat[:${tp2#*:}]
address SQLsum
comment "return the sum of groups";

pattern sql.prod(b:${tp1#*:}, s:lng, e:lng) :${tp2#*:}
address SQLprod
comment "return the product of groups";

pattern batsql.prod(b:bat[:${tp1#*:}], s:bat[:lng], e:bat[:lng]) :bat[:${tp2#*:}]
address SQLprod
comment "return the product of groups";


EOF
	fi
    done
done

	cat <<EOF
pattern sql.avg(b:hge, s:lng, e:lng) :dbl
address SQLavg
comment "return the average of groups";

pattern batsql.avg(b:bat[:hge], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLavg
comment "return the average of groups";

pattern sql.stdev(b:hge, s:lng, e:lng) :dbl
address SQLstddev_samp
comment "return the standard deviation of groups";

pattern batsql.stdev(b:bat[:hge], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLstddev_samp
comment "return the standard deviation of groups";

pattern sql.stdevp(b:hge, s:lng, e:lng) :dbl
address SQLstddev_pop
comment "return the standard deviation of groups";

pattern batsql.stdevp(b:bat[:hge], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLstddev_pop
comment "return the standard deviation of groups";

pattern sql.variance(b:hge, s:lng, e:lng) :dbl
address SQLvar_samp
comment "return the variance of groups";

pattern batsql.variance(b:bat[:hge], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLvar_samp
comment "return the variance of groups";

pattern sql.variancep(b:hge, s:lng, e:lng) :dbl
address SQLvar_pop
comment "return the variance of groups";

pattern batsql.variancep(b:bat[:hge], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLvar_pop
comment "return the variance of groups";

pattern sql.median_avg(b:hge, s:lng, e:lng) :dbl
address SQLmedian_avg
comment "return the median value of groups with average in case of 2 values in the median";

pattern batsql.median_avg(b:bat[:hge], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLmedian_avg
comment "return the median value of groups with average in case of 2 values in the median";

pattern sql.covariance(b:hge, c:hge, s:lng, e:lng) :dbl
address SQLcovar_samp
comment "return the covariance sample value of groups";

pattern batsql.covariance(b:bat[:hge], c:hge, s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLcovar_samp
comment "return the covariance sample value of groups";

pattern batsql.covariance(b:hge, c:bat[:hge], s:lng, e:lng) :bat[:dbl]
address SQLcovar_samp
comment "return the covariance sample value of groups";

pattern batsql.covariance(b:bat[:hge], c:bat[:hge], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLcovar_samp
comment "return the covariance sample value of groups";

pattern sql.covariancep(b:hge, c:hge, s:lng, e:lng) :dbl
address SQLcovar_pop
comment "return the covariance population value of groups";

pattern batsql.covariancep(b:bat[:hge], c:hge, s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLcovar_pop
comment "return the covariance population value of groups";

pattern batsql.covariancep(b:hge, c:bat[:hge], s:lng, e:lng) :bat[:dbl]
address SQLcovar_pop
comment "return the covariance population value of groups";

pattern batsql.covariancep(b:bat[:hge], c:bat[:hge], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLcovar_pop
comment "return the covariance population value of groups";

EOF

for tp in flt dbl; do
	cat <<EOF
pattern sql.quantile_avg(b:hge, q:${tp}, s:lng, e:lng) :dbl
address SQLquantile_avg
comment "return a quantile average value of groups";

pattern batsql.quantile_avg(b:bat[:hge], q:${tp}, s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLquantile_avg
comment "return a quantile average value of groups";

pattern batsql.quantile_avg(b:hge, q:bat[:${tp}], s:lng, e:lng) :bat[:dbl]
address SQLquantile_avg
comment "return a quantile average value of groups";

pattern batsql.quantile_avg(b:bat[:hge], q:bat[:${tp}], s:bat[:lng], e:bat[:lng]) :bat[:dbl]
address SQLquantile_avg
comment "return a quantile average value of groups";

EOF
done
