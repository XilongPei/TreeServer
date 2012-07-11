{*************o**************************************************************
 * FILENAME:                    dio.h
 * Copyright: (c)
 *     Shanghai Tiedao University 1990-1997
 *     East-Union Computer Service Co., Ltd. 1994
 *     China Railway Software Company. 1996-1997
 *
 * MainAuthor:
 *     Xilong Pei
 * Statements:
 *
 ***************************************************************************}

{*==========
 * dERROR =     1012: name is too long
 *              1003: file not found
 *              1001: ENOMEM
 *              1013: buffer is too little
 *              2000: DBF is readonly, and set the dFILE_OFLAG
 *              2001: DBT is readonly, and set the dFILE_OFLAG
 *              3001: Field_num is bigger than the df->field_num
 *              4001: DBT file can't be opened
 *              4002: DBT Record is too long( longer than 10 blocks)
 *==========
 *}

//for WindowsNT Informix use
unit dio;

interface

uses
  Windows, SysUtils;

const
  NAMELEN                = 256;
  FIELDNAMELEN           = 32;
  EXTLEN                 = 4;
  FIELDMAXLEN            = 254;
  dMAXDECLEN             = 35;
  dIOMaxUseRecord        = 32000;
  DBTBLOCKSIZE           = 512;
  DBTDELMEMORYSIZE       = DBTBLOCKSIZE div 4;
  DIO_MAX_SYNC_BTREE     = 10;
  DBT_DATASIZE           = (512-8);
  dBITMEMOMemoMark       = $004E;
  SYSSIZE                = 4096;
  dMINDBTSIZE            = 4096;
  _DIOFILENUM_           = 100;
  TABLENAMELEN           = 20;
  CURSORNAMELEN          = 10;
  IDLEN		         = 10;
  VIEWFLAG               = 1;
  TABLEFLAG              = 2;
  dKEY_UNIQUE	         = 1;
  dKEY_FREE	         = 2;
  MAX_LINE_LENGTH        = 256;
  VIEW_RECNO_FIELD       = 'R_E_C_N_O_';
  dDBTTMPFILEEXT	 = '._BT';
  dOK                    = 0;
  dNOT_OK                = 1;
  IniBufUseTimes         = 7;
  DBFVIEWFLAG            = '#*#ViewDBF#*#';
  DBFVIEWFLAGLENGTH      = 13;

  dSEEK_SET              = 0;
  dSEEK_CUR              = 1;
  dSEEK_END              = 2;

type
  dFIELD = record
    field: array[0..NAMELEN-1] of uchar;
    fieldtype: uchar;
    fieldlen: uchar;
    fielddec: uchar;
    fieldstart: PChar;
  end;

  dBITMEMO = record
    MemoMark: word;
    MemoLen: Longint;
    MemoTime: Longint;
    MemoContent: array[0..1] of char;
  end;

  DBTBLOCKINFO = record
    last: Longint;
    next: Longint;
  end;

  DBTBLOCK = record
    data: array[0..DBT_DATASIZE-1] of char;
    last: Longint;
    next: Longint;
  end;

  dFLAG = record
    buf_write: char;
    dbt_write: char;
    file_write: char;
    append: char;
    creat: char;
    SleepDBF: Integer;
  end;

  dBUFPOINTER = record
    buf_write: char;
    times: SmallInt;
    rec_num: Longint;
  end;

  sqlvar_struct = record
    sqltype: SmallInt;
    sqllen: SmallInt;
    sqldata: PChar;
    sqlind: ^SmallInt;
    sqlname: PChar;
    sqlformat: PChar;
    sqlitype: SmallInt;
    sqlilen: SmallInt;
    sqlidata: PChar;
  end;

  sqlda = record
    sqld: SmallInt;
    sqlvar: ^sqlvar_struct;
    desc_name: array[0..18] of char;
    desc_occ: SmallInt;
    {desc_next: ^sqlda;}
    desc_next: LPSTR;
  end;

  dFILE = record
    dbf_flag: uchar;
    op_flag: uchar;
    name: array[0..NAMELEN-1] of char;
    fp: SmallInt;
    dp: SmallInt;
    oflag: SmallInt;
    shflag: SmallInt;
    pmode: SmallInt;
    last_modi_time: array[0..2] of char;
    headlen: word;
    field_num: word;
    field: ^dFIELD;
    fld_id: ^SmallInt;
    rec_len: word;
    rec_buf: PChar;
    dbt_buf: PChar;
    dbt_head: ^Longint;
    dbt_len: char;
    dbt_p: char;
    rec_num: longint;
    rec_no: Longint;
    rec_p: Longint;
    rec_beg: longint;
    rec_end: Longint;
    buf_bsize: word;
    buf_rsize: word;
    buf_sink: PChar;
    write_flag: dFLAG;
    buf_usetimes: word;
    buf_freshtimes: word;
    max_recnum: Longint;
    min_recnum: Longint;
    buf_pointer: ^dBUFPOINTER;
    buf_methd: Char;
    error: SmallInt;
    sql_id: array[0..IDLEN] of char;
    sqlCursor: array[0..CURSORNAMELEN] of char;
    sqlDesc: ^sqlda;
    lpSqlDataBuf: PChar;
    dIOReadWriteFlag: Integer;
    firstDel: Longint;
    rec_tmp: PChar;
    syncBhNeed: array[0..DIO_MAX_SYNC_BTREE-1] of char;
    bhs: array[0..DIO_MAX_SYNC_BTREE-1] of pchar;
    syncBhNum: SmallInt;
    del_link_id: word;
    del_link_off: ^Longint;
    keep_physical_order: char;
    dpack_auto: char;
    dCriticalSection: TRTLCriticalSection;
    inCriticalSection: Char;
    {pdf: ^dFILE;}
    pdf: LPSTR;
    {pvdf: ^dFILE;}
    pvdf: LPSTR;
    timeStamp: LongInt;
  end;

  dVIEW = record
    source: dFILE;
    name: array[0..NAMELEN-1]of char;
    view: ^dFILE;
    rec_buf: PChar;
    RecnoFldid: word;
    field_num: SmallInt;
    szfield: ^PChar;
    ViewField: PChar;
  end;

implementation

end.

