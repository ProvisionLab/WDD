// ceuserlib.cpp : Defines the exported functions for the DLL application.

#include "ceuserlib.h"
#include "client.h"

CBackupClient* g_pService = NULL;
tstring g_strLastError;

CEUSERLIB_API bool Start( const wchar_t* IniPath )
{
    if( g_pService )
    {
        g_strLastError = _T("Already started");
        return false;
    }

    g_pService = new CBackupClient();
    if( ! g_pService )
    {
        g_strLastError = _T("Out of memory");
        return false;
    }

    tstring error, strIniPath;
    if( IniPath )
        strIniPath = IniPath;
    else
        strIniPath = _T("cebackup.ini");

    if( ! g_pService->Run( strIniPath, error, true ) )
    {
        g_strLastError = error;
        return false;
    }

    return true;
}

CEUSERLIB_API bool Stop()
{
    if( ! g_pService )
    {
        g_strLastError = _T("Already stoppped");
        return false;
    }

    if( ! g_pService->Stop() )
    {
        g_strLastError = _T("Failed to stop");
        return false;
    }

    delete g_pService;
    g_pService = NULL;

    return true;
}

CEUSERLIB_API const wchar_t* GetErrorString()
{
    return g_strLastError.c_str();
}
