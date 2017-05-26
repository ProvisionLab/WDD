#include "usercommon.h"
#include "restore.h"
#include "drvcommon.h"
#include "Settings.h"
#include "Utils.h"

//  Default and Maximum number of threads.
#define BACKUP_REQUEST_COUNT            5
#define BACKUP_DEFAULT_THREAD_COUNT     2
#define BACKUP_MAX_THREAD_COUNT         64

bool CRestore::Run( const tstring& IniPath, const tstring& Command, const tstring& Path, bool IsPath, const tstring& RestoreToDir )
{
    if( IsRunning() )
    {
        ERROR_PRINT( _T("RESTORE: One instance of application is already running\n") );
        return false;
    }

	CSettings settings;
    tstring error;
	if( ! settings.Init( IniPath, error ) )
        return false;

	if( Command == _T("listall") )
		return ListFiles( settings.Destination, true, _T(""), false );
	else if( Command == _T("list") )
		return ListFiles( settings.Destination, false, Path, IsPath );
	else if( Command == _T("restore") )
		return Restore( settings.Destination, Path );
	else if( Command == _T("restore_to") )
		return Restore( settings.Destination, Path, RestoreToDir );
	else
	{
		ERROR_PRINT( _T("Unknown command") );
		return false;
	}

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

bool CRestore::ListFiles( const tstring& Destination, bool All, const tstring& Path, bool IsPath )
{
	tstring StartDirectory = Destination;
	Utils::CPathDetails pd;
	std::vector<tstring> arrFiles;
	bool ret;

	if( All )
	{
		ret = IterateDirectories( Destination, StartDirectory, _T(""), All, IsPath, arrFiles );
	}
	else if( IsPath )
	{
		if( ! pd.Parse( false, Path ) )
		{
			ERROR_PRINT( _T("ERROR: Failed to parse %s\n"), Path.c_str() );
			return false;
		}

		StartDirectory = Utils::MapToDestination( Destination, pd.Directory );

		ret = IterateDirectories( Destination, StartDirectory, pd.Name, All, IsPath, arrFiles );
	}
	else
	{
		ret = IterateDirectories( Destination, StartDirectory, Path, All, IsPath, arrFiles );
	}

	if( ret )
	{
		if( arrFiles.size() )
		{
			INFO_PRINT( _T("Found matches(%d):\n"), (int)arrFiles.size() );
			for( size_t i=0; i<arrFiles.size(); i++ )
			{
				INFO_PRINT( _T("%s\n"), arrFiles[i].c_str() );
			}
		}
		else
		{
			if( All )
			{
				INFO_PRINT( _T("No files where backed up yet\n") );
			}
			else
			{
				INFO_PRINT( _T("No files where found with name '%s'\n"), Path.c_str() );
			}
		}
	}

	return false;
}

bool CRestore::IterateDirectories( const tstring& Destination, const tstring& Directory, const tstring& Name, bool All, bool IsPath, std::vector<tstring>& Files )
{	
    if( Utils::DoesDirectoryExists( Directory ) )
    {
		WIN32_FIND_DATA ffd = {0};
        HANDLE hFind = INVALID_HANDLE_VALUE;
		tstring strSearchFor;
		if( IsPath )
			strSearchFor = Directory + _T("\\") + Name + _T("*.*");
		else
			strSearchFor = Directory + _T("\\*");

        hFind = ::FindFirstFile( strSearchFor.c_str(), &ffd );
        if( hFind == INVALID_HANDLE_VALUE )
        {
			DWORD status = ::GetLastError();
			if( status != ERROR_FILE_NOT_FOUND )
			{
				ERROR_PRINT( _T("RESTORE: ERROR: FindFirstFile failed in %s, error=%s\n"), Directory.c_str(), Utils::GetLastErrorString().c_str() );
				return false;
			}
			else
				return true;
        }

        do
        {
			tstring strName = ffd.cFileName;
            if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
				if( (All || ! IsPath) && strName != _T(".") && strName != _T("..") )
					if( ! IterateDirectories( Destination, Directory + _T("\\") + strName, Name, All, IsPath, Files ) )
						return false;
			}
			else
			{
				if( ! All )
				{
					if( Utils::ToLower( strName.substr( 0, Name.size() ) ) != Utils::ToLower( Name ) )
						continue;
				}

				tstring Path = Directory + _T("\\") + strName;
				Path = Utils::MapToOriginal( Destination, Path );
				Files.push_back( Path );
            }
        } while( ::FindNextFile( hFind, &ffd ) );

        ::FindClose( hFind );
    }

	return true;
}

bool CRestore::Restore( const tstring& Destination, const tstring& Path, const tstring& RestoreToDir )
{
    HANDLE hSrcFile = NULL;
    HANDLE hDstFile = NULL;
	if( Path.rfind( _T('.') ) == tstring::npos )
	{
        ERROR_PRINT( _T("RESTORE: ERROR: Path without index was provided: %s\n"), Path.c_str() );
        return false;
	}

    tstring strPath = Path;
    tstring::size_type pos1 = Path.rfind( _T('.') );
    tstring strDel = _T(".DELETED");
    tstring::size_type pos2 = Path.rfind( strDel, pos1 );
	if( pos2 != tstring::npos && pos2 + strDel.size() == pos1 )
	{
        strPath = Path.substr( 0, pos2 ) + Path.substr( pos1 );
	}

    Utils::CPathDetails pd;
	if( ! pd.Parse( true, strPath ) )
	{
        ERROR_PRINT( _T("RESTORE: ERROR: Failed parse provided path: %s\n"), Path.c_str() );
        return false;
	}

	int iIndex = _tstoi( pd.Index.c_str() );
	if( iIndex <= 0 )
	{
        ERROR_PRINT( _T("RESTORE: ERROR: No Index was found in provided path: %s\n"), Path.c_str() );
        return false;
	}

	HANDLE hPort = NULL;
    HRESULT hr = ::FilterConnectCommunicationPort( RESTORE_PORT_NAME, 0, NULL, 0, NULL, &hPort );
    if( IS_ERROR( hr ) )
    {
        ERROR_PRINT( _T("RESTORE: ERROR: Failed connect to RESTORE filter port: 0x%08x\n"), hr );
        return false;
    }

	tstring strDestPath = Path;
	tstring strSrcPath;
	if( RestoreToDir.size() )
	{
		strSrcPath = Utils::MapToDestination( Destination, Path );
		strDestPath = RestoreToDir + _T("\\") + pd.Name;
	}
	else
	{
		strSrcPath = Utils::MapToDestination( Destination, Path );
		strDestPath = pd.Directory + _T("\\") + pd.Name;
	}

	INFO_PRINT( _T("RESTORE: INFO: Copying %s to %s\n"), strSrcPath.c_str(), strDestPath.c_str() );
	if( ! ::CopyFile( strSrcPath.c_str(), strDestPath.c_str(), FALSE ) )
    {
		if( RestoreToDir.size() )
			ERROR_PRINT( _T("RESTORE: ERROR: Failed to restore file '%s' to _restore_to_ directory '%s'. Error: %s\n"), strDestPath.c_str(), RestoreToDir.c_str(), Utils::GetLastErrorString().c_str() );
		else
			ERROR_PRINT( _T("RESTORE: ERROR: Failed to restore file '%s'. Error: %s\n"), strDestPath.c_str(), Utils::GetLastErrorString().c_str() );
        goto Cleanup;
    }

    hSrcFile = ::CreateFile( strSrcPath.c_str(), FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL );
    if( hSrcFile == INVALID_HANDLE_VALUE )
    {
        ERROR_PRINT( _T("RESTORE: ERROR: Restore: CreateFile: Failed to delete %s\n"), strSrcPath.c_str() );
		goto Cleanup;
    }

    hDstFile = ::CreateFile( strDestPath.c_str(), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL );
    if( hDstFile == INVALID_HANDLE_VALUE )
    {
        ERROR_PRINT( _T("RESTORE: ERROR: Restore: CreateFile: Failed to delete %s\n"), strDestPath.c_str() );
		goto Cleanup;
    }

    FILETIME CreationTime, LastAccessTime, LastWriteTime;
    if( ! ::GetFileTime( hSrcFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        ERROR_PRINT( _T("RESTORE: ERROR: GetFileTime( %s ) failed, error=%s\n"), strSrcPath.c_str(), Utils::GetLastErrorString().c_str() );
        OutputDebugString( (tstring( _T("RESTORE: ERROR: GetFileTime failed: ") ) + strSrcPath + _T("\n")).c_str() );
        goto Cleanup;
    }

    if( ! ::SetFileTime( hDstFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        ERROR_PRINT( _T("RESTORE: ERROR: SetFileTime( %s ) failed, error=%s\n"), strDestPath.c_str(), Utils::GetLastErrorString().c_str() );
        OutputDebugString( (tstring( _T("RESTORE: ERROR: SetFileTime failed: ") ) + strDestPath + _T("\n")).c_str() );
        goto Cleanup;
    }


Cleanup:
    if( hSrcFile )
        ::CloseHandle( hSrcFile );

    if( hDstFile )
        ::CloseHandle( hDstFile );

    if( hPort )
		::CloseHandle( hPort );

	return true;
}
