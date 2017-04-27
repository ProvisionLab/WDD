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

	if( argc < 2 || argc > 6 )
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
		if( strCommand == _T("restore_to") )
		{
			if( bHaveIni ? argc < 6 : argc < 4 )
			{
				Usage();
				return 1;
			}
			strRestoreToDir = bHaveIni ? argv[5] : argv[3];
		}
		else if( bHaveIni ? argc < 5 : argc < 3 )
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
