unit PipeStruct;

interface
{$A-}

uses windows, Grids;
const
     MAX_SERVICE_THREAD	= 10;

     MAX_TED_CAPABILITY = 3;
     MAX_TED_ALIAS = 16;
     MAX_TED_COMMAND = 16;
     MAX_TED_FUNCTION = 64;

     NODE_SIZE = 1024;
     MAX_USER_NAME = 16;
     MAX_USER = 16;
     MAX_PASSWD = 16;
     MAX_COMPUTER = 15;
     MAX_ACCESS_MASK = 16;
     MAX_LOGON_HOURS = 21;
     MAX_LOGON_COMPUTER	= (MAX_COMPUTER+1)*20;
     MAX_DESCRIPTION = 128;
     MAX_PATH = 260;

     MAX_COMMAND = MAX_TED_COMMAND;
     MAX_REQUEST_PARAM = 512;
     MAX_RESPONSE_STRING = 512;

     MIN_QUEUE = 10;
     MAX_QUEUE = 50;

     MAX_QUEUE_WAIT = 1000;

     EXCHNG_BUF_SIZE = 4096;
     PIPE_BUFFER_SIZE = EXCHNG_BUF_SIZE;
     PIPE_TIMEOUT = 5000;



type TTsTime = record
     year : Short;
     month : Byte;
     day : Byte;
     hour : Byte;
     min : Byte;
     sec : Byte;
     week : Byte;
end;

type TUserInfo = record
     szUser : array[0..MAX_USER_NAME - 1] of Char;
     dwUserId : {Longint}DWORD;
     szPasswd : array[0..MAX_PASSWD - 1] of Char;
     szHomeDir : array[0..MAX_PATH - 1] of Char;
     szLogonComputer : array[0..MAX_LOGON_COMPUTER - 1] of Char;
     bLogonHours : array[0..MAX_LOGON_HOURS - 1] of Char;
     szAccessMask : array[0..MAX_ACCESS_MASK - 1] of Char;
     szDescription : array[0..MAX_DESCRIPTION - 1] of Char;
     cLogonFail : Char;
     cLocked : Char;
     tLastLogon : TTsTime;
     tLastLogonFail : TTsTime;
     szReserved : array[0..157] of Char;
end;
type
    PTUserInfo = ^TUserInfo;
    TEnumProc = function( lpUserInfo : PTUserInfo ) : Boolean;
    PTEnumProc = ^TEnumProc;

type TRectInfo = record
     PRect : array[1..168] of TRect;
     CRect : array[1..168] of TGridRect;
     iNum : integer;
end;

var
   ClihPipe : DWORD;
   ComUserInfo : TUserInfo;


implementation

end.
