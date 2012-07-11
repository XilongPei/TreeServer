unit MiniParser;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ComCtrls;

type
  TForm1 = class(TForm)
    Button1: TButton;
    RichEdit1: TRichEdit;
    procedure Button1Click(Sender: TObject);
    procedure RichEditChange(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Note:Boolean;
  Form1: TForm1;

implementation

{$R *.DFM}

uses
  SrcParser;

procedure TForm1.RichEditChange(Sender: TObject);
var
   pos,len:integer;
   oldstart:integer;
   IsKey:integer;
begin
   pos:=richedit1.selstart;
   IsKey:=searchkey(pos,len,richedit1.text,Length(richedit1.text),Note);
   case IsKey of
    0:
      begin
//         if (richedit1.SelAttributes.Color =Clred) or (richedit1.SelAttributes.Color =Clblue) then
         if (richedit1.SelAttributes.Color <> Clblack) then
         begin
            OldStart:=richedit1.SelStart;
            richedit1.SelStart :=pos;
            richedit1.SelLength := len;
            richedit1.SelAttributes.Color :=ClBlack;
            richedit1.SelStart := oldstart;
         end;
      end;
    1:
      begin
         oldstart:=richedit1.SelStart;
         richedit1.SelStart :=pos;
         richedit1.SelLength := len;
         richedit1.SelAttributes.Color :=Clred;
         richedit1.SelStart := oldstart;
      end;
    2:
      begin
         oldstart:=richedit1.SelStart;
         richedit1.SelStart :=pos;
         richedit1.SelLength := 1;
         richedit1.SelAttributes.Color :=clLime;
         richedit1.SelStart := oldstart;
      end;
    3:
      if (richedit1.SelAttributes.Color <> ClGray) then
      begin
         oldstart:=richedit1.SelStart;
         richedit1.SelStart :=pos;
         richedit1.SelLength := len;
         richedit1.SelAttributes.Color :=ClGray;
         richedit1.SelStart := oldstart;
      end;
   end;
end;

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

procedure TForm1.Button1Click(Sender: TObject);

const
  MONO = False;
//  MONO = True;

//  aFile = 'd:\Borland\Delphi 2.0\Source\Vcl\Classes.pas';
//   aFile ='d:\works\richedit\miniparser.pas';
   aFile ='d:\qxxg\backrest.ask';
{  aFile = 'd:\bc5\java\src\java\applet\applet.java';}
{  aFile = 'd:\works\richedit\miniparser.dfm';
{  aFile = 'd:\devl32\tools\xview\unit1.pas' }
begin
  //if MONO then RichEdit1.Color:=clWhite else RichEdit1.Color:=clNavy;
  strToRichEdit(SourceToRtf(aFile,MONO, True), RichEdit1);
//  Richedit1.Onchange:=RichEditChange;
end;

end.

