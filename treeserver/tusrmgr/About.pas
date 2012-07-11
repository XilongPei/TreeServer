{*************************************************************************}
{    Module Name:                                                         }
{        About.pas                                                        }
{    Abstract:                                                            }
{        TreeServer User Maganer about information dialog.                     }
{    Author:                                                              }
{        Niu Jingyu                                                       }
{    Copyright (c) NiuJingyu 1998  China Railway Software Corporation.    }
{*************************************************************************}

unit About;

interface

uses Windows, SysUtils, Classes, Graphics, Forms, Controls, StdCtrls,
  Buttons, ExtCtrls;

type
  TAboutBox = class(TForm)
    Panel1: TPanel;
    ProgramIcon: TImage;
    ProductName1: TLabel;
    Version: TLabel;
    Copyright: TLabel;
    Comments: TLabel;
    OKButton: TButton;
    Bevel1: TBevel;
    ProductName2: TLabel;
    RegistMark: TLabel;
    ProductOwner: TLabel;
    ProductOrg: TLabel;
    Bevel2: TBevel;
    OSInformation: TLabel;
    MemoryInfo: TLabel;
    procedure FormCreate(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  AboutBox: TAboutBox;

implementation

{$R *.DFM}


procedure TAboutBox.FormCreate(Sender: TObject);
var
    OSVersionInfo: TOSVersionInfo;
    MemStat: TMemoryStatus;
    MemSize: Integer;
begin
    RegistMark.Left := ProductName1.Left + ProductName1.Width + 2;
    OSVersionInfo.dwOSVersionInfoSize := SizeOf( TOSVERSIONINFO );
    GetVersionEx( OSVersionInfo );
    if OSVersionInfo.dwPlatformId = VER_PLATFORM_WIN32_NT then begin
        OSInformation.Caption := 'Windows NT '
            + IntToStr( OSVersionInfo.dwMajorVersion ) + '.'
            + IntToStr( OSVersionInfo.dwMinorVersion )
            + ' (Build ' + IntToStr( OSVersionInfo.dwBuildNumber ) + ': '
            + OSVersionInfo.szCSDVersion + ')';
        MemoryInfo.Caption := 'Windows NT ';
    end
    else if OSVersionInfo.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS then begin
        OSInformation.Caption := 'Windows 95 '
            + IntToStr( OSVersionInfo.dwMajorVersion ) + '.'
            + IntToStr( OSVersionInfo.dwMinorVersion )
            + ' (Build ' + IntToStr( OSVersionInfo.dwBuildNumber and $0000FFFF )
            + ')';
        MemoryInfo.Caption := 'Windows 95 ';
    end;

    MemStat.dwLength := SizeOf( TMemoryStatus );
    GlobalMemoryStatus( MemStat );
    MemSize := MemStat.dwTotalPhys div 1024;
    MemoryInfo.Caption := MemoryInfo.Caption + 'ø…”√ƒ⁄¥Ê:    '
        + IntToStr( MemSize div 1000 ) + ','
        + IntToStr( MemSize mod 1000 ) + ' KB';
end;

end.

