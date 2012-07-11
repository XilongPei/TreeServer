
TSADOps.dll: dlldata.obj TSADO_p.obj TSADO_i.obj
	link /dll /out:TSADOps.dll /def:TSADOps.def /entry:DllMain dlldata.obj TSADO_p.obj TSADO_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del TSADOps.dll
	@del TSADOps.lib
	@del TSADOps.exp
	@del dlldata.obj
	@del TSADO_p.obj
	@del TSADO_i.obj
