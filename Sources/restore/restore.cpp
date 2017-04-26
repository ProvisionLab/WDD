#include "usercommon.h"
#include "restore.h"
#include "drvcommon.h"
#include "Settings.h"
#include "Utils.h"

//  Default and Maximum number of threads.
#define BACKUP_REQUEST_COUNT            5
#define BACKUP_DEFAULT_THREAD_COUNT     2
#define BACKUP_MAX_THREAD_COUNT         64

bool CRestore::Run( const tstring& IniPath )
{
    if( IsRunning() )
    {
        ERROR_PRINT( _T("One instance of application is already running") );
        return false;
    }

	CSettings settings;
	if( ! settings.Init( IniPath ) )
        return false;

//Cleanup:
    return true;
}

bool CRestore::IsRunning()
{
    bool ret = false;
    HANDLE hMutex = ::CreateMutex( NULL, FALSE, L"____CE_RESTORE_APPLICATION____" );
    ret = ::GetLastError() == ERROR_ALREADY_EXISTS;
    if( hMutex )
        ::ReleaseMutex( hMutex );;

    return ret;
}
