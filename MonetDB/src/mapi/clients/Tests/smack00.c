#include <stdio.h>
#include <monet_utils.h>
#include <stream.h>
#include <Mapi.h>

#define die(X) {mapi_explain(X,stdout); exit(-1); }

int main(int argc, char **argv){
	Mapi	dbh;
	int i,port;
	char buf[40];
	/* char *line; */

	if( argc != 2){
		printf("usage:smack00 <port>\n");
		exit(-1);
	}
	port = atol(argv[1]);
	snprintf(buf,40,"localhost:%d",port);
	dbh= mapi_connect(buf,"guest",0,0);
	if(mapi_error(dbh)) die(dbh);

	for(i=0; i< 20000; i++){
		snprintf(buf,40,"print(%d);",i);
		if( mapi_query(dbh,buf)) die(dbh);
		while( /*line=*/ mapi_fetch_line(dbh)){
			/*printf("%s \n", line );*/
		}
	}
	mapi_disconnect(dbh);

	return 0;
}
