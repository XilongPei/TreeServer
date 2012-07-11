unit SetDlgs;

interface

uses Windows, SysUtils, Classes, Graphics, Forms, Controls, StdCtrls,
  Buttons, ExtCtrls, Messages, ComCtrls;

type
  TSetDlg = class(TForm)
    YesButton: TButton;
    NoButton: TButton;
    PageControl1: TPageControl;
    TabSheet1: TTabSheet;
    Label4: TLabel;
    GroupBox1: TGroupBox;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    ServerCheckBox: TCheckBox;
    ServerEdit: TEdit;
    UserEdit: TEdit;
    PasswordEdit: TEdit;
    ConnectButton: TButton;
    LocalCheckBox: TCheckBox;
    PathEdit: TEdit;
    TabSheet2: TTabSheet;
    FhbCheckBox: TCheckBox;
    AddButton: TButton;
    ModButton: TButton;
    DelButton: TButton;
    ListView: TListView;
    GroupBox2: TGroupBox;
    TcpIpCheckBox: TCheckBox;
    Label5: TLabel;
    TcpIpEdit: TEdit;
    procedure FormActivate(Sender: TObject);
    procedure LocalCheckBoxClick(Sender: TObject);
    procedure ServerCheckBoxClick(Sender: TObject);
    procedure ConnectButtonClick(Sender: TObject);
    procedure YesButtonClick(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
    procedure ServerEditKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure LocalCheckBoxKeyPress(Sender: TObject; var Key: Char);
    procedure ServerCheckBoxKeyPress(Sender: TObject; var Key: Char);
    procedure AddButtonClick(Sender: TObject);
    procedure ModButtonClick(Sender: TObject);
    procedure DelButtonClick(Sender: TObject);
    procedure TcpIpCheckBoxClick(Sender: TObject);
  private
    LogOn:BOOL;
    procedure SetEdit;
  public
  end;

var
  SetDlg: TSetDlg;

implementation

uses MainForms, ASQLUnit, VarDlgs;

{$R *.DFM}

procedure TSetDlg.SetEdit;
begin
  ServerEdit.Enabled:=ServerCheckBox.Checked;
  UserEdit.Enabled:=ServerCheckBox.Checked;
  PasswordEdit.Enabled:=ServerCheckBox.Checked;
  ConnectButton.Enabled:=ServerCheckBox.Checked;
  TcpIpCheckBox.Enabled:=ServerCheckBox.Checked;
  TcpIpEdit.Enabled:=ServerCheckBox.Checked;

  if not ServerCheckBox.Checked then
  begin
    ServerEdit.Color:=clBtnFace;
    UserEdit.Color:=clBtnFace;
    PasswordEdit.Color:=clBtnFace;
    TcpIpEdit.Color:=clBtnFace;
  end
  else
  begin
    ServerEdit.Color:=clWindow;
    UserEdit.Color:=clWindow;
    PasswordEdit.Color:=clWindow;
    if TcpIpCheckBox.Checked then
      TcpIpEdit.Color:=clWindow
    else
      TcpIpEdit.Color:=clBtnFace;
    TcpIpEdit.Enabled:=TcpIpCheckBox.Checked;
  end;
end;

procedure TSetDlg.FormActivate(Sender: TObject);
var
  i,j:integer;
  p:pointer;
begin
  FhbCheckBox.checked:=Fhb_Mode;
  
  LogOn:=not Run_ASQL_Local_Mode;
  LocalCheckBox.Checked:=Run_ASQL_Local_Mode;
  ServerCheckBox.Checked:=not Run_ASQL_Local_Mode;
  ServerEdit.text:=ASQLUnit.MainServerName;
  UserEdit.text:=ASQLUnit.MainUser;
  PasswordEdit.text:=ASQLUnit.MainPassword;
  PathEdit.Text:=ASQLUnit.MainPath;
  TCPIPEdit.Text:=ASQLUnit.MainTCPIP;
  SetEdit;

  if Fhb<>nil then
  begin
    p:=Fhb;
    for j:=0 to Fhbsize-1 do
    begin
      if j<>0 then
        inc(Fhb);
      ListView.items.add;
      i:=ListView.items.count-1;
      ListView.items.item[i].caption:=Fhb.name;
      ListView.items.item[i].SubItems.Add(LxTable[Fhb.lxint,0]);
      ListView.items.item[i].SubItems.Add(inttostr(Fhb.len));
      ListView.items.item[i].SubItems.Add(Fhb.csz);
      ListView.items.item[i].SubItems.Add(inttostr(Fhb.lxint));
    end;
    Fhb:=p;
  end;
end;

procedure TSetDlg.LocalCheckBoxClick(Sender: TObject);
begin
  ServerCheckBox.Checked:=not LocalCheckBox.Checked;
  PathEdit.Text:=GetCurrentDir();
  SetEdit;
end;

procedure TSetDlg.ServerCheckBoxClick(Sender: TObject);
begin
  LocalCheckBox.checked:=not ServerCheckBox.checked;
  PathEdit.Text:=ASQLUnit.MainPath;
  SetEdit;
end;

procedure TSetDlg.ConnectButtonClick(Sender: TObject);
var
  Path:array[0..100] of char;
  cc:string;
begin
  ServerEdit.text:=trim(ServerEdit.text);
  UserEdit.text:=trim(UserEdit.text);
  PassWordEdit.text:=trim(PasswordEdit.text);
  if ServerEdit.text='' then exit;
  if userEdit.text='' then exit;
  if passwordEdit.text='' then exit;
  ASQLUnit.MainServerName:=ServerEdit.text;
  ASQLUnit.MainUser:=UserEdit.text;
  ASQLUnit.MainPassword:=PasswordEdit.text;
  ASQLUnit.MainTCPIP:=TCPIPEdit.Text;
  if LogOn then
    Logoff_Asql;
  MainForm.StatusBar.Panels[0].Text := '正在登录...';
  
  LogOn:=false;
  cc:='';
  if TcpIpCheckBox.Checked then
  begin
    TcpIpEdit.Text:=trim(TcpIpEdit.Text);
    cc:=TcpIpEdit.Text;
    if cc='' then
    begin
      MainForm.StatusBar.Panels[0].Text := '就绪';
      Application.MessageBox('用TCP/IP运行方式，套接口或服务名不能是空的。', '提示框',
        MB_SYSTEMMODAL+mb_ok+MB_ICONEXCLAMATION);
      if TcpIpEdit.Enabled then
        TcpIpEdit.SetFocus;
      exit;
    end;
  end;

  if Logon_Asql(UserEdit.text,PasswordEdit.text,cc) then
  begin
    LogOn:=True;
    ZeroMemory(@Path[0], 100 );
    tsGetUserNetPath(ASQLUnit.TreeSVR_Handler,@Path[0],100);
    PathEdit.Text:=StrPas(Path);
    YesButton.setFocus;
  end;
  MainForm.StatusBar.Panels[0].Text := '就绪';
end;

procedure TSetDlg.YesButtonClick(Sender: TObject);
var
  p:pointer;
  i:integer;
begin
  if ServerCheckBox.Checked and LogOn then
  begin
    Run_ASQL_Local_Mode:=False;
    Run_ASQL_TCPIP_Mode:=TCPIPCheckBox.Checked;
  end
  else
  begin
    Run_ASQL_Local_Mode:=true;
    Run_ASQL_TCPIP_Mode:=false;
  end;
  MainPath:=PathEdit.Text;

  try
    screen.cursor:=crHourGlass;
    SetCurrentDir(MainPath);
  finally
    screen.cursor:=crdefault;
  end;

  freemem(fhb);
  try
    fhb:=allocmem(sizeof(FhbVar)*ListView.items.count);
  except
    exit;
  end;

  if fhb=nil then exit;

  Fhbsize:=ListView.items.count;
  p:=fhb;
  for i:=0 to ListView.items.count-1 do
  begin
    if i<>0 then
        inc(fhb);
    fhb.name:=ListView.items.item[i].caption;
    fhb.len:=strtoint(ListView.items.item[i].SubItems[1]);
    fhb.csz:=ListView.items.item[i].SubItems[2];
    fhb.lxint:=strtoint(ListView.items.item[i].SubItems[3]);
  end;
  fhb:=p;
  Fhb_Mode:=FhbCheckBox.checked;
  close;
end;

procedure TSetDlg.FormCloseQuery(Sender: TObject; var CanClose: Boolean);
begin
  if Run_ASQL_Local_Mode then
    if Logon then
      Logoff_Asql;
end;

procedure TSetDlg.ServerEditKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
  if key=VK_return then
    PostMessage((Sender as TEdit).handle,wm_keydown,wparam(vk_tab),0);
end;

procedure TSetDlg.LocalCheckBoxKeyPress(Sender: TObject; var Key: Char);
begin
  if key=#13 then
  begin
    LocalCheckBox.Checked:=true;
    ServerCheckBox.Checked:=not LocalCheckBox.Checked;
    PathEdit.Text:=GetCurrentDir();
    SetEdit;
  end;
end;

procedure TSetDlg.ServerCheckBoxKeyPress(Sender: TObject; var Key: Char);
begin
  if key=#13 then
  begin
    ServerCheckBox.checked:=true;
    LocalCheckBox.checked:=not ServerCheckBox.checked;
    PathEdit.Text:=ASQLUnit.MainPath;
    SetEdit;
  end;
end;

procedure TSetDlg.AddButtonClick(Sender: TObject);
var
  i:integer;
begin
  Application.CreateForm(TVarDlg, VarDlg);
  if VarDlg.ShowModal=mrOK then
  begin
    with VarDlg do
    begin
      ListView.items.add;
      i:=ListView.items.count-1;
      ListView.items.item[i].caption:=varname;
      ListView.items.item[i].SubItems.Add(lx);
      ListView.items.item[i].SubItems.Add(lenedit.text);
      ListView.items.item[i].SubItems.Add(varcsz);
      ListView.items.item[i].SubItems.Add(inttostr(LxComboBox.ItemIndex));
    end;
  end;
  VarDlg.Free;
end;

procedure TSetDlg.ModButtonClick(Sender: TObject);
begin
  if ListView.Selected=nil then exit;
  Application.CreateForm(TVarDlg, VarDlg);
  with VarDlg do
  begin
    Nameedit.text:=ListView.Selected.caption;
    LxComboBox.ItemIndex:=strtoint(ListView.Selected.SubItems[3]);
    Lenedit.text:=ListView.Selected.SubItems[1];
    cszedit.text:=ListView.Selected.SubItems[2];
  end;
  if VarDlg.ShowModal=mrok then
  begin
    with VarDlg do
    begin
      ListView.Selected.caption:=varname;
      ListView.Selected.SubItems[0]:=lx;
      ListView.Selected.SubItems[1]:=lenedit.text;
      ListView.Selected.SubItems[2]:=varcsz; //cszedit.text;
      ListView.Selected.SubItems[3]:=inttostr(LxComboBox.ItemIndex);
    end;
  end;
  VarDlg.Free;
end;

procedure TSetDlg.DelButtonClick(Sender: TObject);
begin
  if ListView.Selected=nil then exit;
  ListView.Selected.Delete;
end;

procedure TSetDlg.TcpIpCheckBoxClick(Sender: TObject);
begin
  TcpIpEdit.Enabled:=TcpIpCheckBox.Checked;
  if TcpIpCheckBox.Checked then
    TcpIpEdit.Color:=clWindow
  else
    TcpIpEdit.Color:=clBtnFace;
end;

end.
