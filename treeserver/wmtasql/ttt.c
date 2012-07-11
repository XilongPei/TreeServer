int main( void )
{
    int i = 0;
	int j, k = 1;
	dFILE *df;
        bHEAD *bh;
	dFIELD *field[2];
	char   buff[4096];
	long   l, ll;

	HANDLE hsem;
	TS_COM_PROPS tscp1;

	strncpy(buff, "abcdefg", 10);
	strncpy(buff, "abcdefg", 3);

/*	ProcessAttach();
        field[0] = "BH";
        field[1] = NULL;
        bh = IndexBuild("lz0.dbf", field, "lz0", BTREE_FOR_CLOSEDBF);
        IndexClose(bh);
	ProcessDetach();
        return  0;

	ProcessAttach();
	df = dAwake("e:\\wmtasql\\ls.dbf", DOPENPARA);
	ProcessDetach();
	return  0;

	get1rec(df);
        dbtFromFile(df, 1, "e:\\wmtasql\\debug\\a1");
	put_fld(df, 0, "1");
	putrec(df);

	get1rec(df);
	dbtFromFile(df, 1, "e:\\wmtasql\\debug\\a2");
	put_fld(df, 0, "2");
	putrec(df);

	get1rec(df);
	dbtFromFile(df, 1, "e:\\wmtasql\\debug\\a3");
	put_fld(df, 0, "3");
	putrec(df);

	dseek(df, 0, dSEEK_SET);
	get1rec(df);
	dbtFromFile(df, 1, "e:\\wmtasql\\debug\\aa.c");
	put_fld(df, 0, "2");
	put1rec(df);
	dbtToFile(df, 1, "e:\\wmtasql\\debug\\bb.c");
	dclose(df);
	return  2;*/

	/*strcpy(field[0].field,"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	field[0].fieldtype = 'C';
	field[0].fieldlen = 10;
	field[0].fielddec = 0;
	field[1].field[0] = '\0';

	hBuildCodeLib( "d:\\code\\code.dbf" );
	//return 0;
*/
/*	ProcessAttach();
	df = dopen("e:\\wmtasql\\ls.dbf", DOPENPARA);
	dseek(df, 10, dSEEK_SET);
	RecDelete(df);
	dseek(df, 18, dSEEK_SET);
	RecDelete(df);
	dpack(df);
	dclose(df);
	ProcessDetach();
	return  0;

*/

	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = strlen("lz0.dbf");
	tscp1.lp = cmTS_OPEN_DBF;

	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), "lz0.dbf", 8);

	justRunASQL(buff, buff, 4096);

	ll = l = ((TS_COM_PROPS *)buff)->lp;

	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = 8;
	tscp1.lp = cmTS_REC_FETCH;
	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), &l, 4);
	l = 1;
	memcpy(buff+sizeof(TS_COM_PROPS)+4, &l, 4);

	justRunASQL(buff, buff, 4096);

	tscp1.packetType = 'Q';
	tscp1.msgType = 'Q';
	tscp1.len = 8;
	tscp1.lp = cmTS_CLOSE_DBF;
	memcpy(buff, &tscp1, sizeof(TS_COM_PROPS));
	memcpy(buff+sizeof(TS_COM_PROPS), &ll, 4);
	justRunASQL(buff, buff, 4096);

	return  0;
	buf[0] = "thread0 ";
	buf[1] = "thread1 ";
	buf[2] = "thread2 ";
	buf[3] = "thread3 ";
	buf[4] = "thread4 ";
	buf[5] = "thread5 ";
	buf[6] = "thread6 ";


	ProcessAttach();
	printf("init success\n");

	hsem = CreateSemaphore(NULL, 1, 1, NULL);
	j = wmtAskQS(1234, "Tgask", AsqlExprInFile|Asql_USEENV, &asqlEnv, NULL, NULL, buff, 255, hsem);
	if( j >= asqlTaskThreadNum )
		printf("no thread to service\n");
	else
		printf("thread %d is servicing\n", j);
	

    {
	MSG msg1;

	while( 1 ) {
	    GetMessage(&msg1, NULL, 0, 65535);
	    printf("message received: %d\n", msg1.message);
	    if( msg1.message == ASQL_ENDTASK_MSG ) {
		break;
	    }
	}
	ReleaseSemaphore(hsem, 1, NULL);
    }
    CloseHandle(hsem);

	printf("asql finish. return value %s", buff);
	
	ProcessDetach();
	return  0;

}

