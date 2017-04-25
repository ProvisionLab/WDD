#include "common.h"
#include "client.h"

VOID Usage ()
{
    _tprintf( _T("Connects to the CeedoBackup filter and backup files upon change\n") );
    _tprintf( _T("Usage: ceuser -ini [cebackup.ini]\n") );
}

int _cdecl main ( _In_ int argc, _In_reads_(argc) TCHAR* argv[] )
{
    _tprintf( _T("Starting ...\n") );
    std::wstring strIniPath = _T("cebackup.ini");

    if( argc > 1 )
    {
        if( _tcscmp( argv[1], _T("/?") ) )
        {
            Usage();
            return 1;
        }

        if( ! _tcscmp( argv[1], _T("-ini") ) )
        {
            Usage();
            return 1;
        }
        
        if( argc > 3 )
        {
            Usage();
            return 1;
        }

        strIniPath = argv[2];
    }

    CBackupClient client;

    if( ! client.Run( strIniPath ) )
        return 1;

    _tprintf( _T("Exiting ...") );
    return 0;
}
