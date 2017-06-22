#include "ceuserlib.h"
#include "client.h"
#include "restore.h"

CRestore* g_pRestore;
CBackupClient* g_pService = NULL;
tstring g_strLastError;
bool g_LogInitialize = false;

static void __cdecl InitializeLog()
{
    if( ! g_LogInitialize )
    {
        CLog::instance().Init( _T("ceuserlib.log") );
        g_LogInitialize = true;
    }
}

ServerBackupCallback g_BackupEvent = NULL;
ServerCleanupCallback g_CleanupEvent = NULL;

static TCHAR* __cdecl AllocateCSharpString( const tstring& str )
{
    ULONG ulSize = (str.size() + 1 ) * sizeof(TCHAR);
    TCHAR* pszReturn = (TCHAR*)::CoTaskMemAlloc( ulSize );
    _tcscpy_s( pszReturn, str.size() + 1, str.c_str() );

    return pszReturn;
}

CEUSERLIB_API const wchar_t* __cdecl GetLastErrorString()
{
    return AllocateCSharpString(  g_strLastError );
}

CEUSERLIB_API CeUserLib_Retval __cdecl Init()
{
    InitializeLog();

    if( g_pService )
    {
        g_strLastError = _T("Already initialized");
        return CEUSERLIB_AlreadyInitialized;
    }

    if( CBackupClient::IsAlreadyRunning() )
    {
        g_strLastError = _T("One instance of driver client is already initialized");
        return CEUSERLIB_OneInstanceOnly;
    }

    g_pService = new CBackupClient();
    if( ! g_pService )
    {
        g_strLastError = _T("Out of memory");
        return CEUSERLIB_OutOfMemory;
    }

    if( ! g_pService->LockAccess() )
    {
        g_strLastError = _T("One instance of driver client is already initialized");
        delete g_pService;
        g_pService = NULL;
        return CEUSERLIB_OneInstanceOnly;
    }

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl Uninit()
{
    if( ! g_pService )
    {
        g_strLastError = _T("Not Initialized");
        return CEUSERLIB_NotInitialized;
    }

    if( g_pService->IsStarted() )
        g_pService->Stop();

    delete g_pService;
    g_pService = NULL;

    return CEUSERLIB_OK;
}

CEUSERLIB_API int __cdecl IsDriverStarted()
{
    return CBackupClient::IsDriverStarted();
}

CEUSERLIB_API int __cdecl IsBackupStarted()
{
    if( ! g_pService )
        return 0;

    return g_pService->IsStarted();
}

CEUSERLIB_API CeUserLib_Retval __cdecl Start( const wchar_t* IniPath )
{
    if( ! g_pService )
    {
        g_strLastError = _T("Not Initialized");
        return CEUSERLIB_NotInitialized;
    }

    tstring strError, strIniPath;
    if( IniPath )
        strIniPath = IniPath;
    else
        strIniPath = _T("cebackup.ini");

    g_pService->SetBackupCallback( g_BackupEvent );
    g_pService->SetCleanupCallback( g_CleanupEvent );

    if( ! g_pService->Start( strIniPath, strError, true ) )
    {
        g_strLastError = strError;
        return CEUSERLIB_BackupIsNotStarted;
    }

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl Stop()
{
    if( ! g_pService )
    {
        g_strLastError = _T("Not Initialized");
        return CEUSERLIB_NotInitialized;
    }

    if( ! g_pService->IsStarted() )
    {
        g_strLastError = _T("Not Started");
        return CEUSERLIB_DriverIsNotStarted;
    }

    if( ! g_pService->Stop() )
    {
        g_strLastError = _T("Failed to stop");
        return CEUSERLIB_Failed;
    }

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl ReloadConfig( const wchar_t* IniPath )
{
    if( ! g_pService )
    {
        g_strLastError = _T("Not Initialized");
        return CEUSERLIB_NotInitialized;
    }

    if( ! g_pService->IsStarted() )
    {
        g_strLastError = _T("Not Started");
        return CEUSERLIB_DriverIsNotStarted;
    }

    tstring strError;
    if( ! g_pService->ReloadConfig( IniPath, strError ) )
    {
        g_strLastError = strError;
        return CEUSERLIB_ConfigurationError;
    }

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl SubscribeForBackupEvents( ServerBackupCallback CallBack )
{
    if( g_BackupEvent )
    {
        g_strLastError = _T("One BackupEvent is already subscribed. Unsubscribe first");
        return CEUSERLIB_AlreadySubscribed;
    }
    
    g_BackupEvent = CallBack;
    if( g_pService )
        g_pService->SetBackupCallback( g_BackupEvent );

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl UnsubscribeFromBackupEvents()
{
    g_BackupEvent = NULL;

    g_pService->SetBackupCallback( NULL );

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl SubscribeForCleanupEvents( ServerCleanupCallback CallBack )
{
    if( g_CleanupEvent )
    {
        g_strLastError = _T("One ClenupEvent is already subscribed. Unsubscribe first");
        return CEUSERLIB_AlreadySubscribed;
    }

    g_CleanupEvent = CallBack;
    if( g_pService )
        g_pService->SetCleanupCallback( g_CleanupEvent );

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl UnsubscribeFromCleanupEvents()
{
    g_CleanupEvent = NULL;

    g_pService->SetCleanupCallback( NULL );

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl Restore_Init( const wchar_t* IniPath )
{
    InitializeLog();

    if( g_pRestore )
    {
        g_strLastError = _T("Already initialized");
        return CEUSERLIB_AlreadyInitialized;
    }

    if( CRestore::IsAlreadyRunning() )
    {
        g_strLastError = _T("One instance of driver client is already initialized");
        return CEUSERLIB_OneInstanceOnly;
    }

    g_pRestore = new CRestore();
    if( ! g_pRestore )
    {
        g_strLastError = _T("Out of memory");
        return CEUSERLIB_OutOfMemory;
    }

    if( ! g_pRestore->LockAccess() )
    {
        g_strLastError = _T("One instance of restore is already initialized");
        delete g_pRestore;
        g_pRestore = NULL;
        return CEUSERLIB_OneInstanceOnly;
    }

    tstring strIniPath, strError;
    if( IniPath )
        strIniPath = IniPath;
    else
        strIniPath = _T("cebackup.ini");

    if( ! g_pRestore->Init( strIniPath, strError ) )
    {
        g_strLastError = strError;
        return CEUSERLIB_ConfigurationError;
    }

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl Restore_Uninit()
{
    if( ! g_pRestore )
    {
        g_strLastError = _T("Not Initialized");
        return CEUSERLIB_NotInitialized;
    }

    delete g_pRestore;
    g_pRestore = NULL;

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl Restore_ListAll( wchar_t*** ppRestorePath, unsigned int* PathCount )
{
    if( ! g_pRestore )
    {
        g_strLastError = _T("Not Initialized");
        return CEUSERLIB_NotInitialized;
    }

    if( ! PathCount )
    {
        g_strLastError = _T("PathCount is NULL");
        return CEUSERLIB_InvalidParameter;
    }

    if( ! ppRestorePath )
    {
        g_strLastError = _T("ppRestorePath is NULL");
        return CEUSERLIB_InvalidParameter;
    }

    *PathCount = 0;
    *ppRestorePath = NULL;

    std::vector<tstring> arrFiles;
    bool ret = g_pRestore->ListFiles( true, _T(""), false, arrFiles );

    if( ! ret )
    {
        g_strLastError = _T("ListFiles failed");
        return CEUSERLIB_Failed;
    }

    *PathCount = arrFiles.size();
    size_t stSizeOfArray = sizeof(wchar_t*) * arrFiles.size();
    *ppRestorePath = (wchar_t**)::CoTaskMemAlloc( stSizeOfArray );
    memset (*ppRestorePath, 0, stSizeOfArray);

    for( size_t i=0; i < arrFiles.size(); i++ )
    {
        (*ppRestorePath)[i] = AllocateCSharpString( arrFiles[i] );
    }

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl Restore_Restore( const wchar_t* BackupPath )
{
    if( ! g_pRestore )
    {
        g_strLastError = _T("Not Initialized");
        return CEUSERLIB_NotInitialized;
    }

    if( ! IsDriverStarted() )
    {
        g_strLastError = _T("Driver is not running");
        return CEUSERLIB_DriverIsNotStarted;
    }

    tstring strError;
    bool ret = g_pRestore->Restore( BackupPath, strError );
    if( ! ret )
    {
        g_strLastError = strError;
        return CEUSERLIB_Failed;
    }

    return CEUSERLIB_OK;
}

CEUSERLIB_API CeUserLib_Retval __cdecl Restore_RestoreTo( const wchar_t* BackupPath, const wchar_t* ToDirectory )
{
    if( ! g_pRestore )
    {
        g_strLastError = _T("Not Initialized");
        return CEUSERLIB_NotInitialized;
    }

    if( ! IsDriverStarted() )
    {
        g_strLastError = _T("Driver is not running");
        return CEUSERLIB_DriverIsNotStarted;
    }

    tstring strError;
    bool ret = g_pRestore->Restore( BackupPath, strError, ToDirectory );
    if( ! ret )
    {
        g_strLastError = strError;
        return CEUSERLIB_Failed;
    }

    return CEUSERLIB_OK;
}
