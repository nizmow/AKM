:: MCMD HELP SYSTEM
SET HLP_PATH=C:\BIN\HELP\
SET HELPFILE=%HLP_PATH%%1.hlp
:echo %HELPFILE%
:echo %HLP_PATH%.hlp

IF NOT %HELPFILE% == %HLP_PATH%.hlp GOTO _help
GOTO _help_topics

:_help_topics
echo Help topics:

echo attrib,break,cd,cls,ck,copy,date,del,dir,echo,end,exit
echo find,for,free,goto,if,md,more,mv,path,pause,prompt
echo rd,ren,shift,set,sort,time,touch,tree,type,ver,verify
end

:_help
echo ""
more <%HELPFILE%
echo ""
end
