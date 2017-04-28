BUILD:

The project was built and tested only on Windows 10 x64 at the moment.
There should be no big afforts to build and run it on another Windows version if appropriate SDK/DDK is installed

Requirements to build the project:
1. Download and install Visual Studio 2015:
https://www.visualstudio.com/downloads/
2. Download and install Window SDK for Windows 10
https://go.microsoft.com/fwlink/p/?LinkID=845298
3. Download and install WDK for Windows 10
https://go.microsoft.com/fwlink/p/?LinkId=845980



INSTALL:
Install driver on Windows 10 x64:
1. Install cebackup.inf by right-click Install in File Explorer
2.copy cebackup.sys C:\Windows\System32\drivers\
fltmc load cebackup

To run backup application
1. Fill cebackup.ini with appropriate insformation
2. Run ceuser.exe as standalone application. It will create backups upon writing to files from [Include*] in cebackup.ini



TEST:
1. Fill cebackup.ini with appropriate files and directories you want to monitor
2. Make sure cebackup is installed and run, ceuser.exe launched, see Install.txt
3. Write to the file form the [Included*] list of cebackup.ini, check Destination directory
4. Try to list of backup files: restore.exe listall
5. Try to restore some backup file, e.g: restore.exe restore_to example.txt.1 c:\temp
6. Try to restore some backup file to the original location, e.g: restore.exe restore example.txt.1
