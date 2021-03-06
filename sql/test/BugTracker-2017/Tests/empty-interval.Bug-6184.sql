START TRANSACTION;

CREATE TABLE "sys"."unitTestDontDelete" (
	"A" VARCHAR(255),
	"B" BIGINT,
	"C" DOUBLE,
	"D" TIMESTAMP
);
COPY 10 RECORDS INTO "sys"."unitTestDontDelete" FROM stdin USING DELIMITERS E'\t',E'\n','"';
NULL	NULL	NULL	NULL
"Cat1"	0	0.5	2013-06-10 11:10:10.000000
"Cat2"	1	1.5	2013-06-11 12:11:11.000000
"Cat1"	2	2.5	2013-06-12 13:12:12.000000
"Cat2"	3	3.5	2013-06-13 14:13:13.000000
"Cat1"	4	4.5	2013-06-14 15:14:14.000000
"Cat2"	5	5.5	2013-06-15 16:15:15.000000
"Cat1"	6	6.5	2013-06-16 17:16:16.000000
"Cat2"	7	7.5	2013-06-17 18:17:17.000000
"Cat1"	8	8.5	2013-06-18 19:18:18.000000

select * from "unitTestDontDelete" where "B" < 5 AND "B" >= 5;
select * from "unitTestDontDelete" where "A" < 'Cat2' AND "A" >= 'Cat2';
select * from "unitTestDontDelete" where "C" < 5.5 AND "C" >= 5.5;

ROLLBACK;
