{*************************************************************************}
{    Module Name:                                                         }
{        chngname.pas                                                     }
{    Abstract:                                                            }
{        TreeServer User change name dialog.                              }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{*************************************************************************}

unit chngname;

interface
{$A-}

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls;

type
  TChngNameForm = class(TForm)
    Change: TButton;
    Cancel: TButton;
    Label1: TLabel;
    Label2: TLabel;
    edNewUserName: TEdit;
    lbOldUserName: TLabel;
    procedure FormShow(Sender: TObject);
    procedure ChangeClick(Sender: TObject);
    procedure CancelClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  ChngNameForm: TChngNameForm;

implementation

{$R *.DFM}

uses main, AcuntCtl, PipeStruct;

procedure TChngNameForm.FormShow(Sender: TObject);
begin
     lbOldUserName.Caption := TUserMgr.UserList.Selected.Caption;
end;

procedure TChngNameForm.ChangeClick(Sender: TObject);
var
   szUsrNmeOld : Array[ 0..MAX_USER_NAME] of Char;
   szUsrNmeNew : Array[ 0..MAX_USER_NAME] of Char;
begin
     if ( lbOldUserName.Caption = '' ) or
        ( edNewUserName.Text = '' ) then exit;

     StrPCopy ( szUsrNmeOld, lbOldUserName.Caption );
     StrPCopy ( szUsrNmeNew, edNewUserName.Text );

     if not ChgUsrName ( ClihPipe, szUsrNmeOld, szUsrNmeNew ) then
     begin
         MessageBox ( 0, '更改用户名失败！', 'TreeServer 用户管理器', MB_Ok or MB_ICONSTOP );
         exit;
     end;

     ChngNameForm.Close;
     TUserMgr.N4Click( Sender );
end;

procedure TChngNameForm.CancelClick(Sender: TObject);
begin
     ChngNameForm.Close;
end;

end.
