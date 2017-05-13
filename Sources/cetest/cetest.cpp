#include "usercommon.h"
#include "test.h"

VOID Usage ()
{
    _tprintf( _T("Tests CeedoBackup filter\n") );
    _tprintf( _T("Usage: cetest [-ini cebackup.ini]\n") );
}

int _cdecl main ( _In_ int argc, _In_reads_(argc) TCHAR* argv[] )
{
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

    CTest test;

    if( ! test.Run( strIniPath ) )
	{
		ERROR_PRINT( _T("\nSome tests failed\n") );
		return 1;
	}

	ERROR_PRINT( _T("\nAll tests are OK\n") );
    return 0;
}
