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
        ERROR_PRINT( _T("One instance of application is already running") );
        return false;
    }

	CSettings settings;
	if( ! settings.Init( IniPath ) )
        return false;

	if( Command == _T("listall") )
		return ListFiles( settings.Destination, true, _T(""), false );
	else if( Command == _T("list") )
		return ListFiles( settings.Destination, false, Path, IsPath );
	else if( Command == _T("restore") )
		return Restore( settings.Destination, false, Path );
	else if( Command == _T("restore_to") )
		return Restore( settings.Destination, false, Path, RestoreToDir );
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

		StartDirectory += _T("\\") + Utils::MapToDestination( pd.Directory );

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
			INFO_PRINT( _T("Found matches(%lld):\n"), arrFiles.size() );
			for( int i=0; i<arrFiles.size(); i++ )
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
				ERROR_PRINT( _T("ERROR: FindFirstFile failed in %s, error=%s\n"), Directory.c_str(), Utils::GetLastErrorString().c_str() );
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
				if( (All || !IsPath) && strName != _T(".") && strName != _T("..") )
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
				Path = Path.substr( Destination.size() + 1 );
				Path = Utils::MapToOriginal( Path );
				Files.push_back( Path );
            }
        } while( ::FindNextFile( hFind, &ffd ) );

        ::FindClose( hFind );
    }

	return true;
}

bool CRestore::Restore( const tstring& Destination, bool To, const tstring& Path, const tstring& RestoreToDir )
{
	return true;
}
