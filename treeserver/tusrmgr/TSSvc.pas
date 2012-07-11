{*************************************************************************}
{    Module Name:                                                         }
{        TSSvc.pas                                                        }
{    Abstract:                                                            }
{        TreeServer service control interface.                            }
{    Author:                                                              }
{        Niu Jingyu                                                       }
{    Copyright (c) NiuJingyu 1998  China Railway Software Corporation.    }
{*************************************************************************}

unit TSSvc;

interface

uses
  Windows, WinSvc;

const
    SZAPPNAME = 'TreeSvr';
    SZSERVICENAME = 'TreeServer';
    SZSERVICEDISPLAYNAME = 'TreeServer Service';
    SZDEPENDENCIES = '';

function TSServiceQuery( var dwStatus: DWORD ): DWORD;
function TSServiceStart( argc: DWORD; var argv: PAnsiChar ): DWORD;
function TSServiceStop: DWORD;

implementation

{ function TSServiceQuery() Query the TreeServer Service's status. }
function TSServiceQuery( var dwStatus: DWORD ): DWORD;
var
    schService: SC_HANDLE;
    schSCManager: SC_HANDLE;
    //sshStatusHandle SERVICE_STATUS_HANDLE;
    ssStatus: TServiceStatus;
    bSuccess: Boolean;
                                        
begin
    dwStatus := 0;
    Result := 0;

    schSCManager := OpenSCManager( nil, nil, SC_MANAGER_ALL_ACCESS );
    if schSCManager <> 0 then begin
        schService := OpenService( schSCManager, SZSERVICENAME, SERVICE_ALL_ACCESS );
        if schService <> 0 then begin
            bSuccess := QueryServiceStatus( schService, ssStatus );
            if bSuccess then
                dwStatus := ssStatus.dwCurrentState
            else
                Result := GetLastError();

            CloseServiceHandle( schService );
        end
        else
            Result := GetLastError();

        CloseServiceHandle( schSCManager );
    end
    else
        Result := GetLastError();
end;

{ function TSServiceStart() Start the TreeServer Service. }
function TSServiceStart( argc: DWORD; var argv: PAnsiChar ): DWORD;
var
    schService: SC_HANDLE;
    schSCManager: SC_HANDLE;
    ssStatus: TServiceStatus;
    bSuccess: Boolean;

begin
    Result := 0;

    schSCManager := OpenSCManager( nil, nil, SC_MANAGER_ALL_ACCESS );
    if schSCManager <> 0 then begin
        schService := OpenService( schSCManager, SZSERVICENAME, SERVICE_ALL_ACCESS );
        if schService <> 0 then begin
            { Query current service status. }
            bSuccess := QueryServiceStatus( schService, ssStatus );
            if bSuccess then begin
                { If service already started, close all handles and return. }
                if ssStatus.dwCurrentState = SERVICE_RUNNING then begin
                    CloseServiceHandle( schService );
                    CloseServiceHandle( schSCManager );
                    exit;
                end;
            end;

	    { try to start the service. }
            bSuccess := StartService( schService, argc, argv );
            if bSuccess then begin
                Sleep( 100 );

                while QueryServiceStatus( schService, ssStatus ) do begin
                    if ssStatus.dwCurrentState = SERVICE_START_PENDING then
                        Sleep( 100 )
                    else
                        break;
                end;

                if ssStatus.dwCurrentState = SERVICE_STOPPED then
                    Result := GetLastError();
            end
            else
                Result := GetLastError();

            CloseServiceHandle( schService );
        end
        else
            Result := GetLastError();

        CloseServiceHandle( schSCManager );
    end
    else
        Result := GetLastError();
end;

{ function TSServiceStop() Stop the TreeServer Service. }
function TSServiceStop: DWORD;
var
    schService: SC_HANDLE;
    schSCManager: SC_HANDLE;
    ssStatus: TServiceStatus;
    bSuccess: Boolean;

begin
    Result := 0;

    schSCManager := OpenSCManager( nil, nil, SC_MANAGER_ALL_ACCESS );
    if schSCManager <> 0 then begin
        schService := OpenService( schSCManager, SZSERVICENAME, SERVICE_ALL_ACCESS );
        if schService <> 0 then begin
            { Query current service status. }
            bSuccess := QueryServiceStatus( schService, ssStatus );
            if bSuccess then begin
                { If service already stopped, close all handles and return. }
                if ssStatus.dwCurrentState = SERVICE_STOPPED then begin
                    CloseServiceHandle( schService );
                    CloseServiceHandle( schSCManager );
                    exit;
                end;
            end;

	    { try to stop the service. }
            bSuccess := ControlService( schService, SERVICE_CONTROL_STOP, ssStatus );
            if bSuccess then begin
                Sleep( 100 );

                while QueryServiceStatus( schService, ssStatus ) do begin
                    if ssStatus.dwCurrentState = SERVICE_STOP_PENDING then
                        Sleep( 100 )
                    else
                        break;
                end;

                if ssStatus.dwCurrentState <> SERVICE_STOPPED then
                    Result := GetLastError();
            end
            else
                Result := GetLastError();

            CloseServiceHandle( schService );
        end
        else
            Result := GetLastError();

        CloseServiceHandle( schSCManager );
    end
    else
        Result := GetLastError();
end;

end.
