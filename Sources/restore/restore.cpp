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
        TRACE_ERROR( _T("RESTORE: One instance of application is already running") );
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
		TRACE_ERROR( _T("Unknown command") );
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
			TRACE_ERROR( _T("Failed to parse %s"), Path.c_str() );
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
			TRACE_INFO( _T("Found matches(%d):"), (int)arrFiles.size() );
			for( size_t i=0; i<arrFiles.size(); i++ )
			{
				TRACE_INFO( _T("%s"), arrFiles[i].c_str() );
			}
		}
		else
		{
			if( All )
			{
				TRACE_INFO( _T("No files where backed up yet") );
			}
			else
			{
				TRACE_INFO( _T("No files where found with name '%s'"), Path.c_str() );
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
				TRACE_ERROR( _T("RESTORE: FindFirstFile failed in %s, error=%s"), Directory.c_str(), Utils::GetLastErrorString().c_str() );
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
        TRACE_ERROR( _T("RESTORE: Path without index was provided: %s"), Path.c_str() );
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
        TRACE_ERROR( _T("RESTORE: Failed parse provided path: %s"), Path.c_str() );
        return false;
	}

	int iIndex = _tstoi( pd.Index.c_str() );
	if( iIndex <= 0 )
	{
        TRACE_ERROR( _T("RESTORE: No Index was found in provided path: %s"), Path.c_str() );
        return false;
	}

	HANDLE hPort = NULL;
    HRESULT hr = ::FilterConnectCommunicationPort( RESTORE_PORT_NAME, 0, NULL, 0, NULL, &hPort );
    if( IS_ERROR( hr ) )
    {
        TRACE_ERROR( _T("RESTORE: Failed connect to RESTORE filter port: 0x%08x"), hr );
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

	TRACE_INFO( _T("RESTORE: Copying %s to %s"), strSrcPath.c_str(), strDestPath.c_str() );
	if( ! ::CopyFile( strSrcPath.c_str(), strDestPath.c_str(), FALSE ) )
    {
		if( RestoreToDir.size() )
			TRACE_ERROR( _T("RESTORE: Failed to restore file '%s' to _restore_to_ directory '%s'. Error: %s"), strDestPath.c_str(), RestoreToDir.c_str(), Utils::GetLastErrorString().c_str() );
		else
			TRACE_ERROR( _T("RESTORE: Failed to restore file '%s'. Error: %s"), strDestPath.c_str(), Utils::GetLastErrorString().c_str() );
        goto Cleanup;
    }

    hSrcFile = ::CreateFile( strSrcPath.c_str(), FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL );
    if( hSrcFile == INVALID_HANDLE_VALUE )
    {
        TRACE_ERROR( _T("RESTORE: Restore: CreateFile: Failed to delete %s"), strSrcPath.c_str() );
		goto Cleanup;
    }

    hDstFile = ::CreateFile( strDestPath.c_str(), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL );
    if( hDstFile == INVALID_HANDLE_VALUE )
    {
        TRACE_ERROR( _T("RESTORE: Restore: CreateFile: Failed to delete %s"), strDestPath.c_str() );
		goto Cleanup;
    }

    FILETIME CreationTime, LastAccessTime, LastWriteTime;
    if( ! ::GetFileTime( hSrcFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        TRACE_ERROR( _T("RESTORE: GetFileTime( %s ) failed, error=%s"), strSrcPath.c_str(), Utils::GetLastErrorString().c_str() );
        goto Cleanup;
    }

    if( ! ::SetFileTime( hDstFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        TRACE_ERROR( _T("RESTORE: SetFileTime( %s ) failed, error=%s"), strDestPath.c_str(), Utils::GetLastErrorString().c_str() );
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
