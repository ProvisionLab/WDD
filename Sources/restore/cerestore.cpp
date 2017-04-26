/*
3. output all backup list                               8
   param listall
4. output backup list by file name                      2
   param list name
5. output backup list by path                           2
   param list full_name
6. restore to custom directory                          4
   param restore_to full_name_with_index dest_dir
7. restore to original location/overwrite               4
   param restore full_name_with_index
8. run one instance run only check to avoid conflicts   2
*/

#include "usercommon.h"
#include "restore.h"

VOID Usage ()
{
    _tprintf( _T("Restores backed up file\n") );
    _tprintf( _T("Usage: restore [-ini cebackup.ini] command\n") );
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

    CRestore restore;

    if( ! restore.Run( strIniPath ) )
        return 1;

    _tprintf( _T("Exiting ...") );
    return 0;
}
