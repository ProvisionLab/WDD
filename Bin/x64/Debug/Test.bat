cd C:\Max
taskkill /IM ceuser.exe
fltmc unload cebackup
rem xcopy /Y C:\DriverTest\Drivers\cebackup.sys C:\Windows\System32\drivers\
xcopy /Y C:\Max\cebackup.sys C:\Windows\System32\drivers\
fltmc load cebackup
if exist C:\Max\Test\file* del C:\Max\Test\file*
if exist C:\Max\Backup\C\Max\file1.txt.* del C:\Max\Backup\C\Max\file1.txt.*
start ceuser.exe
timeout 1
if not exist c:\Max\Test mkdir c:\Max\Test
echo 1 > C:\Max\Test\file1.txt
echo 2 > C:\Max\Test\file1.txt
echo 3 > C:\Max\Test\file1.txt
echo 1 > C:\Max\Test\file2.txt
echo 2 > C:\Max\Test\file2.txt
timeout 1
rem taskkill /IM ceuser.exe
restore.exe unknown
restore.exe listall
restore.exe -ini cebackup.ini list f
restore.exe list file2
restore.exe list file2.txt
restore.exe list c:\Max\Test\file1.txt
if exist c:\Max\restore\* del /Q c:\Max\restore\*
if not exist c:\Max\restore mkdir c:\Max\restore
restore.exe -ini cebackup.ini restore_to c:\Max\Test\file1.txt.1 c:\Max\restore
@set /p file1=<C:\Max\Backup\C\Max\Test\file1.txt.1
@if NOT %file1% == 1 (
echo ERROR1
echo %file1%
exit -1
)
@set /p file1=<C:\Max\Backup\C\Max\Test\file1.txt.2
@if NOT %file1% == 2 (
echo ERROR2
exit -1
)
@set /p file1=<C:\Max\Test\file1.txt
@if NOT %file1% == 3 (
echo ERROR3
exit -1
)
restore.exe restore c:\Max\Test\file1.txt.1
@set /p file1=<C:\Max\Test\file1.txt
@if NOT %file1% == 1 (
echo ERROR4
exit -1
)
