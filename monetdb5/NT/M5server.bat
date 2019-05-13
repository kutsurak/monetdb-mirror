@REM This Source Code Form is subject to the terms of the Mozilla Public
@REM License, v. 2.0.  If a copy of the MPL was not distributed with this
@REM file, You can obtain one at http://mozilla.org/MPL/2.0/.
@REM
@REM Copyright 1997 - July 2008 CWI, August 2008 - 2019 MonetDB B.V.

@echo off

setlocal

rem figure out the folder name
set MONETDB=%~dp0

rem remove the final backslash from the path
set MONETDB=%MONETDB:~0,-1%

rem extend the search path with our EXE and DLL folders
set PATH=%MONETDB%\bin;%MONETDB%\lib;%MONETDB%\lib\MonetDB5;%PATH%

rem prepare the arguments to mserver5 to tell it where to put the dbfarm

if "%APPDATA%" == "" goto usevar
rem if the APPDATA variable does exist, put the database there
set MONETDBDIR=%APPDATA%\MonetDB5
set MONETDBFARM="--dbpath=%MONETDBDIR%\dbfarm\demo"
if not exist %MONETDBDIR%\dbfarm ( mkdir %MONETDBDIR%\dbfarm )
goto skipusevar
:usevar
rem if the APPDATA variable does not exist, put the database in the
rem installation folder (i.e. default location, so no command line argument)
set MONETDBDIR=%MONETDB%\var\MonetDB5
set MONETDBFARM=
:skipusevar

rem the SQL log directory used to be in %MONETDBDIR%, but we now
rem prefer it inside the dbfarm, so move it there

if not exist "%MONETDBDIR%\sql_logs" goto skipmove
for /d %%i in ("%MONETDBDIR%"\sql_logs\*) do move "%%i" "%MONETDBDIR%\dbfarm"\%%~ni\sql_logs
rmdir "%MONETDBDIR%\sql_logs"
:skipmove

set MONETDBPYTHONUDF=embedded_py=false

if not exist "%MONETDB%\pyapi_locatepython3.bat" goto skippython3
call "%MONETDB%\pyapi_locatepython3.bat"
if not "%MONETDBPYTHONUDF%" == "embedded_py=false" goto skippython2
:skippython3
if not exist "%MONETDB%\pyapi_locatepython2.bat" goto skippython2
call "%MONETDB%\pyapi_locatepython2.bat"
:skippython2

rem start the real server
"%MONETDB%\bin\mserver5.exe" --set "prefix=%MONETDB%" --set %MONETDBPYTHONUDF% --set "exec_prefix=%MONETDB%" %MONETDBFARM% %*

if ERRORLEVEL 1 pause

endlocal
