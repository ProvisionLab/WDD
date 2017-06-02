#include "usercommon.h"
#include "restore.h"

VOID Usage ()
{
    _tprintf( _T("Restores backed up file\n") );
    _tprintf( _T("Usage: restore [-ini cebackup.ini] command\n\tCommands: listall | list name | list path | restore path.Index | restore_to path.Index dir\n") );
}

bool Run( const tstring& IniPath, const tstring& Command, const tstring& Path, bool IsPath, const tstring& RestoreToDir )
{
    bool ret = false;
	CRestore restore;
    std::vector<tstring> arrFiles;

    if( CRestore::IsAlreadyRunning() )
    {
        TRACE_ERROR( _T("RESTORE: One instance of application is already running") );
        return false;
    }

    tstring error;
	if( ! restore.Init( IniPath, error ) )
        return false;

	if( Command == _T("listall") )
    {
        ret = restore.ListFiles( true, _T(""), false, arrFiles );
    }
	else if( Command == _T("list") )
    {
		ret = restore.ListFiles( false, Path, IsPath, arrFiles );
    }
	else if( Command == _T("restore") )
		ret = restore.Restore( Path );
	else if( Command == _T("restore_to") )
		ret = restore.Restore( Path, RestoreToDir );
	else
	{
		TRACE_ERROR( _T("Unknown command") );
		return false;
	}

    if( Command == _T("listall") || Command == _T("list") )
    {
	    if( ret )
	    {
		    if( arrFiles.size() )
		    {
			    TRACE_INFO( _T("Found matches(%d):"), (int)arrFiles.size() );
			    for( size_t i=0; i<arrFiles.size(); i++ )
			    {
				    TRACE_INFO( _T("%s"), arrFiles[i].c_str() );
			    }
		    }
		    else
		    {
			    if( Command == _T("listall") )
			    {
				    TRACE_INFO( _T("No files where backed up yet") );
			    }
			    else
			    {
				    TRACE_INFO( _T("No files where found with name '%s'"), Path.c_str() );
			    }
		    }
	    }
    }

    return ret;
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

    if( ! Run( strIniPath, strCommand, strPath, bIsPath, strRestoreToDir ) )
        return 1;

    return 0;
}
