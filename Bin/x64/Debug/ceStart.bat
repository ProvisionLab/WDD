cd C:\Max
taskkill /IM ceuser.exe
fltmc unload cebackup
xcopy /Y C:\Max\cebackup.sys C:\Windows\System32\drivers\
fltmc load cebackup
timeout 1
start ceuser.exe
