@echo off

REM To avoid complicated and maybe impossible tasks in windows batch scripts,
REM this script will execute another linux shell script using cygwin.

set BASH=env\win32\bin\bash.exe
set DATE_COMMAND=env\win32\bin\date.exe

REM Converts a windows path to a linux path for cygwin (/cygdrive/c/...).
for /f %%s in ('env\win32\bin\cygpath.exe -u "%DATE_COMMAND%"') do set DATE_COMMAND=%%s

%BASH% updateversion.sh %DATE_COMMAND%
exit
