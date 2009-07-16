function GetFontName(file2test:string):string;
var
	dlina2:integer;
	file2test2:string;
	j:integer;
	buff2:dword;
	FontName2:String;
	font2,FontName: Array[1..buff] of Char;

begin

	buff2:=$60;
	file2test2:=file2test;

	if cp=866 then begin
		CharToOem(pchar(file2test),@q2);
		file2test2:='';
		for j:=1 to Length(file2test) do file2test2:=file2test2+q2[j];
	end;

	flg_bad:=false;
	t:=AddFontResource(Pchar(file2test));

	if t=0 then
	begin
		flg_bad:=true;
		beep32(2);
	end;

	if not flg_bad then
	begin

		gdi32:=LoadLibrary('gdi32.dll');
		REGPROCTEST:=GetProcAddress(gdi32,'GetFontResourceInfo');
		if REGPROCTEST=nil then REGPROC:=GetProcAddress(gdi32,'GetFontResourceInfoW')
		else REGPROC:=GetProcAddress(gdi32,'GetFontResourceInfo');


		dlina2:=Length(file2test)+1;
		if cp=1251 then MultiByteToWideChar(CP_ACP,0,pchar(file2test),dlina2,@ss,dlina2*2);
		if cp=866  then MultiByteToWideChar(CP_OEMCP,0,pchar(file2test2),dlina2,@ss,dlina2*2);

		RegProc(@ss,@buff2,@font2,1);

		WideCharToMultiByte(CP_OEMCP,0,@font2,buff,@FontName,buff,nil,nil);

		RegProc(@file2test,@buff2,@font2,1);

		RemoveFontResource(Pchar(file2test));

		FontName2:='';
		j:=1;

		while FontName[j]<>#0 do begin 
			FontName2:=FontName2+FontName[j]; inc(j);
		end;

		result:=FontName2;

	end
	else
	begin
		Inc(bad_font);
		result:='E*R*R*O*R - Is no valid Font File !!!';
	end;
end;
