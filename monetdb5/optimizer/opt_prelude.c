/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 1997 - July 2008 CWI, August 2008 - 2018 MonetDB B.V.
 */

/*
 * opt_prelude
 * M. Kersten
 * These definitions are handy to have around in the optimizer
 */
#include "monetdb_config.h"
#include "opt_prelude.h"
#include "optimizer_private.h"

str abortRef;
str actionRef;
str affectedRowsRef;
str aggrRef;
str alarmRef;
str algebraRef;
str alter_add_tableRef;
str alter_constraintRef;
str alter_del_tableRef;
str alter_functionRef;
str alter_indexRef;
str alter_roleRef;
str alter_schemaRef;
str alter_seqRef;
str alter_set_tableRef;
str alter_tableRef;
str alter_triggerRef;
str alter_typeRef;
str alter_userRef;
str alter_viewRef;
str andRef;
str antijoinRef;
str appendidxRef;
str appendRef;
str arrayRef;
str assertRef;
str attachRef;
str avgRef;
str bandjoinRef;
str basketRef;
str batalgebraRef;
str batcalcRef;
str batmalRef;
str batmmathRef;
str batmtimeRef;
str batpyapi3Ref;
str batpyapiRef;
str batrapiRef;
str batRef;
str batsqlRef;
str batstrRef;
str batxmlRef;
str bbpRef;
str betweenRef;
str betweensymmetricRef;
str binddbatRef;
str bindidxRef;
str bindRef;
str blockRef;
str bpmRef;
str bstreamRef;
str calcRef;
str catalogRef;
str clear_tableRef;
str closeRef;
str columnBindRef;
str columnRef;
str comment_onRef;
str commitRef;
str connectRef;
str copy_fromRef;
str copyRef;
str count_no_nilRef;
str countRef;
str create_constraintRef;
str create_functionRef;
str create_indexRef;
str createRef;
str create_roleRef;
str create_schemaRef;
str create_seqRef;
str create_tableRef;
str create_triggerRef;
str create_typeRef;
str create_userRef;
str create_viewRef;
str crossRef;
str dataflowRef;
str dateRef;
str dblRef;
str defineRef;
str deleteRef;
str deltaRef;
str dense_rankRef;
str differenceRef;
str diffRef;
str disconnectRef;
str divRef;
str drop_constraintRef;
str drop_functionRef;
str drop_indexRef;
str drop_roleRef;
str drop_schemaRef;
str drop_seqRef;
str drop_tableRef;
str drop_triggerRef;
str drop_typeRef;
str drop_userRef;
str drop_viewRef;
str emptybindidxRef;
str emptybindRef;
str eqRef;
str evalRef;
str execRef;
str expandRef;
str exportOperationRef;
str export_tableRef;
str findRef;
str finishRef;
str firstnRef;
str generatorRef;
str getRef;
str getTraceRef;
str grant_functionRef;
str grantRef;
str grant_rolesRef;
str groupbyRef;
str groupdoneRef;
str groupRef;
str group_concatRef;
str hashRef;
str hgeRef;
str identityRef;
str ifthenelseRef;
str ilikeRef;
str ilikeselectRef;
str ilikethetaselectRef;
str inplaceRef;
str intersectcandRef;
str intersectRef;
str intRef;
str ioRef;
str iteratorRef;
str jitRef;
str joinRef;
str jsonRef;
str languageRef;
str leftjoinRef;
str likeRef;
str likeselectRef;
str likethetaselectRef;
str listRef;
str lockRef;
str lookupRef;
str malRef;
str manifoldRef;
str mapiRef;
str markRef;
str matRef;
str max_no_nilRef;
str maxRef;
str mdbRef;
str mergecandRef;
str mergepackRef;
str min_no_nilRef;
str minRef;
str minusRef;
str mirrorRef;
str mitosisRef;
str mkeyRef;
str mmathRef;
str mtimeRef;
str mulRef;
str multicolumnRef;
str multiplexRef;
str mvcRef;
str newRef;
str nextRef;
str not_ilikeRef;
str not_likeRef;
str notRef;
str not_uniqueRef;
str oidRef;
str oltpRef;
str openRef;
str optimizerRef;
str pack2Ref;
str packIncrementRef;
str packRef;
str parametersRef;
str partitionRef;
str passRef;
str pcreRef;
str pinRef;
str plusRef;
str postludeRef;
str preludeRef;
str printRef;
str prodRef;
str profilerRef;
str projectdeltaRef;
str projectionpathRef;
str projectionRef;
str projectRef;
str putRef;
str pyapi3mapRef;
str pyapi3Ref;
str pyapimapRef;
str pyapiRef;
str querylogRef;
str queryRef;
str capiRef;
str batcapiRef;
str subeval_aggrRef;
str rankRef;
str dense_rankRef;
str raiseRef;
str rangejoinRef;
str rankRef;
str rapiRef;
str reconnectRef;
str refineRef;
str registerRef;
str register_supervisorRef;
str releaseRef;
str remapRef;
str remoteRef;
str rename_userRef;
str replaceRef;
str replicatorRef;
str resultSetRef;
str reuseRef;
str revoke_functionRef;
str revokeRef;
str revoke_rolesRef;
str rollbackRef;
str row_numberRef;
str rpcRef;
str rsColumnRef;
str sampleRef;
str schedulerRef;
str selectNotNilRef;
str selectRef;
str semaRef;
str semijoinRef;
str seriesRef;
str setAccessRef;
str setVariableRef;
str setWriteModeRef;
str singleRef;
str sinkRef;
str sliceRef;
str sortRef;
str sortReverseRef;
str sqlcatalogRef;
str sqlRef;
str startRef;
str starttraceRef;
str stopRef;
str stoptraceRef;
str streamsRef;
str strRef;
str subavgRef;
str subcountRef;
str subdeltaRef;
str subdiffRef;
str subeval_aggrRef;
str subgroupdoneRef;
str subgroupRef;
str subinterRef;
str submaxRef;
str submedianRef;
str subminRef;
str subprodRef;
str subsliceRef;
str subsumRef;
str subuniformRef;
str sumRef;
str takeRef;
str thetajoinRef;
str thetaselectRef;
str tidRef;
str timestampRef;
str transaction_abortRef;
str transaction_beginRef;
str transaction_commitRef;
str transactionRef;
str transaction_releaseRef;
str transaction_rollbackRef;
str uniqueRef;
str unlockRef;
str unpackRef;
str unpinRef;
str updateRef;
str userRef;
str vectorRef;
str wlcRef;
str wlrRef;
str zero_or_oneRef;
str vaultRef;
str checktableRef;
str analyzetableRef;

void optimizerInit(void)
{
	abortRef = putName("abort");
	actionRef = putName("action");
	affectedRowsRef = putName("affectedRows");
	aggrRef = putName("aggr");
	alarmRef = putName("alarm");
	algebraRef = putName("algebra");
	andRef = putName("and");
	appendidxRef = putName("append_idxbat");
	appendRef = putName("append");
	assertRef = putName("assert");
	attachRef = putName("attach");
	alter_seqRef = putName("alter_seq");
	alter_schemaRef = putName("alter_schema");
	alter_viewRef = putName("alter_view");
	alter_tableRef = putName("alter_table");
	alter_constraintRef = putName("alter_constraint");
	alter_typeRef = putName("alter_type");
	alter_userRef = putName("alter_user");
	alter_roleRef = putName("alter_role");
	alter_userRef = putName("alter_user");
	alter_indexRef = putName("alter_index");
	alter_functionRef = putName("alter_function");
	alter_triggerRef = putName("alter_trigger");
	alter_add_tableRef = putName("alter_add_table");
	alter_del_tableRef = putName("alter_del_table");
	alter_set_tableRef = putName("alter_set_table");
	avgRef = putName("avg");
	arrayRef = putName("array");
	batRef = putName("bat");
	batalgebraRef = putName("batalgebra");
	batcalcRef = putName("batcalc");
	basketRef = putName("basket");
	batstrRef = putName("batstr");
	batmtimeRef = putName("batmtime");
	batmmathRef = putName("batmmath");
	batxmlRef = putName("batxml");
	batsqlRef = putName("batsql");
	betweenRef = putName("between");
	betweensymmetricRef = putName("betweensymmetric");
	blockRef = putName("block");
	bbpRef = putName("bbp");
	tidRef = putName("tid");
	deltaRef = putName("delta");
	subdeltaRef = putName("subdelta");
	projectdeltaRef = putName("projectdelta");
	binddbatRef = putName("bind_dbat");
	bindidxRef = putName("bind_idxbat");
	bindRef = putName("bind");
	emptybindRef = putName("emptybind");
	emptybindidxRef = putName("emptybindidx");
	bpmRef = putName("bpm");
	bstreamRef = putName("bstream");
	calcRef = putName("calc");
	catalogRef = putName("catalog");
	clear_tableRef = putName("clear_table");
	closeRef = putName("close");
	columnRef = putName("column");
	columnBindRef = putName("columnBind");
	comment_onRef = putName("comment_on");
	commitRef = putName("commit");
	connectRef = putName("connect");
	countRef = putName("count");
	subcountRef = putName("subcount");
	copyRef = putName("copy");
	copy_fromRef = putName("copy_from");
	export_tableRef = putName("export_table");
	count_no_nilRef = putName("count_no_nil");
	crossRef = putName("crossproduct");
	createRef = putName("create");
	create_seqRef = putName("create_seq");
	create_schemaRef = putName("create_schema");
	create_viewRef = putName("create_view");
	create_tableRef = putName("create_table");
	create_constraintRef = putName("create_constraint");
	create_typeRef = putName("create_type");
	create_userRef = putName("create_user");
	create_roleRef = putName("create_role");
	create_userRef = putName("create_user");
	create_indexRef = putName("create_index");
	create_functionRef = putName("create_function");
	create_triggerRef = putName("create_trigger");
	dateRef = putName("date");
	dataflowRef = putName("dataflow");
	dblRef = putName("dbl");
	defineRef = putName("define");
	deleteRef = putName("delete");
	differenceRef = putName("difference");
	intersectRef = putName("intersect");
	drop_seqRef = putName("drop_seq");
	drop_schemaRef = putName("drop_schema");
	drop_viewRef = putName("drop_view");
	drop_tableRef = putName("drop_table");
	drop_constraintRef = putName("drop_constraint");
	drop_typeRef = putName("drop_type");
	drop_userRef = putName("drop_user");
	drop_roleRef = putName("drop_role");
	drop_userRef = putName("drop_user");
	drop_indexRef = putName("drop_index");
	drop_functionRef = putName("drop_function");
	drop_triggerRef = putName("drop_trigger");
	mergecandRef= putName("mergecand");
	mergepackRef= putName("mergepack");
	intersectcandRef= putName("intersectcand");
	eqRef = putName("==");
	disconnectRef= putName("disconnect");
	evalRef = putName("eval");
	execRef = putName("exec");
	expandRef = putName("expand");
	exportOperationRef = putName("exportOperation");
	findRef = putName("find");
	finishRef = putName("finish");
	firstnRef = putName("firstn");
	getRef = putName("get");
	getTraceRef = putName("getTrace");
	generatorRef = putName("generator");
	grantRef = putName("grant");
	grant_rolesRef = putName("grant_roles");
	grant_functionRef = putName("grant_function");
	groupRef = putName("group");
	groupdoneRef = putName("groupdone");
	group_concatRef = putName("group_concat");
	subgroupRef = putName("subgroup");
	subgroupdoneRef= putName("subgroupdone");
	groupbyRef = putName("groupby");
	hgeRef = putName("hge");
	hashRef = putName("hash");
	identityRef = putName("identity");
	ifthenelseRef = putName("ifthenelse");
	inplaceRef = putName("inplace");
	intRef = putName("int");
	ioRef = putName("io");
	iteratorRef = putName("iterator");
	projectionpathRef = putName("projectionpath");
	joinRef = putName("join");
	semijoinRef = putName("semijoin");
	leftjoinRef = putName("leftjoin");
	antijoinRef = putName("antijoin");
	bandjoinRef = putName("bandjoin");
	rangejoinRef = putName("rangejoin");
	thetajoinRef = putName("thetajoin");
	jitRef = putName("jit");
	jsonRef = putName("json");
	languageRef= putName("language");
	projectionRef = putName("projection");
	likeselectRef = putName("likeselect");
	listRef = putName("list");
	likeRef = putName("like");
	ilikeRef = putName("ilike");
	ilikeselectRef = putName("ilikeselect");
	likethetaselectRef = putName("likethetaselect");
	ilikethetaselectRef = putName("ilikethetaselect");
	not_likeRef = putName("not_like");
	not_ilikeRef = putName("not_ilike");
	lockRef = putName("lock");
	lookupRef = putName("lookup");
	malRef = putName("mal");
	batmalRef = putName("batmal");
	mapiRef = putName("mapi");
	markRef = putName("mark");
	mtimeRef = putName("mtime");
	multicolumnRef = putName("multicolumn");
	matRef = putName("mat");
	max_no_nilRef = putName("max_no_nil");
	maxRef = putName("max");
	submaxRef = putName("submax");
	submedianRef = putName("submedian");
	mdbRef = putName("mdb");
	min_no_nilRef = putName("min_no_nil");
	minRef = putName("min");
	subminRef = putName("submin");
	mirrorRef = putName("mirror");
	mitosisRef = putName("mitosis");
	mkeyRef = putName("mkey");
	mmathRef = putName("mmath");
	multiplexRef = putName("multiplex");
	manifoldRef = putName("manifold");
	mvcRef = putName("mvc");
	newRef = putName("new");
	notRef = putName("not");
	nextRef = putName("next");
	oltpRef = putName("oltp");
	oidRef = putName("oid");
	optimizerRef = putName("optimizer");
	openRef = putName("open");
	parametersRef = putName("parameters");
	packRef = putName("pack");
	pack2Ref = putName("pack2");
	packIncrementRef = putName("packIncrement");
	passRef = putName("pass");
	partitionRef = putName("partition");
	pcreRef = putName("pcre");
	pinRef = putName("pin");
	plusRef = putName("+");
	minusRef = putName("-");
	mulRef = putName("*");
	divRef = putName("/");
	printRef = putName("print");
	preludeRef = putName("prelude");
	prodRef = putName("prod");
	subprodRef = putName("subprod");
	profilerRef = putName("profiler");
	postludeRef = putName("postlude");
	projectRef = putName("project");
	putRef = putName("put");
	querylogRef = putName("querylog");
	queryRef = putName("query");
	rapiRef = putName("rapi");
	batrapiRef = putName("batrapi");
    pyapiRef = putName("pyapi");
    batpyapiRef = putName("batpyapi");
    capiRef = putName("capi");
    batcapiRef = putName("batcapi");
    pyapimapRef = putName("batpyapimap");
    pyapi3Ref = putName("pyapi3");
    batpyapi3Ref = putName("batpyapi3");
    pyapi3mapRef = putName("batpyapi3map");
	subeval_aggrRef = putName("subeval_aggr");
	rankRef = putName("rank");
	dense_rankRef = putName("dense_rank");
	raiseRef = putName("raise");
	reconnectRef = putName("reconnect");
	refineRef = putName("refine");
	registerRef = putName("register");
	register_supervisorRef = putName("register_supervisor");
	releaseRef = putName("release");
	remapRef = putName("remap");
	remoteRef = putName("remote");
	rename_userRef = putName("rename_user");
	replaceRef = putName("replace");
	replicatorRef = putName("replicator");
	resultSetRef = putName("resultSet");
	revokeRef = putName("revoke");
	reuseRef = putName("reuse");
	revoke_rolesRef = putName("revoke_roles");
	rollbackRef = putName("rollback");
	revoke_functionRef = putName("revoke_function");
	row_numberRef = putName("row_number");
	rpcRef = putName("rpc");
	rsColumnRef = putName("rsColumn");
	schedulerRef = putName("scheduler");
	selectNotNilRef = putName("selectNotNil");
	seriesRef = putName("series");
	semaRef = putName("sema");
	setAccessRef = putName("setAccess");
	setVariableRef = putName("setVariable");
	setWriteModeRef= putName("setWriteMode");
	sinkRef = putName("sink");
	sliceRef = putName("slice");
	subsliceRef = putName("subslice");
	singleRef = putName("single");
	sortRef = putName("sort");
	sortReverseRef = putName("sortReverse");
	sqlRef = putName("sql");
	sqlcatalogRef = putName("sqlcatalog");
	streamsRef = putName("streams");
	startRef = putName("start");
	starttraceRef = putName("starttrace");
	stopRef = putName("stop");
	stoptraceRef = putName("stoptrace");
	strRef = putName("str");
	sumRef = putName("sum");
	subsumRef = putName("subsum");
	subavgRef = putName("subavg");
	sortRef = putName("sort");
	takeRef= putName("take");
	transactionRef= putName("transaction");
	transaction_beginRef= putName("transaction_begin");
	transaction_releaseRef= putName("transaction_release");
	transaction_commitRef= putName("transaction_commit");
	transaction_abortRef= putName("transaction_abort");
	transaction_rollbackRef= putName("transaction_rollback");
	timestampRef = putName("timestamp");
	not_uniqueRef= putName("not_unique");
	sampleRef= putName("sample");
	uniqueRef= putName("unique");
	subuniformRef= putName("subuniform");
	unlockRef= putName("unlock");
	unpackRef = putName("unpack");
	unpinRef = putName("unpin");
	updateRef = putName("update");
	userRef = putName("user");
	wlcRef = putName("wlc");
	wlrRef = putName("wlr");
	selectRef = putName("select");
	thetaselectRef = putName("thetaselect");
	vectorRef = putName("vector");
	zero_or_oneRef = putName("zero_or_one");
	vaultRef = putName("vault");
	checktableRef = putName("checktable");
	analyzetableRef = putName("analyzetable");
}
