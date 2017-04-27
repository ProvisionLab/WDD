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
    _tprintf( _T("Usage: restore [-ini cebackup.ini] command\n\tCommands: listall | list name | list path | restore path.Index | restore_to path.Index dir\n") );
}

int _cdecl wmain ( _In_ int argc, _In_reads_(argc) TCHAR* argv[] )
{
	bool bIsPath = false;
    tstring strIniPath = _T("cebackup.ini");
	tstring strCommand, strPath, strRestoreToDir;

	if( argc < 2 || argc > 5 )
	{
		Usage();
		return 1;
	}

	tstring strFirstParam = argv[1];
    if( strFirstParam == _T("/?") )
    {
        Usage();
        return 1;
    }

	bool bHaveIni = false;
	if( strFirstParam == _T("-ini") )
    {
		if( argc < 4 )
		{
			Usage();
			return 1;
		}
	    strIniPath = argv[2];
		bHaveIni = true;
    }

	if( bHaveIni )
		strCommand = argv[3];
	else
		strCommand = strFirstParam;

	if( strCommand == _T("listall") )
	{
	}
	else if( strCommand == _T("list") || strCommand == _T("restore") || strCommand == _T("restore_to") )
	{
		if( argc < 3 )
		{
			Usage();
			return 1;
		}
		strPath = bHaveIni ? argv[4] : argv[2];
		bIsPath = strPath.find( _T('\\') ) != tstring::npos;
	}
	else
	{
		Usage();
		return 1;
	}

	CRestore restore;

    if( ! restore.Run( strIniPath, strCommand, strPath, bIsPath, strRestoreToDir ) )
        return 1;

    return 0;
}
