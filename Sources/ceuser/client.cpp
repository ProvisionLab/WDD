#include "usercommon.h"
#include "client.h"
#include "drvcommon.h"
#include "ceuser.h"
#include "Settings.h"
#include "Utils.h"

//  Default and Maximum number of threads.
#define BACKUP_REQUEST_COUNT            5
#define BACKUP_DEFAULT_THREAD_COUNT     2
#define BACKUP_MAX_THREAD_COUNT         64


bool CBackupClient::IsIncluded( const tstring& Path )
{
	Utils::CPathDetails pd;

    if( ! pd.Parse( false, Path ) )
    {
        ERROR_PRINT( _T("ERROR: Failed to parse %s\n"), Path.c_str() );
        return false;
    }

    // Logic is next:
    // 1. If file is explicitly set in [ExcludeFile] - ignore it right away
    // 2. If file's directory is explicitly set in [ExcludeDirectory] - ignore it right away
    // 3. If If file is explicitly set in [IncludeFile] - backup it
    // 4. If If file's directory is explicitly set in [IncludeFile] - backup it

    std::vector<tstring>::iterator it = std::find( _Settings.ExcludedFiles.begin(), _Settings.ExcludedFiles.end(), Path ) ;
    if( it != _Settings.ExcludedFiles.end() )
        return false;

    for( it = _Settings.ExcludedDirectories.begin(); it != _Settings.ExcludedDirectories.end(); it++ )
    {
        if( Path.substr( 0, (*it).size() ) == (*it)  )
            return false;
    }

    it = std::find( _Settings.IncludedFiles.begin(), _Settings.IncludedFiles.end(), Path ) ;
    if( it != _Settings.IncludedFiles.end() )
        return true;

    for( it = _Settings.IncludedDirectories.begin(); it != _Settings.IncludedDirectories.end(); it++ )
    {
        if( Path.substr( 0, (*it).size() ) == (*it)  )
            return true;
    }

    return false;
}

bool CBackupClient::DoBackup( HANDLE hSrcFile, const tstring& SrcPath, DWORD SrcAttribute, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime )
{
    bool ret = false;
    tstring strMappedPath = Utils::MapToDestination( SrcPath );

    int iIndex = 0;
    if( ! Utils::GetLastIndex( _Settings.Destination, strMappedPath, iIndex ) )
        return false;

    iIndex ++;
    std::wstringstream oss;
    oss << _Settings.Destination << _T("\\") << strMappedPath << _T(".") << iIndex;
    tstring strDestPath = oss.str();

    Utils::CPathDetails pd;
    if( ! pd.Parse( true, strDestPath ) )
    {
        ERROR_PRINT( _T("ERROR: Failed to parse %s\n"), strDestPath.c_str() );
        return false;
    }

    EnterCriticalSection( &_guardDestFile );

    //::CopyFile( Path.c_str(), strDestPath.c_str(), TRUE );
    HANDLE hDestFile = NULL;

    char Buffer[4*1024];
    DWORD BufferLength = sizeof(Buffer);
    BOOL bRead = TRUE;
    while( bRead )
    {
        DWORD dwRead = 0;

        bRead = ::ReadFile( hSrcFile, Buffer, BufferLength, &dwRead, NULL ); // ERROR_INVALID_PARAMETER

        if( ! bRead )
        {
            //::CreateFile( L"C:\\XXXXX.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            ERROR_PRINT( _T("ERROR: ReadFile %s failed, hFile=%p status=%s\n"), SrcPath.c_str(), hSrcFile, Utils::GetLastErrorString().c_str() );
            ret = false;
            goto Cleanup;
        }

        if( dwRead > 0 )
        {
            if( hDestFile == NULL )
            {
                //Lets create directory only if file is not empty
                if( ! Utils::CreateDirectories( pd.Directory ) )
                {
                    ret = false;
                    goto Cleanup;
                }

                hDestFile = ::CreateFile( strDestPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
                if( hDestFile == INVALID_HANDLE_VALUE )
                {
                    ERROR_PRINT( _T("ERROR: Backup: Failed to open destination file %s, error=0x%X\n"), strDestPath.c_str(), ::GetLastError() );
                    ret = false;
                    goto Cleanup;
                }
            }

            DWORD dwWritten = 0;
            BOOL bWrite = ::WriteFile( hDestFile, Buffer, dwRead, &dwWritten, NULL );
            if( ! bWrite )
            {
                ERROR_PRINT( _T("ERROR: WriteFile %s failed, error=0x%X\n"), SrcPath.c_str(), ::GetLastError() );
                ret = false;
                goto Cleanup;
            }
        }
		else
		{
			bRead = FALSE;
		}
    }

    if( hDestFile && ! ::SetFileTime( hDestFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        ERROR_PRINT( _T("ERROR: SetFileTime( %s, 0x%X ) failed, error=%s\n"), strDestPath.c_str(), SrcAttribute, Utils::GetLastErrorString().c_str() );
        ret = false;
        goto Cleanup;
    }

    //Copy attributes
    if( hDestFile && ! ::SetFileAttributes( strDestPath.c_str(), SrcAttribute ) )
    {
        ERROR_PRINT( _T("ERROR: SetFileAttributes( %s, 0x%X ) failed, error=%s\n"), strDestPath.c_str(), SrcAttribute, Utils::GetLastErrorString().c_str() );
        ret = false;
        goto Cleanup;
    }

    ret = true;
    INFO_PRINT( _T("INFO Backup done OK: %s, index=%d\n"), SrcPath.c_str(), iIndex );

Cleanup:
    if( hDestFile != NULL && hDestFile != INVALID_HANDLE_VALUE )
    {
        if( ! ::CloseHandle( hDestFile ) )
            ERROR_PRINT( _T("ERROR: CloseHandle hDestFile failed, hDestFile=%p"), hDestFile );
    }
    else if( hDestFile == NULL )
    {
        INFO_PRINT( _T("DEBUG: Skipping empty file %s\n"), SrcPath.c_str() );
    }

    LeaveCriticalSection( &_guardDestFile );

    return ret;
}

bool CBackupClient::BackupFile ( HANDLE hFile, const tstring& Path, DWORD SrcAttribute, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime )
{
    if( SrcAttribute & FILE_ATTRIBUTE_DIRECTORY )
    {
        ERROR_PRINT( _T("ERROR: Directory attribute detected for %s\n"), Path.c_str() );
        return true;
    }

    tstring strLower = Utils::ToLower( Path );
    if( strLower.substr( 0, _Settings.Destination.size() ) == _Settings.Destination )
    {
        DEBUG_PRINT( _T("DEBUG: Skipping write to Destination=%s"), Path.c_str() );
        return true;
    }

    if( IsIncluded( strLower ) )
    {
        return DoBackup( hFile, Path, SrcAttribute, CreationTime, LastAccessTime, LastWriteTime );
    }
    else
    {
        DEBUG_PRINT( _T("DEBUG: Skipping NOT Included file=%s"), Path.c_str() );
        return true;
    }

    return true;
}

DWORD CBackupClient::_BackupWorker( _In_ PBACKUP_THREAD_CONTEXT pContext )
{
    pContext->This->BackupWorker( pContext->Completion, pContext->Port );
    return 0;
}

void CBackupClient::BackupWorker( HANDLE Completion, HANDLE Port )
{
    PBACKUP_NOTIFICATION notification = NULL;
    BACKUP_REPLY_MESSAGE replyMessage;
    PBACKUP_MESSAGE message = NULL;
    LPOVERLAPPED pOvlp = NULL;
    HRESULT hr;

    while( TRUE )
    {
        //  Poll for messages from the filter component to scan.
        ULONG_PTR key;
        DWORD outSize = 0;
        BOOL result = ::GetQueuedCompletionStatus( Completion, &outSize, &key, &pOvlp, INFINITE );

        //  Obtain the message: note that the message we sent down via FltGetMessage() may NOT be
        //  the one dequeued off the completion queue: this is solely because there are multiple
        //  threads per single port handle. Any of the FilterGetMessage() issued messages can be
        //  completed in random order - and we will just dequeue a random one.
        message = CONTAINING_RECORD( pOvlp, BACKUP_MESSAGE, Ovlp );

        if( ! result )
        {
            //  An error occurred.
            hr = HRESULT_FROM_WIN32( ::GetLastError() );
            break;
        }

        notification = &message->Notification;
        DEBUG_PRINT( _T("DEBUG: File write notification: Path=%s HANDLE=0x%p\n"), notification->Path, notification->hFile );

        FILETIME CreationTime, LastAccessTime, LastWriteTime;
        CreationTime.dwLowDateTime = notification->CreationTime.LowPart;
        CreationTime.dwHighDateTime = notification->CreationTime.HighPart;
        LastAccessTime.dwLowDateTime = notification->LastAccessTime.LowPart;
        LastAccessTime.dwHighDateTime = notification->LastAccessTime.HighPart;
        LastWriteTime.dwLowDateTime = notification->LastWriteTime.LowPart;
        LastWriteTime.dwHighDateTime = notification->LastWriteTime.HighPart;

        result = BackupFile( notification->hFile, notification->Path, notification->FileAttributes, CreationTime, LastAccessTime, LastWriteTime );

        replyMessage.ReplyHeader.Status = 0;
        replyMessage.ReplyHeader.MessageId = message->MessageHeader.MessageId;

        replyMessage.Reply.OkToOpen = TRUE;

        hr = ::FilterReplyMessage( Port, (PFILTER_REPLY_HEADER) &replyMessage, sizeof( replyMessage ) );
        if( SUCCEEDED( hr ) )
        {
            //DEBUG_PRINT( _T("DEBUG: Replied back to driver\n") );
        }
        else
        {
            ERROR_PRINT( _T("ERROR: Error replying message. Error = 0x%X\n"), hr );
            break;
        }

        memset( &message->Ovlp, 0, sizeof( OVERLAPPED ) );

        hr = ::FilterGetMessage( Port, &message->MessageHeader, FIELD_OFFSET( BACKUP_MESSAGE, Ovlp ), &message->Ovlp );
        if( hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING ) )
        {
            break;
        }
    }

    if( ! SUCCEEDED( hr ) )
    {
        if( hr == HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE ) )
        {
            // CeedoBackup port disconnected.
            INFO_PRINT( _T("INFO: Port is disconnected, probably due to scanner filter unloading.\n") );
        }
        else
        {
            ERROR_PRINT( _T("ERROR: Unknown error occurred. Error = 0x%X\n"), hr );
        }
    }

    free( message );
}

bool CBackupClient::Run( const tstring& IniPath )
{
	DWORD requestCount = BACKUP_REQUEST_COUNT;
    DWORD threadCount = BACKUP_DEFAULT_THREAD_COUNT;
    HANDLE threads[BACKUP_MAX_THREAD_COUNT];
    BACKUP_THREAD_CONTEXT context;
    HANDLE port, completion;

    if( IsRunning() )
    {
        ERROR_PRINT( _T("One instance of application is already running") );
        return false;
    }

	if( ! _Settings.Init( IniPath ) )
        return false;

    InitializeCriticalSection( &_guardDestFile );

    //  Open a communication channel to the filter
    INFO_PRINT( _T("Connecting to the filter ...\n") );

    HRESULT hr = ::FilterConnectCommunicationPort( BACKUP_PORT_NAME, 0, NULL, 0, NULL, &port );
    if( IS_ERROR( hr ) )
    {
        ERROR_PRINT( _T("ERROR: Failed connect to filter port: 0x%08x\n"), hr );
        return false;
    }

    //  Create a completion port to associate with this handle.
    completion = ::CreateIoCompletionPort( port, NULL, 0, threadCount );
    if( ! completion )
    {
        ERROR_PRINT( _T("ERROR: Creating completion port: %d\n"), GetLastError() );
        CloseHandle( port );
        return false;
    }

    DEBUG_PRINT( _T("DEBUG: Connected: Port = 0x%p Completion = 0x%p\n"), port, completion );

    context.Port = port;
    context.Completion = completion;
    context.This = this;

    // Create specified number of threads.
    DWORD i = 0;
    for( i = 0; i < threadCount; i++ )
    {
        DWORD threadId;
        threads[i] = ::CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) _BackupWorker, &context, 0, &threadId );
        if( ! threads[i] ) 
        {
            //  Couldn't create thread.
            hr = ::GetLastError();
            ERROR_PRINT( _T("ERROR: Couldn't create thread: %d\n"), hr );
            goto Cleanup;
        }

        for( DWORD j = 0; j < requestCount; j++ )
        {
            //  Allocate the message.
            PBACKUP_MESSAGE msg = (PBACKUP_MESSAGE) malloc( sizeof( BACKUP_MESSAGE ) );
            if( ! msg )
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            memset( msg, 0, sizeof( BACKUP_MESSAGE ) );

            // Request messages from the filter driver.
            hr = ::FilterGetMessage( port, &msg->MessageHeader, FIELD_OFFSET( BACKUP_MESSAGE, Ovlp ), &msg->Ovlp );

            if( hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING ) )
            {
                free( msg );
                goto Cleanup;
            }
        }
    }

    ::WaitForMultipleObjectsEx( i, threads, TRUE, INFINITE, FALSE );

Cleanup:

    ::CloseHandle( port );
    ::CloseHandle( completion );

    return true;
}

bool CBackupClient::IsRunning()
{
    bool ret = false;
    HANDLE hMutex = ::CreateMutex( NULL, FALSE, L"____CE_USER_APPLICATION____" );
    ret = ::GetLastError() == ERROR_ALREADY_EXISTS;
    if( hMutex )
        ::ReleaseMutex( hMutex );;

    return ret;
}

/*TMP
    tstring name = _T("C:\\Max\\Test\\file1.txt");
    HANDLE hFile = ::CreateFile( name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    DWORD dwAttributes = ::GetFileAttributesEx( name.c_str() );
    FILETIME CreationTime, LastAccessTime, LastWriteTime, ChangeTime;
    ::GetFileTime( hFile, &CreationTime, &LastAccessTime, &LastWriteTime );
    BackupFile( hFile, name, dwAttributes, CreationTime, LastAccessTime, LastWriteTime );
    ::CloseHandle( hFile );
*/
