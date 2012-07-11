SOURCE FILES:
	\src\conv.c
	\src\des.c
	\src\exchbuff.c
	\src\notify.c
	\src\password.c
	\src\s_proc.c
	\src\schedule.c
	\src\secapi.c
	\src\shell.c
	\src\tengine.c
	\src\terror.c
	\src\tqueue.c
	\src\tree_tcp.c
	\src\treesvr.c
	\src\truninfo.c
	\src\tsftp.c
	\src\tuser.c
	\include\conv.h
	\include\dbtree.h
	\include\des.h
	\include\exchbuff.h
	\include\histdata.h
	\include\notify.h
	\include\ntfyres.h
	\include\password.h
	\include\resource.h
	\include\s_proc.h
	\include\schedule.h
	\include\secapi.h
	\include\shell.h
	\include\taccount.h
	\include\tengine.h
	\include\terror.h
	\include\tlimits.h
	\include\tqueue.h
	\include\tree_tcp.h
	\include\treesvr.h
	\include\truninfo.h
	\include\ts_com.h
	\include\ts_const.h
	\include\ts_dict.h
	\include\tscommon.h
	\include\tsdbfutl.h
	\include\tsftp.h
	\include\tuser.h
	\include\wmtasql.h
	\res\treesvr.rc
	\lib\tscommon.lib
	\lib\dbtree.lib
	\lib\taccount.lib
	\lib\asqlwmt.lib

COMMON:
	+ preprocessor
		WIN32
		_WINDOWS
		_MT
		_NTSDK
		SECURITY_WIN32
	+ Library
		kernel32.lib
		user32.lib
		gdi32.lib
		winspool.lib
		comdlg32.lib
		advapi32.lib
		shell32.lib
		ole32.lib
		oleaut32.lib
		uuid.lib
		odbc32.lib
		odbccp32.lib

WINDOWS95:
	+ preprocessor
		TREESVR_STANDALONE

	+ Link options
		/subsystem:windows

Trial version
	+ preprocessor
		TRIAL_VERSION