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
    std::map<int, int> mapLastIndexes;
    std::map<int, int> mapDummy;

	srand( (unsigned int)time(NULL) );

    tstring error;
	if( ! _Settings.Init( IniPath, error ) )
		return false;

	INFO_PRINT( _T("CETEST: [Settings] File: %s\n"), IniPath.c_str() );
	INFO_PRINT( _T("CETEST: [Settings] Backup directory: %s\n"), _Settings.Destination.c_str() );

	//Clean up
	if( ! StopDriver() )
		return false;

    CleanUserApp();

	tstring strRestoreDirectory = _T("TestRestore");
	tstring strRestoreDirectoryTo = _T("TestRestoreTo");
	if( Utils::DoesDirectoryExists( strRestoreDirectory ) && ! Utils::RemoveDirectory( strRestoreDirectory ) )
		return false;
	if( Utils::DoesDirectoryExists( strRestoreDirectoryTo ) && ! Utils::RemoveDirectory( strRestoreDirectoryTo ) )
		return false;

    if( ! Utils::CreateDirectory( strRestoreDirectory.c_str() ) )
        goto Cleanup;
    if( ! Utils::CreateDirectory( strRestoreDirectoryTo.c_str() ) )
        goto Cleanup;

    //Start
	if( ! StartDriver() )
		goto Cleanup;

	if( ! StartCeuser( true ) )
		goto Cleanup;

	// Write to IncludeFile. N files, N times, check content
    INFO_PRINT( _T("CETEST: INFO: --- IncludeFile test\n") );
	for( size_t i=0; i<_Settings.IncludedFiles.size(); i++ )
	{
		if( ! WriteAndCheckContent( _Settings.IncludedFiles[i], true, false, mapLastIndexes[i] ) )
			goto Cleanup;
	}

	// Write to IncludeDirectory
    INFO_PRINT( _T("CETEST: INFO: --- IncludeDirectory test\n") );
	for( size_t i=0; i<_Settings.IncludedDirectories.size(); i++ )
	{
		tstring strPath = _Settings.IncludedDirectories[i] + _T("\\") + _T("fileTmp.txt");
		if( ! WriteAndCheckContent( strPath, true, false, mapDummy[i] ) )
			return false;
	}

	// Write to ExcludedFile
    INFO_PRINT( _T("CETEST: INFO: --- ExcludedFile test\n") );
	for( size_t i=0; i<_Settings.ExcludedFiles.size(); i++ )
	{
		if( ! WriteAndCheckContent( _Settings.ExcludedFiles[i], false, false, mapDummy[i] ) )
			goto Cleanup;
	}

	// Write to ExcludedDirectory
    INFO_PRINT( _T("CETEST: INFO: --- ExcludedDirectory test\n") );
	for( size_t i=0; i<_Settings.ExcludedDirectories.size(); i++ )
	{
		tstring strPath = _Settings.ExcludedDirectories[i] + _T("\\") + _T("fileTmp.txt");
		if( ! WriteAndCheckContent( strPath, false, false, mapDummy[i] ) )
			goto Cleanup;
	}

	// Write to ExcludedExtension
    INFO_PRINT( _T("CETEST: INFO: --- ExcludedExtension test\n") );
	for( size_t i=0; i<_Settings.ExcludedDirectories.size(); i++ )
	{
		tstring strPath = _Settings.ExcludedDirectories[i] + _T("\\") + _T("fileTmp.bat");
		if( ! WriteAndCheckContent( strPath, false, false, mapDummy[i] ) )
			goto Cleanup;
	}

    INFO_PRINT( _T("CETEST: INFO: --- Restore test\n") );
    for( size_t i=0; i<_Settings.IncludedFiles.size(); i++ )
	{
    	//Restore to
		if( ! Restore( _Settings.Destination, _Settings.IncludedFiles[i], strRestoreDirectoryTo, 1, true ) )
		    goto Cleanup;

		//Restore
		if( ! Restore( _Settings.Destination, _Settings.IncludedFiles[i], strRestoreDirectory ) )
		    goto Cleanup;
	}

	// Delete IncludeFile. N files, check content
    INFO_PRINT( _T("CETEST: INFO: --- Delete IncludeFile test\n") );
	for( size_t i=0; i<_Settings.IncludedFiles.size(); i++ )
	{
		if( ! DeleteAndCheckContent( _Settings.Destination, _Settings.IncludedFiles[i], mapLastIndexes[i] + 1 ) )
			goto Cleanup;

		//Restore
		if( ! Restore( _Settings.Destination, _Settings.IncludedFiles[i], strRestoreDirectory, mapLastIndexes[i] + 1, false, true ) )
		    goto Cleanup;
    }

    //Check IgnoreSaveForMinutes
    INFO_PRINT( _T("CETEST: INFO: --- IgnoreSave test\n") );
    if( ! CleanUserApp() )
        goto Cleanup;

	if( ! StartCeuser( false ) )
		goto Cleanup;

    //Write to ignore file
	for( size_t i=0; i<_Settings.IncludedDirectories.size(); i++ )
	{
		tstring strPath = _Settings.IncludedDirectories[i] + _T("\\") + _T("fileTmp.txt");
		if( ! WriteAndCheckContent( strPath, false, true, mapDummy[i] ) )
			goto Cleanup;
	}

    //NumberOfCopies
    INFO_PRINT( _T("CETEST: INFO: --- NumberOfCopies test\n") );
    if( ! CleanUserApp() )
        goto Cleanup;

	if( ! StartCeuser( true ) )
		goto Cleanup;

	for( size_t i=0; i<_Settings.IncludedDirectories.size(); i++ )
	{
    	tstring strPath = _Settings.IncludedDirectories[i] + _T("\\") + _T("fileTmp.txt");
    	for( int j=0; j<_Settings.NumberOfCopies * 2; j++ )
	    {
		    std::wstringstream ossContent;
		    ossContent << _T("NumberOfCopies check") << _T(".") << j;

	    	if( ! WriteContent( strPath, ossContent.str() ) )
			    goto Cleanup;
        }

        int Count = 0;
		tstring strMappedName = Utils::MapToDestination( _Settings.Destination, strPath );

        if( ! Utils::GetIndexCount( strMappedName, Count ) )
	    {
		    ERROR_PRINT( _T("CETEST: ERROR: GetIndexCount failed for %s\n"), strMappedName.c_str() );
		    goto Cleanup;
	    }

	    if( Count > _Settings.NumberOfCopies )
	    {
		    ERROR_PRINT( _T("CETEST: ERROR: Backup copies (%d) > INI NumberOfCopies (%d) for %s\n"), Count, _Settings.NumberOfCopies, strPath.c_str() );
		    goto Cleanup;
	    }
    }

    //NumberOfCopies
    INFO_PRINT( _T("CETEST: INFO: --- Same content skip test\n") );
    if( ! CleanUserApp() )
        goto Cleanup;

	if( ! StartCeuser( true ) )
		goto Cleanup;

    //Same content skip check
	for( size_t i=0; i<_Settings.IncludedDirectories.size(); i++ )
	{
    	tstring strPath = _Settings.IncludedDirectories[i] + _T("\\") + _T("fileTmp.txt");
    	for( int j=0; j<_Settings.NumberOfCopies / 2; j++ )
	    {
	    	if( ! WriteContent( strPath, _T("NumberOfCopies check") ) )
			    goto Cleanup;
        }

        int Count = 0;
		tstring strMappedName = Utils::MapToDestination( _Settings.Destination, strPath );

        if( ! Utils::GetIndexCount( strMappedName, Count ) )
	    {
		    ERROR_PRINT( _T("CETEST: ERROR: GetIndexCount failed for %s\n"), strMappedName.c_str() );
		    goto Cleanup;
	    }

	    if( Count > 1 )
	    {
		    ERROR_PRINT( _T("CETEST: ERROR: Backup copies (%d) > INI NumberOfCopies (%d) for %s\n"), Count, _Settings.NumberOfCopies, strPath.c_str() );
		    goto Cleanup;
	    }
    }

    ret = true;

Cleanup:
	if( ! StopCeuser() )
		return false;

	if( ! StopDriver() )
		return false;

	if( Utils::DoesDirectoryExists( strRestoreDirectory ) && ! Utils::RemoveDirectory( strRestoreDirectory ) )
		return false;
	if( Utils::DoesDirectoryExists( strRestoreDirectoryTo ) && ! Utils::RemoveDirectory( strRestoreDirectoryTo ) )
		return false;

    if( ! ret )
    {
        INFO_PRINT( _T("Failed. Press any key\n") );
        getchar();
    }
    return ret;
}

bool CTest::CreateEmptyFile( const tstring& Path )
{
    return WriteContent( Path, _T("") );
}

bool CTest::WriteAndCheckContent( const tstring& Path, bool Included, bool Ignore, int& LastIndex )
{
	tstring strPreviousContent;
	int nWrites = ( rand() % 10 ) + 2;
    LastIndex = nWrites - 1;
	for( int i=0; i<nWrites; i++ )
	{
		tstring strContent;
		std::wstringstream oss;
		for( int k=1; k <= nWrites * (i + 1); k++ )
		{
			oss << k;
		}

		strContent = oss.str();

        if( ! WriteContent( Path, strContent ) )
            return false;

		if( i > 0 )
		{
			tstring strMappedName = Utils::MapToDestination( _Settings.Destination, Path );
			std::wstringstream ossName;
			ossName << strMappedName << _T(".") << i;
			strMappedName = ossName.str();

			if( Included || Ignore )
			{
				tstring strBackupContent;
				if( ! GetContent( strMappedName, strBackupContent ) )
                {
                    if( ! Ignore )
                    {
                        ERROR_PRINT( _T("CETEST: ERROR: CheckFile: failed for %s\n"), strMappedName.c_str() );
					    return false;
                    }
                }

				if( strBackupContent != strPreviousContent )
				{
                    if( ! Ignore || (Ignore && i < 2) )
                    {
					    ERROR_PRINT( _T("CETEST: ERROR: Backup content mismatch: backup: %s read: %s\n"), strBackupContent.c_str(), strPreviousContent.c_str() );
					    return false;
                    }
				}
			}
			else
			{
                FILE* f = NULL;
				_tfopen_s( &f, strMappedName.c_str(), _T("r") );
				if( f )
				{
					fclose( f );
					ERROR_PRINT( _T("CETEST: ERROR: Excluded file %s is found when it shouldn't\n"), strMappedName.c_str() );
					return false;
				}
			}
		}

		strPreviousContent = strContent;
	}

	return true;
}

bool CTest::Restore( const tstring& Destination, const tstring& Path, const tstring& Dir, int Index, bool To, bool Deleted )
{
	tstring strMapped = Utils::MapToDestination( Destination, Path ) + _T(".1");
    std::wstringstream oss;
    oss << Path;
    if( Deleted )
        oss << _T(".DELETED");
    oss << _T(".");
    oss << Index;
    tstring strListed = oss.str();
	std::string strPath( strListed.begin(), strListed.end() );
	std::string strDir( Dir.begin(), Dir.end() );
	std::string strCmd;
	if( To )
		strCmd = std::string("restore.exe restore_to ") + strPath + " " + strDir;
	else
		strCmd = ("restore.exe restore ") + strPath;

	tstring strBackupContent;
	if( ! GetContent( strMapped, strBackupContent ) )
    {
        ERROR_PRINT( _T("CETEST: ERROR: Restore: GetContent: failed for %s\n"), Path.c_str() );
		return false;
    }

	if( ! Utils::ExecuteProcess( strCmd.c_str() ) )
		return false;

	tstring strRestoredContent;
	if( To )
	{
		Utils::CPathDetails pd;
		pd.Parse( false, Path );
        tstring strRestoreToPath = Dir + _T("\\") + pd.Name;
		if( ! GetContent( strRestoreToPath, strRestoredContent ) )
        {
            ERROR_PRINT( _T("CETEST: ERROR: Restore TO: GetContent: failed for %s\n"), Path.c_str() );
			return false;
        }
	}
	else
	{
		if( ! GetContent( Path, strRestoredContent ) )
        {
            ERROR_PRINT( _T("CETEST: ERROR: Restore: GetContent: failed for %s\n"), Path.c_str() );
			return false;
        }
	}

	if( strBackupContent != strRestoredContent )
	{
		ERROR_PRINT( _T("CETEST: ERROR: Restored content mismatch: backup: %s restored: %s\n"), strBackupContent.c_str(), strRestoredContent.c_str() );
		return false;
	}

	return true;
}

bool CTest::StopDriver()
{
    INFO_PRINT( _T("CETEST: INFO: Stopping driver\n") );
	return Utils::ExecuteProcess( "fltmc unload cebackup" );
}

bool CTest::StopCeuser()
{
    INFO_PRINT( _T("CETEST: INFO: Stopping ceuser\n") );
	return Utils::ExecuteProcess( "taskkill /F /IM ceuser.exe" );
}

bool CTest::StartDriver()
{
    INFO_PRINT( _T("CETEST: INFO: Starting driver\n") );
	return Utils::ExecuteProcess( "fltmc load cebackup" );
}

bool CTest::StartCeuser( bool cetest )
{
    INFO_PRINT( _T("INFO: Starting ceuser\n") );
    if( cetest )
    {
	    if( ! Utils::ExecuteProcess( "cmd.exe /C ceuser.exe -ini cetest.ini", FALSE ) )
		    return false;
    }
    else
    {
	    if( ! Utils::ExecuteProcess( "cmd.exe /C ceuser.exe -ini cebackup.ini", FALSE ) )
		    return false;
    }


	::Sleep( 1000 );
	return true;
}

bool CTest::WriteContent( const tstring& Path, const tstring& Content )
{
	FILE* f = NULL;
	_tfopen_s( &f, Path.c_str(), _T("wb") );
	if( ! f )
	{
		ERROR_PRINT( _T("CETEST: ERROR: Failed to open file %s for writing\n"), Path.c_str() );
		return false;
	}

    size_t iWritten = fwrite( Content.c_str(), sizeof(TCHAR), Content.size(), f );
	fclose( f );
	if( iWritten != Content.size() )
	{
		ERROR_PRINT( _T("CETEST: ERROR: Failed to write file %s\n"), Path.c_str() );
		return false;
	}

    return true;
}

bool CTest::GetContent( const tstring& Path, tstring& Content )
{
	FILE* f = NULL;
	_tfopen_s( &f, Path.c_str(), _T("r") );
	if( ! f )
	{
		return false;
	}

	TCHAR Buffer[MAX_PATH] = {0};
	size_t iRead = fread( Buffer, sizeof(TCHAR), sizeof(Buffer), f );
	if( iRead <= 0 )
	{
		ERROR_PRINT( _T("CETEST: ERROR: Failed to read file %s\n"), Path.c_str() );
		return false;
	}

	fclose( f );
	Content = Buffer;

	return true;
}

bool CTest::DeleteAndCheckContent( const tstring& Destination, const tstring& Path, int Index )
{
	tstring strPreviousContent, strBackupContent;

	if( ! GetContent( Path, strPreviousContent ) )
    {
        ERROR_PRINT( _T("CETEST: ERROR: Delete: GetContent: failed for %s\n"), Path.c_str() );
		return false;
    }

    static bool deleteMethod = false ;
    deleteMethod = ! deleteMethod;
    if( deleteMethod )
    {
        if( ! ::DeleteFile( Path.c_str() ) )
        {
            ERROR_PRINT( _T("CETEST: ERROR: Delete: DeleteFile: Failed to delete %s\n"), Path.c_str() );
		    return false;
        }
    }
    else
    {
        HANDLE hFile = ::CreateFile( Path.c_str(), DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE | FILE_ATTRIBUTE_NORMAL, NULL );
        if( hFile == INVALID_HANDLE_VALUE )
        {
            ERROR_PRINT( _T("CETEST: ERROR: Delete: CreateFile: Failed to delete %s\n"), Path.c_str() );
		    return false;
        }
        ::CloseHandle( hFile );

        FILE* f = NULL;
		_tfopen_s( &f, Path.c_str(), _T("r") );
		if( f )
		{
			fclose( f );
			ERROR_PRINT( _T("CETEST: ERROR: Delete: CreateFile file %s is found when it shouldn't\n"), Path.c_str() );
			return false;
		}
    }

	std::wstringstream oss;
    oss << Utils::MapToDestination( Destination, Path ) << _T(".DELETED.");
    oss << Index;

	if( ! GetContent( oss.str(), strBackupContent ) )
    {
        ERROR_PRINT( _T("CETEST: ERROR: Restore TO: GetContent: failed for %s\n"), oss.str().c_str() );
		return false;
    }

	if( strBackupContent != strPreviousContent )
	{
		ERROR_PRINT( _T("CETEST: ERROR: Delete: Backup content mismatch: backup: %s read: %s\n"), strBackupContent.c_str(), strPreviousContent.c_str() );
		return false;
	}

    return true;
}

bool CTest::CleanUserApp()
{
	if( ! StopCeuser() )
		return false;

	if( Utils::DoesDirectoryExists( _Settings.Destination ) && ! Utils::RemoveDirectory( _Settings.Destination ) )
		return false;

    // Create files in IncludeDirectory
	for( size_t i=0; i<_Settings.IncludedFiles.size(); i++ )
	{
		if( ! CreateEmptyFile( _Settings.IncludedFiles[i] ) )
			return false;
	}

	// Write to IncludeDirectory
	for( size_t i=0; i<_Settings.IncludedDirectories.size(); i++ )
	{
		tstring strPath = _Settings.IncludedDirectories[i] + _T("\\") + _T("fileTmp.txt");
		if( ! CreateEmptyFile( strPath ) )
			return false;
		strPath = _Settings.IncludedDirectories[i] + _T("\\") + _T("fileTmp.bat");
		if( ! CreateEmptyFile( strPath ) )
			return false;
	}

    return true;
}
