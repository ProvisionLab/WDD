#include "usercommon.h"
#include "restore.h"
#include "drvcommon.h"
#include "Utils.h"

//  Default and Maximum number of threads.
#define BACKUP_REQUEST_COUNT            5
#define BACKUP_DEFAULT_THREAD_COUNT     2
#define BACKUP_MAX_THREAD_COUNT         64

#define UNIQUE_CERESTORE_MUTEX L"____CE_RESTORE_APPLICATION____"

CRestore::CRestore()
    : _hLockMutex(NULL)
{
}

CRestore::~CRestore()
{
    if( _hLockMutex )
    {
        ::CloseHandle( _hLockMutex );
        _hLockMutex = NULL;
    }
}

bool CRestore::Init( const tstring& IniPath, tstring& Error )
{
	if( ! _Settings.Init( IniPath, Error ) )
        return false;

    if( ! LockAccess() )
    {
        Error = _T("One instance of restore is already running");
        TRACE_ERROR( (tstring( _T("RESTORE: ") ) + Error).c_str() );
        return false;
    }

    return true;
}

bool CRestore::LockAccess()
{
    if( _hLockMutex )
        return true;

    _hLockMutex = ::CreateMutex( NULL, FALSE, UNIQUE_CERESTORE_MUTEX );
    bool exists = ::GetLastError() == ERROR_ALREADY_EXISTS;
    if( _hLockMutex )
    {
        ::ReleaseMutex( _hLockMutex );
        if( exists )
        {
            ::CloseHandle( _hLockMutex );
            _hLockMutex = NULL;
        }
    }

    return ! exists;
}

bool CRestore::IsAlreadyRunning()
{
    HANDLE hMutex = ::OpenMutex( NULL, FALSE, UNIQUE_CERESTORE_MUTEX );
    bool ret = ::GetLastError() == ERROR_ALREADY_EXISTS;

    return ret;
}

bool CRestore::ListFiles( bool All, const tstring& Path, bool IsPath, std::vector<tstring>& Files )
{
	tstring StartDirectory = _Settings.Destination;
	Utils::CPathDetails pd;
	bool ret = false;

	if( All )
	{
		ret = IterateDirectories( StartDirectory, _T(""), All, IsPath, Files );
	}
	else if( IsPath )
	{
		if( ! pd.Parse( false, Path ) )
		{
			TRACE_ERROR( _T("Failed to parse %s"), Path.c_str() );
			return false;
		}

		StartDirectory = Utils::MapToDestination( _Settings.Destination, pd.Directory );

		ret = IterateDirectories( StartDirectory, pd.Name, All, IsPath, Files );
	}
	else
	{
		ret = IterateDirectories( StartDirectory, Path, All, IsPath, Files );
	}

	return true;
}

bool CRestore::IterateDirectories( const tstring& Directory, const tstring& Name, bool All, bool IsPath, std::vector<tstring>& Files )
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
					if( ! IterateDirectories( Directory + _T("\\") + strName, Name, All, IsPath, Files ) )
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
				Path = Utils::MapToOriginal( _Settings.Destination, Path );
				Files.push_back( Path );
            }
        } while( ::FindNextFile( hFind, &ffd ) );

        ::FindClose( hFind );
    }

	return true;
}

bool CRestore::Restore( const tstring& Path, tstring& strError, const tstring& RestoreToDir )
{
    bool ret = false;

    HANDLE hSrcFile = NULL;
    HANDLE hDstFile = NULL;
	if( Path.rfind( _T('.') ) == tstring::npos )
	{
        strError = _T("RESTORE: Path without index was provided: ") + Path;
        TRACE_ERROR( strError.c_str() );
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
        strError = _T("RESTORE: Failed parse provided path: ") + Path;
        TRACE_ERROR( strError.c_str() );
        return false;
	}

	int iIndex = _tstoi( pd.Index.c_str() );
	if( iIndex <= 0 )
	{
        strError = _T("RESTORE: No Index was found in provided path: ") + Path;
        TRACE_ERROR( strError.c_str() );
        return false;
	}

	HANDLE hPort = NULL;
    HRESULT hr = ::FilterConnectCommunicationPort( RESTORE_PORT_NAME, 0, NULL, 0, NULL, &hPort );
    if( IS_ERROR( hr ) )
    {
        std::wstringstream oss;
        oss << _T("RESTORE: Failed connect to RESTORE filter port: ") << hr;
        strError = oss.str();
        TRACE_ERROR( strError.c_str() );
        return false;
    }

	tstring strDestPath = Path;
	tstring strSrcPath;
	if( RestoreToDir.size() )
	{
		strSrcPath = Utils::MapToDestination( _Settings.Destination, Path );
		strDestPath = RestoreToDir + _T("\\") + pd.Name;
	}
	else
	{
		strSrcPath = Utils::MapToDestination( _Settings.Destination, Path );
		strDestPath = pd.Directory + _T("\\") + pd.Name;
	}

	TRACE_INFO( _T("RESTORE: Copying %s to %s"), strSrcPath.c_str(), strDestPath.c_str() );
	if( ! ::CopyFile( strSrcPath.c_str(), strDestPath.c_str(), FALSE ) )
    {
		if( RestoreToDir.size() )
        {
            strError = _T("RESTORE: CopyFile failed: '") + strSrcPath + _T("' to RestoreTo '") + RestoreToDir + _T("'. Error: ") + Utils::GetLastErrorString();
            TRACE_ERROR( strError.c_str() );
        }
		else
        {
            strError = _T("RESTORE: CopyFile failed: '") + strSrcPath + _T("' to Restore '") + pd.Directory + _T("'. Error: ") + Utils::GetLastErrorString();
            TRACE_ERROR( strError.c_str() );
        }
        goto Cleanup;
    }

    hSrcFile = ::CreateFile( strSrcPath.c_str(), FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL );
    if( hSrcFile == INVALID_HANDLE_VALUE )
    {
        strError = _T("RESTORE: Restore: CreateFile: Failed to delete ") + strSrcPath;
        TRACE_ERROR( strError.c_str() );
		goto Cleanup;
    }

    hDstFile = ::CreateFile( strDestPath.c_str(), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL );
    if( hDstFile == INVALID_HANDLE_VALUE )
    {
        strError = _T("RESTORE: Restore: CreateFile: Failed to delete ") + strDestPath;
        TRACE_ERROR( strError.c_str() );
		goto Cleanup;
    }

    FILETIME CreationTime, LastAccessTime, LastWriteTime;
    if( ! ::GetFileTime( hSrcFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        strError = _T("RESTORE: GetFileTime( ") + strSrcPath + _T(" ) failed, error=") + Utils::GetLastErrorString();
        TRACE_ERROR( strError.c_str() );
        goto Cleanup;
    }

    if( ! ::SetFileTime( hDstFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        strError = _T("RESTORE: SetFileTime( ") + strDestPath + _T(" ) failed, error=") + Utils::GetLastErrorString();
        TRACE_ERROR( strError.c_str() );
        goto Cleanup;
    }

    ret = true;

Cleanup:
    if( hSrcFile )
        ::CloseHandle( hSrcFile );

    if( hDstFile )
        ::CloseHandle( hDstFile );

    if( hPort )
		::CloseHandle( hPort );

	return ret;
}
