unit ts_com;

interface
{$A-}

uses windows;

type TTS_COM_PROPS = record
    packetType : Char;		// '$'
    msgType : Char;
    len : Short;				// this packet length
    lp : Longint;
    leftPacket : Char;
    endPacket : Char;
end;

type pTTS_COM_PROPS = ^TTS_COM_PROPS;

//###################################################
const pkTS_SYNC_PIPE = '$';
const msgTS_CLIENT_LOGON = 'L';
const msgTS_CLIENT_LOGOFF = 'E';
const msgTS_LOGON_OK = 'O';
const msgTS_LOGON_ERROR = 'X';
const msgTS_PIPE_CLOSE = 'C';
const msgTS_NO_DATA = 'N';

//###################################################
const pkTS_ASQL = 'Q';
const msgFAST_ASQL = 'Q';
const cmTS_OPEN_DBF = $2000;
const cmTS_CLOSE_DBF = $2001;
const cmTS_REC_FETCH = $2002;
const cmTS_REC_PUT = $2003;
const cmTS_REC_ADD = $2004;
const cmTS_REC_APP = $2005;
const cmTS_REC_DEL = $2006;
const cmTS_BLOB_FETCH = $2007;
const cmTS_BLOB_PUT = $2008;
const cmTS_DBF_RECNUM = $2009;
const msgASQL_TYPE_A = 'A';
const msgASQL_TYPE_B = 'B';
const msgASQL_TYPE_C = 'C';

//###################################################
const pkTS_PIPE_FTP = 'F';
const msgTS_FTP_FILE = 'F';
const cmFTP_FILE_OK = 0;
const cmFTP_FILE_ERROR = $FFFFFFFF;
const cmFTP_FILE_PUT = $3001;
const cmFTP_FILE_GET = $3002;
const cmFTP_FILE_DATA = $3003;
const cmFTP_FILE_END = $3004;

//###################################################
const pkTS_TEST = 'T';

//###################################################
const pkTS_MONITOR = 'M';
//###################################################
const pkTS_INFO = 'I';

//###################################################

const pkTS_ERROR = 'E';
const msgTS_ERROR_MSG = 'E';

//###################################################

const pkTS_ACCOUNT_CTL = 'U';
const msgTS_ACCOUNT_CTL_M = 'M';

const cmTS_M_ACCOUNT_ADD = $4001;
const cmTS_M_ACCOUNT_DEL = $4002;
const cmTS_M_ACCOUNT_RECEIVE = $4003;
const cmTS_M_ACCOUNT_MODIFY = $4004;
const cmTS_M_ACCOUNT_ENUM = $4005;
const cmTS_M_ACCOUNT_LOCK = $4006;
const cmTS_M_ACCOUNT_UNLOCK = $4007;
const cmTS_M_ACCOUNT_CLRFAIL = $4008;
const cmTS_M_ACCOUNT_CHGNAME = $4009;
const cmTS_M_ACCOUNT_RCVRULE = $4010;
const cmTS_M_ACCOUNT_MDFYRULE = $4011;
const cmTS_M_ACCOUNT_GETHMEDIR = $4012;
const msgTS_ACCOUNT_CTL_U = 'U';
const cmTS_U_ACCOUNT_MDFYPSWD = $4000;

implementation
end.

