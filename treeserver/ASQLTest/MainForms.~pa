unit MainForms;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ToolWin, ComCtrls, Menus, ImgList, ExtCtrls, IniFiles,Math,URLMon;

type
  pFhbVar=^FhbVar;
  FhbVar = record
    name:string;
    lxint:integer;
    len:integer;
    csz:string;
  end;

  TMainForm = class(TForm)
    File1: TMenuItem;
    Open1: TMenuItem;
    Save1: TMenuItem;
    N1: TMenuItem;
    ExitItem: TMenuItem;
    CoolBar: TCoolBar;
    ToolBar: TToolBar;
    OpenBtn: TToolButton;
    ExitBtn: TToolButton;
    ImageList: TImageList;
    SaveBtn: TToolButton;
    OpenFileDialog: TOpenDialog;
    N2: TMenuItem;
    NewBtn: TToolButton;
    SaveAs: TMenuItem;
    SaveFileDialog: TSaveDialog;
    MainMenu: TMainMenu;
    E1: TMenuItem;
    N3: TMenuItem;
    C1: TMenuItem;
    P1: TMenuItem;
    D1: TMenuItem;
    N4: TMenuItem;
    N5: TMenuItem;
    ToolButton1: TToolButton;
    RunBtn: TToolButton;
    SetBtn: TToolButton;
    N6: TMenuItem;
    R1: TMenuItem;
    S1: TMenuItem;
    ToolButton2: TToolButton;
    MsgBtn: TToolButton;
    DbfBtn: TToolButton;
    OpenDbfDialog: TOpenDialog;
    N7: TMenuItem;
    I1: TMenuItem;
    F1: TMenuItem;
    StatusBar: TStatusBar;
    FindDialog1: TFindDialog;
    N8: TMenuItem;
    Editor: TRichEdit;
    R2: TMenuItem;
    ReplaceDialog1: TReplaceDialog;
    H1: TMenuItem;
    N9: TMenuItem;
    O1: TMenuItem;
    N10: TMenuItem;
    N12: TMenuItem;
    FontDialog1: TFontDialog;
    Splitter1: TSplitter;
    N11: TMenuItem;
    N13: TMenuItem;
    Panel1: TPanel;
    MsgEditor: TRichEdit;
    ListView: TListView;
    N14: TMenuItem;
    N15: TMenuItem;
    procedure OpenBtnClick(Sender: TObject);
    procedure ExitItemClick(Sender: TObject);
    procedure SaveBtnClick(Sender: TObject);
    procedure FormKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure FormCreate(Sender: TObject);
    procedure NewBtnClick(Sender: TObject);
    procedure SaveAsClick(Sender: TObject);
    procedure N3Click(Sender: TObject);
    procedure C1Click(Sender: TObject);
    procedure P1Click(Sender: TObject);
    procedure D1Click(Sender: TObject);
    procedure N5Click(Sender: TObject);
    procedure RunBtnClick(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
    procedure SetBtnClick(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure MsgBtnClick(Sender: TObject);
    procedure DbfBtnClick(Sender: TObject);
    procedure ExitBtnClick(Sender: TObject);
    procedure N8Click(Sender: TObject);
    procedure FindDialog1Find(Sender: TObject);
    procedure R2Click(Sender: TObject);
    procedure ReplaceDialog1Replace(Sender: TObject);
    procedure N9Click(Sender: TObject);
    procedure N10Click(Sender: TObject);
    procedure FontDialog1Apply(Sender: TObject; Wnd: HWND);
    procedure N13Click(Sender: TObject);
    procedure N14Click(Sender: TObject);
    procedure N15Click(Sender: TObject);
    procedure EditorChange(Sender: TObject);
  private
    CannotOpenDBF:boolean;
    procedure FileNew;
    function FileOpen(const AFileName: string):BOOL;
    function FileModify:BOOL;
    procedure Read_Table(const FileName:string);
  public
    PathName,AppPath: string;
  end;

var
  MainForm: TMainForm;
  FhbList:TStringList;
  Fhb:pFhbVar;
  Fhbsize:integer;
  MaxRecReturn:integer;

const
  DefaultFileName = '未命名';

implementation

uses ASQLUnit, SetDlgs, AboutBoxs, VarDlgs, ChgPassDlgs, MiniParser,
  SrcParser;

procedure strToRichEdit(const S: String; aEdit: TRichEdit);
var
  aMem : TMemoryStream;
begin
  aMem:=TMemoryStream.Create;
  try
    aMem.Write(Pointer(S)^, Length(S));
    aMem.Position:=0;
    aEdit.Lines.LoadFromStream(aMem);
  finally
    aMem.Free;
  end;
end;

{$R *.DFM}

procedure TMainForm.ExitItemClick(Sender: TObject);
begin
  close;
end;

function TMainForm.FileOpen(const AFileName: string):BOOL;
begin
  FileOpen:=False;
  with Editor do
  begin
    StatusBar.Panels[0].Text := '正打开文件...';
    try
      //strToRichEdit(SourceToRtf(OpenFileDialog.FileName,False, True), Editor);
      Lines.LoadFromFile(AFileName);
      StatusBar.Panels[0].Text := '就绪';
    except
      StatusBar.Panels[0].Text := '停止';
      MessageBeep(MB_ICONEXCLAMATION);
      Application.MessageBox('文件打开错误，文件不正确或格式不正确', '提示框',
        MB_SYSTEMMODAL+mb_ok+MB_ICONEXCLAMATION);
      StatusBar.Panels[0].Text := '就绪';
      exit;
    end;
    PathName := AFileName;
    Caption:='ASQL 测试工具 - '+ExtractFileName(AFileName);
    FileOpen:=True;
    SelStart := 0;
    Modified := False;
    Fhb_Mode:=false;
  end;
end;

procedure TMainForm.FileNew;
begin
  PathName:=DefaultFileName;
  Caption:='ASQL 测试工具 - '+DefaultFileName;
  Editor.Lines.clear;
  Fhb_Mode:=false;
end;

function TMainForm.FileModify:BOOL;
const
  SWarningText = '%s 文件的已改变，是否要存盘 ？';
begin
  FileModify:=True;
  if Editor.Modified then
  begin
    case MessageDlg(Format(SWarningText, [ExtractFileName(PathName)]), mtConfirmation,
      [mbYes, mbNo, mbCancel], 0) of
      idYes:
        SaveBtnClick(Self);
      idCancel:
        FileModify := False;
    end;
  end;
end;

procedure TMainForm.OpenBtnClick(Sender: TObject);
begin
  OpenFileDialog.DefaultExt:='DBF';
  OpenFileDialog.Filter := 'DBF文件 (*.DBF)|*.DBF';
  if OpenFileDialog.Execute then
    if FileModify then
    begin
      editor.PlainText := False;
      FileOpen(OpenFileDialog.FileName);
      editor.PlainText := True;
    end;
end;

procedure TMainForm.SaveBtnClick(Sender: TObject);
begin
  if PathName = DefaultFileName then
    SaveAsClick(Sender)
  else
  begin
    try
      Editor.PlainText:=true;
      Editor.Lines.SaveToFile(PathName);
      Editor.Modified := False;
    finally
      Editor.PlainText:=false;
      Editor.Refresh;
    end;
  end;
end;

procedure TMainForm.FormKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
  if ((ssAlt in Shift)and(key=18))or(key=vk_f10) then
  begin
    key:=0;
  end;
end;

procedure TMainForm.FormCreate(Sender: TObject);
var
  IniFile:TiniFile;
  cc:array [0..256] of char;
  p,pp:pchar;
  i:integer;
  str:string;
begin
  StatusBar.Panels[0].Text := '就绪';
  StatusBar.Panels[2].Text :=  '单机运行方式';
  PathName:=DefaultFileName;
  Caption:='ASQL 测试工具 - '+DefaultFileName;
  ASQLUnit.Run_ASQL_Local_Mode:=true;
  ASQLUnit.Run_ASQL_TCPIP_Mode:=false;
  ASQLUnit.MainPath:=GetCurrentDir;
  FhbList:=TStringList.create;
  Fhb:=nil;
  Fhb_Mode:=false;
  CannotOpenDBF:=false;

  zeromemory(@cc,256);
  copymemory(@cc,pchar(application.exename),256);

  pp:=cc;
  P := StrRScan(pp,'\');
  if p=nil then
    Beep
  else
    p^:=#0;
    
  AppPath:=string(cc);
  if trim(AppPath)<>'' then
    AppPath:=AppPath+'\';

  IniFile:=TiniFile.create(AppPath+'asqltest.ini');
  MainServerName:=IniFile.ReadString('Configuration','ServerName','');
  Mainuser:=IniFile.ReadString('Configuration','User','');
  Mainpassword:=IniFile.ReadString('Configuration','Password','');
  MainTCPIP:=IniFile.ReadString('Configuration','TCPIP','');

  for i:=1 to length(Mainpassword) do
    Mainpassword[i]:=chr(ord(Mainpassword[i])+i);
  str:=IniFile.ReadString('Configuration','Font name','');
  if str<>'' then
  begin
    Editor.font.name:=str;
    Editor.font.size:=IniFile.ReadInteger('Configuration','Font size',10);
    Editor.font.color:=TColor(IniFile.ReadInteger('Configuration','Font color',clWindowText));
  end;

  MaxRecReturn:=IniFile.ReadInteger('Configuration','Max Record Return',1000);
end;

procedure TMainForm.NewBtnClick(Sender: TObject);
begin
  FileNew;
end;

procedure TMainForm.SaveAsClick(Sender: TObject);
begin
  SaveFileDialog.FileName := PathName;
  if SaveFileDialog.Execute then
  begin
    PathName := SaveFileDialog.FileName;
    Caption:='ASQL 测试工具 - '+ExtractFileName(PathName);
    SaveBtnClick(Sender);
  end;
end;

procedure TMainForm.N3Click(Sender: TObject);
begin
  Editor.CutToClipboard;
end;

procedure TMainForm.C1Click(Sender: TObject);
begin
  Editor.CopyToClipboard;
end;

procedure TMainForm.P1Click(Sender: TObject);
begin
  Editor.PasteFromClipboard;
end;

procedure TMainForm.D1Click(Sender: TObject);
begin
  Editor.ClearSelection;
end;

procedure TMainForm.N5Click(Sender: TObject);
begin
  Editor.SelectAll;
end;

procedure TMainForm.FormCloseQuery(Sender: TObject; var CanClose: Boolean);
begin
  if not FileModify then
    CanClose:=False;
end;

procedure TMainForm.RunBtnClick(Sender: TObject);
var
  j:integer;
  cci:Smallint;
  Size,ccli:longint;
  ccr:double;
  mXexpVar:pXexpVar;
  p,p1,pp:pointer;
  ppp:pchar;
  cc:string;
  wpp:dword;
begin
  mXexpVar:=nil;
  if Editor.Lines.Text='' then exit;

  Size := Editor.GetTextLen;
  Inc(Size);
  ASQL_Mem:=nil;
  if ASQL_Mem<>nil then
    FreeMem(ASQL_Mem, Size);

  GetMem(ASQL_Mem, Size);
  try
    try
      Editor.PlainText:=true;
      Editor.GetTextBuf(ASQL_Mem,Size);
    finally
      Editor.PlainText:=false;
      Editor.Refresh;
    end;
    StatusBar.Panels[0].Text := '正运行...';
    StatusBar.Panels[1].Text := '';

    if not Fhb_Mode then
      Run_Asql
    else
    begin
      try
        mXexpVar:=allocmem(sizeof(XexpVar)*fhbsize);
        p1:=mXexpVar;
        if Fhb<>nil then
        begin
          p:=Fhb;
          for j:=0 to Fhbsize-1 do
          begin
            if j<>0 then
            begin
              inc(Fhb);
              inc(mXexpVar);
            end;
            mXexpVar.nType:=strtoint(LxTable[fhb.lxint,1]);
            strpcopy(mXexpVar.VarOFunName,fhb.name);
            mXexpVar.Length:=31;
            ZeroMemory(@mXexpVar.values[0],32);
            if trim(fhb.csz)<>'' then
            begin
              try
                case mXexpVar.nType of
                  1054:
                    begin
                      cci:=strtoint(fhb.csz);
                      copymemory(@mXexpVar.values[0],@cci,2);
                    end;
                  1055:
                    begin
                      ccli:=strtoint(fhb.csz);
                      copymemory(@mXexpVar.values[0],@ccli,4);
                    end;
                  1056:
                    begin
                      ccr:=strtofloat(fhb.csz);
                      copymemory(@mXexpVar.values[0],@ccr,8);
                    end;
                  else
                    copymemory(@mXexpVar.values[0],pchar(fhb.csz),round(minvalue([32,length(fhb.csz)])));
                end;
              except
                beep;
                exit;
              end;
            end;

            if fhb.len>=32 then
            begin
              ZeroMemory(@mXexpVar.values[0],32);
              mXexpVar.Length:=fhb.len;
              GetMem(pp,mXexpVar.Length);
              ZeroMemory(pp,mXexpVar.Length);
              copymemory(pp,pchar(fhb.csz),trunc(minvalue([mXexpVar.Length,length(fhb.csz)])));
              wpp:=dword(pp);
              copymemory(@mXexpVar.Values[0],@wpp,4);
            end;
          end;
          Fhb:=p;
          mXexpVar:=p1;
        end;
        if not Run_Asql_Var(mXexpVar,fhbsize) then exit;
        MsgEditor.Lines.Add('');
        MsgEditor.Lines.Add('');
        MsgEditor.font.Color:=clOlive;
        for j:=0 to Fhbsize-1 do
        begin
          if j<>0 then
            inc(mXexpVar);
          cc:='';
          case mXexpVar.nType of
            1054:
              begin
                copymemory(@cci,@mXexpVar.Values,2);
                cc:=inttostr(cci);
              end;
            1055:
              begin
                copymemory(@ccli,@mXexpVar.Values,4);
                cc:=inttostr(ccli);
              end;
            1056:
              begin
                copymemory(@ccr,@mXexpVar.Values,8);
                cc:=floattostr(ccr);
              end;
            1051:
              begin
                if mXexpVar.Length>=32 then
                begin
                  copymemory(@wpp,@mXexpVar.Values[0],4);
                  pp:=pointer(wpp);
                  GetMem(ppp,mXexpVar.Length);
                  copymemory(ppp,pp,mXexpVar.Length);

                  MsgEditor.Lines.Add(mXexpVar.VarOFunName+':'+string(ppp));
                  FreeMem(pp,mXexpVar.Length);
                  FreeMem(ppp,mXexpVar.Length);
                end
                else
                  cc:=mXexpVar.Values;
              end;
            else
              cc:=mXexpVar.Values;
          end;
          if mXexpVar.Length<32 then
            MsgEditor.Lines.Add(mXexpVar.VarOFunName+':'+cc);
        end;
      except
        StatusBar.Panels[0].Text := '错误';
        exit;
      end;
    end;
  finally
    FreeMem(ASQL_Mem, Size);
  end;
  try
    if mXexpVar<>nil then
      freemem(mXexpVar);
  except
  end;
  StatusBar.Panels[0].Text := '就绪';
  MsgEditor.Visible:=true;
  ListView.Visible:=false;
  ListView.Items.clear;
  ListView.Columns.clear;
end;

procedure TMainForm.SetBtnClick(Sender: TObject);
begin
  Application.CreateForm(TSetDlg, SetDlg);
  SetDlg.ShowModal;
  SetDlg.Free;
  if  Run_ASQL_Local_Mode then
    StatusBar.Panels[2].Text :=  '单机运行方式'
  else
    StatusBar.Panels[2].Text :=  '服务器运行方式';
end;

procedure TMainForm.FormDestroy(Sender: TObject);
var
  IniFile:TiniFile;
  MFont:TFont;
  i:integer;
begin
  FhbList.free;
  freemem(fhb);
  if not Run_ASQL_Local_Mode then
    Logoff_Asql;

  IniFile:=TiniFile.create(AppPath+'asqltest.ini');
  IniFile.WriteString('Configuration','ServerName',MainServerName);
  IniFile.WriteString('Configuration','User',Mainuser);
  IniFile.WriteString('Configuration','TCPIP',MainTCPIP);
  for i:=1 to length(Mainpassword) do
    Mainpassword[i]:=chr(ord(Mainpassword[i])-i);
  IniFile.WriteString('Configuration','Password',Mainpassword);

  MFont:=Editor.font;
  IniFile.WriteString('Configuration','Font name',MFont.name);
  IniFile.WriteInteger('Configuration','Font size',MFont.size);
  IniFile.WriteInteger('Configuration','Font color',integer(MFont.color));
  IniFile.WriteInteger('Configuration','Max Record Return',MaxRecReturn);
end;

procedure TMainForm.MsgBtnClick(Sender: TObject);
begin
  MsgEditor.Visible:=true;
  ListView.Visible:=false;
end;

procedure TMainForm.DbfBtnClick(Sender: TObject);
begin
  if MsgEditor.Visible and (ListView.Columns.Count>1) then
  begin
    ListView.Visible:=true;
    Splitter1.Visible:=true;
    MsgEditor.Visible:=False;
    exit;
  end;
  if CannotOpenDBF then
  begin
    Beep;
    exit;
  end;
  OpenDbfDialog.InitialDir:=MainPath;
  if OpenDbfDialog.Execute then
  begin
    StatusBar.Panels[0].Text := '正打开文件...';
    StatusBar.Panels[1].Text := '';
    try
      DbfBtn.Enabled:=false;
      CannotOpenDBF:=true;
      Read_Table(OpenDbfDialog.FileName);
      CannotOpenDBF:=false;
      DbfBtn.Enabled:=true;
      StatusBar.Panels[0].Text := '就绪';
    except
      CannotOpenDBF:=false;
      DbfBtn.Enabled:=true;
      StatusBar.Panels[0].Text := '停止';
      MessageBeep(MB_ICONEXCLAMATION);
      StatusBar.Panels[0].Text := '就绪';
      exit;
    end;
    ListView.Visible:=true;
    MsgEditor.Visible:=false;
  end;
end;

procedure TMainForm.ExitBtnClick(Sender: TObject);
begin
  close;
end;

procedure TMainForm.Read_Table(const FileName:string);
type
  RecVar = record
   field:array [0..31] of char;
   fieldtype:char;
   fieldlen:byte;
   fielddec:byte;
end;
var
  buffer:array [0..35] of char;
  field:^RecVar;
  i,j,k,l,nbytes,retrec,bytesread,field_num:integer;
  rec_num:Longint;
  headlen,rec_len,w:word;
  p:pointer;
  onerec:pchar;
  FileStream,savef:TFileStream;
  str:array [0..200] of char;
  rec:string;
  ListItem: TListItem;
  NewColumn: TListColumn;
begin
  ListView.Items.clear;
  ListView.Columns.clear;
  nbytes:=32;
  p:=nil;
  try
    FileStream:=TFileStream.Create(FileName,fmOpenRead);
    savef:=FileStream;
  except
    exit;
  end;

  try
    bytesread:=FileStream.Read(Buffer,nbytes);
    if bytesread <= 0  then exit;
    copymemory(@rec_num,@Buffer[4],4);
    copymemory(@headlen,@Buffer[8],2);
    copymemory(@rec_len,@Buffer[10],2);
    copymemory(@w,@Buffer[8],2);
    field_num:=round(w/32-1);
    StatusBar.Panels[1].Text := format('记录数:%d, 记录长度:%d',[rec_num,rec_len]);
    try
      GetMem(field,field_num*sizeof(RecVar));
      p:=field;
      for i:=0 to field_num-1 do
      begin
        zeromemory(@buffer,32);
        FileStream.Read(buffer,32);
        if buffer[0] <= chr($0D) then
        begin
          field_num:=i;
          reallocmem( field, field_num * sizeof ( RecVar ) );
          break;
        end;
        copymemory(@field.field, @buffer, 11);
        field.field[11] := #0;
        buffer[32] := #0;
        field.fieldtype := buffer[11];
        copymemory(@field.fieldlen,@buffer[16],1);
        copymemory(@field.fielddec,@buffer[17],1);

        NewColumn:=ListView.Columns.Add;
        NewColumn.caption:=field.field;
        if field.fieldlen>length(string(field.field))+3 then
          l:=field.fieldlen
        else
          l:=length(string(field.field))+3;
        NewColumn.Width:=l*ListView.font.size;
        inc(field);
      end;
    except
      exit;
    end;

    FileStream.Seek(headlen+1,soFromBeginning);
    onerec:=StrAlloc(rec_len+1);
    try
      retrec:=rec_num;
      if retrec>MaxRecReturn then
      begin
        retrec:=MaxRecReturn;
        StatusBar.Panels[1].Text := format('记录数:%d, 记录长度:%d  (只显示前%d条记录)',[rec_num,rec_len,retrec]);
      end;
      for i:=0 to retrec-1 do
      begin
        l:=0;
        ZeroMemory(onerec,rec_len+1);
        while l<rec_len do
        begin
          if rec_len-l>length(str) then
            k:=length(str)
          else
            k:=rec_len-l;
          ZeroMemory(@str,length(str));
          bytesread:=FileStream.Read(str,k);
          if l=0 then
            rec:=string(str)
          else
            rec:=rec+string(str);

          copymemory(pointer(integer(pointer(onerec))+l),@str,bytesread);
          l:=l+bytesread;
          if bytesread<=0 then break;
        end;
        Application.ProcessMessages;

        ListItem := ListView.Items.Add;
        field:=p;
        l:=0;
        for j:=0 to field_num-1 do
        begin
          ZeroMemory(@str,length(str));
          if field.fieldlen>length(str) then
            k:=length(str)
          else
            k:=field.fieldlen;

          if (l+k)>rec_len then
            k:=rec_len-l;
          if k<0 then break;

          copymemory(@str,pointer(integer(pointer(onerec))+l),k);
          if l=0 then
            ListItem.caption:=str
          else
            ListItem.SubItems.Add(str);

          l:=l+field.fieldlen;
          inc(field);
        end;

      end;
    finally
      StrDispose(onerec);
    end;
  finally
    try
      field:=p;
      if field<>nil then
        FreeMem(field);
    except
    end;
    try
      FileStream:=savef;
      if FileStream<>nil then
        FileStream.Destroy;
    except
    end;
  end;
end;

procedure TMainForm.N8Click(Sender: TObject);
begin
  FindDialog1.Execute;
end;

procedure TMainForm.FindDialog1Find(Sender: TObject);
var
  FoundAt: LongInt;
  StartPos, ToEnd: integer;
begin
  with Editor do
  begin
    if SelLength <> 0 then
      StartPos := SelStart + SelLength
    else
      StartPos := 0;

    ToEnd := Length(Text) - StartPos;

    FoundAt := FindText(FindDialog1.FindText, StartPos, ToEnd, [stMatchCase]);
    if FoundAt <> -1 then
    begin
      SetFocus;
      SelStart := FoundAt;
      SelLength := Length(FindDialog1.FindText);
    end;
  end;
end;

procedure TMainForm.R2Click(Sender: TObject);
begin
  ReplaceDialog1.Execute;
end;

procedure TMainForm.ReplaceDialog1Replace(Sender: TObject);
var
  SelPos: Integer;
  StartPos, ToEnd: integer;
begin
  with TReplaceDialog(Sender) do
  begin
    if frReplace in Options then
    begin
      SelPos := Pos(FindText, Editor.Lines.Text);
      if SelPos > 0 then
      begin
        Editor.SelStart := SelPos - 1;
        Editor.SelLength := Length(FindText);
        Editor.SelText := ReplaceText;
      end
      else MessageDlg(Concat('找不到 "', FindText, '"'), mtError, [mbOk], 0);
    end
    else
      if frReplaceAll in Options then
      begin
        SelPos := Pos(FindText, Editor.Lines.Text);
        while SelPos > 0 do
        begin
          Editor.SelStart := SelPos - 1;
          Editor.SelLength := Length(FindText);
          Editor.SelText := ReplaceText;
          with Editor do
          begin
            StartPos:=SelStart+SelLength;
            ToEnd := Length(Text);
            SelPos:=FindText(ReplaceDialog1.FindText, StartPos, ToEnd, [stMatchCase]);
            if SelPos=-1 then break;
            SelPos:=SelPos+1;
          end;
        end;
      end
      else MessageDlg(Concat('找不到 "', FindText, '"'), mtError, [mbOk], 0);
  end;
end;

procedure TMainForm.N9Click(Sender: TObject);
begin
  Application.CreateForm(TAboutDlg, AboutDlg);
  AboutDlg.ShowModal;
  AboutDlg.Free;
end;

procedure TMainForm.N10Click(Sender: TObject);
begin
  FontDialog1.Font:=Editor.font;
  if FontDialog1.Execute then
    FontDialog1Apply(FontDialog1,MainForm.handle);
end;

procedure TMainForm.FontDialog1Apply(Sender: TObject; Wnd: HWND);
begin
  if ActiveControl is TRichEdit then
    with ActiveControl as TRichEdit do
      Font:=TFontDialog(Sender).Font
  else
    Beep;
end;

procedure TMainForm.N13Click(Sender: TObject);
var
  InputString:string;
  i:integer;
begin
  InputString:= InputBox('结果集最大记录数', 'ASQL测试', inttostr(MaxRecReturn) );
  try
    i:=strtoint(InputString);
    if i<=0 then
    begin
      beep;
      exit;
    end;
    MaxRecReturn:=i;
  except
    exit;
  end;
end;

procedure TMainForm.N14Click(Sender: TObject);
begin
  if Run_ASQL_Local_Mode then
  begin
    Beep;
    Application.MessageBox('改用户口令不能在单机运行方式运行', '提示框',
      MB_SYSTEMMODAL+mb_ok+MB_ICONEXCLAMATION);
    exit;
  end
  else
  begin
    Application.CreateForm(TChgPassDlg, ChgPassDlg);
    ChgPassDlg.ShowModal;
    ChgPassDlg.Free;
  end;
end;

procedure TMainForm.N15Click(Sender: TObject);
var
  Str:WideString;
begin
  Str:=mainform.apppath+'help\default.htm';
  if not FileExists(Str) then
  begin
    Beep;
    Application.MessageBox(pchar('帮助文件 "'+string(Str)+'" 不存在'), '提示框',
      MB_SYSTEMMODAL+mb_ok+MB_ICONEXCLAMATION);
  end
  else
    HLinkNavigateString(nil,pwidechar(Str));
end;

procedure TMainForm.EditorChange(Sender: TObject);
begin
end;
{
var
   pos,len:integer;
   oldstart:integer;
   IsKey:integer;
begin
   pos:=editor.selstart;
   IsKey:=searchkey(pos,len,editor.text,Length(editor.text),Note);
   case IsKey of
    0:
      begin
//         if (editor.SelAttributes.Color =Clred) or (editor.SelAttributes.Color =Clblue) then
         if (editor.SelAttributes.Color <> Clblack) then
         begin
            OldStart:=editor.SelStart;
            editor.SelStart :=pos;
            editor.SelLength := len;
            editor.SelAttributes.Color :=ClBlack;
            editor.SelStart := oldstart;
         end;
      end;
    1:
      begin
         oldstart:=editor.SelStart;
         editor.SelStart :=pos;
         editor.SelLength := len;
         editor.SelAttributes.Color :=Clred;
         editor.SelStart := oldstart;
      end;
    2:
      begin
         oldstart:=editor.SelStart;
         editor.SelStart :=pos;
         editor.SelLength := 1;
         editor.SelAttributes.Color :=clLime;
         editor.SelStart := oldstart;
      end;
    3:
      if (editor.SelAttributes.Color <> ClGray) then
      begin
         oldstart:=editor.SelStart;
         editor.SelStart :=pos;
         editor.SelLength := len;
         editor.SelAttributes.Color :=ClGray;
         editor.SelStart := oldstart;
      end;
   end;
end;
}
end.
