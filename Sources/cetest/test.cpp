/*
1. create rnd file that is [IncludeFiles] in .ini       2
2. write to file N times with random content            4
3. Check destination for N files                        4
4. Restore file N times, check content                  4
*/

#include "usercommon.h"
#include "test.h"
#include "Settings.h"
#include "Utils.h"

bool CTest::Run( const tstring& IniPath )
{
	bool ret = false;

	srand( (unsigned int)time(NULL) );

	if( ! _Settings.Init( IniPath ) )
		return false;

	INFO_PRINT( _T("[Settings] File: %s\n"), IniPath.c_str() );
	INFO_PRINT( _T("[Settings] Backup directory: %s\n"), _Settings.Destination.c_str() );

	//Clean up
	if( ! StopDriver() )
		return false;

	if( ! StopCeuser() )
		return false;

	if( Utils::DoesDirectoryExists( _Settings.Destination ) && ! Utils::RemoveDirectory( _Settings.Destination ) )
		return false;

	tstring strRestoreDirectory = _T("CetestRestore");
	if( Utils::DoesDirectoryExists( _Settings.Destination ) && ! Utils::RemoveDirectory( strRestoreDirectory ) )
		return false;

	//Start
	if( ! StartDriver() )
		return false;

	if( ! StartCeuser() )
		return false;

    if( ! Utils::CreateDirectory( strRestoreDirectory.c_str() ) )
        goto Cleanup;

	// Write to InclideFile. N files, N times, check content
	for( size_t i=0; i<_Settings.IncludedFiles.size(); i++ )
	{
		if( ! CheckFile( _Settings.IncludedFiles[i], true ) )
			return false;
	}

	// Write to IncludeDirectory
	for( size_t i=0; i<_Settings.IncludedDirectories.size(); i++ )
	{
		tstring strPath = _Settings.IncludedDirectories[i] + _T("\\") + _T("fileTmp.txt");
		if( ! CheckFile( strPath, true ) )
			return false;
	}

	// Write to ExcludedFile
	for( size_t i=0; i<_Settings.ExcludedFiles.size(); i++ )
	{
		if( ! CheckFile( _Settings.ExcludedFiles[i], false ) )
			return false;
	}

	// Write to ExcludedDirectory
	for( size_t i=0; i<_Settings.ExcludedDirectories.size(); i++ )
	{
		tstring strPath = _Settings.ExcludedDirectories[i] + _T("\\") + _T("fileTmp.txt");
		if( ! CheckFile( strPath, false ) )
			return false;
	}

	//Restore

	//Restore to

	ret = true;

Cleanup:
	if( ! StopDriver() )
		return false;

	if( ! StopCeuser() )
		return false;

	return ret;
}

bool CTest::CheckFile( const tstring& Path, bool Included )
{
	tstring strPreviousContent;
	int nWrites = rand() % 10;
	for( int j=0; j<nWrites; j++ )
	{
		tstring strContent;
		std::wstringstream oss;
		for( int k=0; k<nWrites; k++ )
		{
			oss << k;
		}

		strContent = oss.str();
		FILE* f = NULL;
		_tfopen_s( &f, Path.c_str(), _T("wb") );
		if( ! f )
		{
			ERROR_PRINT( _T("ERROR: Failed to open file %s for write\n"), Path.c_str() );
			return false;
		}
		INFO_PRINT( _T("INFO: Writing to write file %s\n"), Path.c_str() );
		size_t iWritten = fwrite( strContent.c_str(), sizeof(TCHAR), strContent.size(), f );
		if( iWritten <= 0 )
		{
			ERROR_PRINT( _T("ERROR: Failed to write file %s\n"), Path.c_str() );
			return false;
		}
		fclose( f );

		if( j > 0 )
		{
			tstring strMappedName = Utils::MapToDestination( _Settings.Destination, Path );
			std::wstringstream ossName;
			ossName << strMappedName << _T(".") << j;
			strMappedName = ossName.str();

			if( Included )
			{
				_tfopen_s( &f, strMappedName.c_str(), _T("r") );
				if( ! f )
				{
					ERROR_PRINT( _T("ERROR: Failed to open file %s for read\n"), strMappedName.c_str() );
					return false;
				}
				INFO_PRINT( _T("INFO: Checking file %s\n"), strMappedName.c_str() );
				TCHAR Buffer[MAX_PATH] = {0};
				size_t iRead = fread( Buffer, sizeof(TCHAR), sizeof(Buffer), f );
				if( iRead <= 0 )
				{
					ERROR_PRINT( _T("ERROR: Failed to read file %s\n"), strMappedName.c_str() );
					return false;
				}
				fclose( f );
				tstring strBackupContent = Buffer;

				if( strBackupContent != strPreviousContent )
				{
					ERROR_PRINT( _T("ERROR: Content mismatch: backup: %s read: %s\n"), strBackupContent.c_str(), strPreviousContent.c_str() );
					return false;
				}
			}
			else
			{
				_tfopen_s( &f, strMappedName.c_str(), _T("r") );
				if( f )
				{
					fclose( f );
					ERROR_PRINT( _T("ERROR: Excluded file %s is found when it shouldn't\n"), strMappedName.c_str() );
					return false;
				}
			}
		}

		strPreviousContent = strContent;
	}

	return true;
}

bool CTest::StopDriver()
{
	return Utils::ExecuteProcess( "fltmc unload cebackup" );
}

bool CTest::StopCeuser()
{
	return Utils::ExecuteProcess( "taskkill /IM ceuser.exe" );
}

bool CTest::StartDriver()
{
	return Utils::ExecuteProcess( "fltmc load cebackup" );
}

bool CTest::StartCeuser()
{
	if( ! Utils::ExecuteProcess( "ceuser.exe", FALSE ) )
		return false;

	::Sleep( 1000 );
	return true;
}
