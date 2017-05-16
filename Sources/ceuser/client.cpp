#include "usercommon.h"
#include "client.h"
#include "drvcommon.h"
#include "ceuser.h"
#include "Settings.h"
#include "Utils.h"

#define METHOD_CLOSE_HANDLE_IN_DRIVER 0

//  Default and Maximum number of threads.
#define BACKUP_REQUEST_COUNT            5
#define BACKUP_DEFAULT_THREAD_COUNT     2
#define BACKUP_MAX_THREAD_COUNT         64

CBackupClient::CBackupClient()
    : _completion(NULL)
    , _port(NULL)
{
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

bool CBackupClient::IsIncluded( const tstring& Path )
{
	Utils::CPathDetails pd;

    if( ! pd.Parse( false, Path ) )
    {
        ERROR_PRINT( _T("CEUSER: ERROR: Failed to parse %s\n"), Path.c_str() );
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
    tstring strMappedPathNoIndex = Utils::MapToDestination( _Settings.Destination, SrcPath );

    int iIndex = 0;
    if( ! Utils::GetLastIndex( strMappedPathNoIndex, iIndex ) )
        return false;

    iIndex ++;
    std::wstringstream oss;
    oss << strMappedPathNoIndex << _T(".") << iIndex;
    tstring strDestPath = oss.str();

    Utils::CPathDetails pd;
    if( ! pd.Parse( false, strDestPath ) )
    {
        ERROR_PRINT( _T("CEUSER: ERROR: Failed to parse %s\n"), strDestPath.c_str() );
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
            ERROR_PRINT( _T("CEUSER: ERROR: ReadFile %s failed, hFile=%p status=%s\n"), SrcPath.c_str(), hSrcFile, Utils::GetLastErrorString().c_str() );
            OutputDebugString( (tstring( _T("CEUSER: ERROR: ReadFile failed: ") ) + SrcPath).c_str() );
            ret = false;
            goto Cleanup;
        }

        if( dwRead > 0 )
        {
            if( hDestFile == NULL )
            {
                //Lets create directory only if file is not empty
                if( ! Utils::CreateDirectory( pd.Directory ) )
                {
                    ret = false;
                    goto Cleanup;
                }

                hDestFile = ::CreateFile( strDestPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
                if( hDestFile == INVALID_HANDLE_VALUE )
                {
                    ERROR_PRINT( _T("CEUSER: ERROR: CreateFile failed to open destination file %s, error=0x%X\n"), strDestPath.c_str(), ::GetLastError() );
                    OutputDebugString( (tstring( _T("CEUSER: ERROR: CreateFile failed to open destination file: ") ) + strDestPath).c_str() );
                    ret = false;
                    goto Cleanup;
                }
            }

            DWORD dwWritten = 0;
            BOOL bWrite = ::WriteFile( hDestFile, Buffer, dwRead, &dwWritten, NULL );
            if( ! bWrite )
            {
                ERROR_PRINT( _T("CEUSER: ERROR: WriteFile %s failed, error=0x%X\n"), SrcPath.c_str(), ::GetLastError() );
                OutputDebugString( (tstring( _T("CEUSER: ERROR: WriteFile failed: ") ) + SrcPath).c_str() );
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
        ERROR_PRINT( _T("CEUSER: ERROR: SetFileTime( %s, 0x%X ) failed, error=%s\n"), strDestPath.c_str(), SrcAttribute, Utils::GetLastErrorString().c_str() );
        OutputDebugString( (tstring( _T("CEUSER: ERROR: SetFileTime failed: ") ) + strDestPath).c_str() );
        ret = false;
        goto Cleanup;
    }

    //Copy attributes
    if( hDestFile && ! ::SetFileAttributes( strDestPath.c_str(), SrcAttribute ) )
    {
        ERROR_PRINT( _T("CEUSER: ERROR: SetFileAttributes( %s, 0x%X ) failed, error=%s\n"), strDestPath.c_str(), SrcAttribute, Utils::GetLastErrorString().c_str() );
        OutputDebugString( (tstring( _T("CEUSER: ERROR: SetFileAttributes failed: ") ) + strDestPath).c_str() );
        ret = false;
        goto Cleanup;
    }

    ret = true;
    INFO_PRINT( _T("CEUSER: INFO Backup done OK: %s, index=%d\n"), SrcPath.c_str(), iIndex );
    OutputDebugString( (tstring( _T("CEUSER: INFO Backup done OK: ") ) + SrcPath).c_str() );

Cleanup:
    if( hDestFile != NULL && hDestFile != INVALID_HANDLE_VALUE )
    {
        if( ! ::CloseHandle( hDestFile ) )
            ERROR_PRINT( _T("CEUSER: ERROR: CloseHandle hDestFile failed, hDestFile=%p"), hDestFile );
    }
    else if( hDestFile == NULL )
    {
        INFO_PRINT( _T("CEUSER: DEBUG: Skipping empty file %s\n"), SrcPath.c_str() );
    }

    LeaveCriticalSection( &_guardDestFile );

    return ret;
}

bool CBackupClient::BackupFile ( HANDLE hFile, const tstring& Path, DWORD SrcAttribute, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime )
{
    if( SrcAttribute & FILE_ATTRIBUTE_DIRECTORY )
    {
        ERROR_PRINT( _T("CEUSER: WARNING: Directory attribute detected for %s. Skipping\n"), Path.c_str() );
        return true;
    }

    tstring strLower = Utils::ToLower( Path );
    if( strLower.substr( 0, _Settings.Destination.size() ) == _Settings.Destination )
    {
        INFO_PRINT( _T("CEUSER: INFO: Skipping write to Destination=%s"), Path.c_str() );
        return true;
    }

    if( IsIncluded( strLower ) )
    {
        return DoBackup( hFile, Path, SrcAttribute, CreationTime, LastAccessTime, LastWriteTime );
    }
    else
    {
        DEBUG_PRINT( _T("CEUSER: DEBUG: Skipping NOT Included file=%s"), Path.c_str() );
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
        DEBUG_PRINT( _T("CEUSER: DEBUG: File write notification: Path=%s HANDLE=0x%p\n"), notification->Path, notification->hFile );

        FILETIME CreationTime, LastAccessTime, LastWriteTime;
        CreationTime.dwLowDateTime = notification->CreationTime.LowPart;
        CreationTime.dwHighDateTime = notification->CreationTime.HighPart;
        LastAccessTime.dwLowDateTime = notification->LastAccessTime.LowPart;
        LastAccessTime.dwHighDateTime = notification->LastAccessTime.HighPart;
        LastWriteTime.dwLowDateTime = notification->LastWriteTime.LowPart;
        LastWriteTime.dwHighDateTime = notification->LastWriteTime.HighPart;

        result = BackupFile( notification->hFile, notification->Path, notification->FileAttributes, CreationTime, LastAccessTime, LastWriteTime );

#if METHOD_CLOSE_HANDLE_IN_DRIVER

#else
		::CloseHandle( notification->hFile );
#endif

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
            ERROR_PRINT( _T("CEUSER: ERROR: Error replying message. Error = 0x%X\n"), hr );
            OutputDebugString( _T("CEUSER: ERROR: Error replying message") );
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
            INFO_PRINT( _T("CEUSER: INFO: Port is disconnected, probably due to scanner filter unloading\n") );
            OutputDebugString( _T("CEUSER: INFO: Port is disconnected, probably due to scanner filter unloading") );
        }
        else
        {
            ERROR_PRINT( _T("CEUSER: ERROR: Port Get: Unknown error occurred. Error = 0x%X\n"), hr );
            OutputDebugString( _T("CEUSER: ERROR: Port Get: Unknown error occurred") );
        }
    }

    if( message )
        free( message );
}

bool CBackupClient::Run( const tstring& IniPath, tstring& error, bool async )
{
    bool ret = false;
	DWORD requestCount = BACKUP_REQUEST_COUNT;
    DWORD threadCount = BACKUP_DEFAULT_THREAD_COUNT;
    HANDLE threads[BACKUP_MAX_THREAD_COUNT];

    if( IsRunning() )
    {
        error = _T("One instance of application is already running");
        ERROR_PRINT( (tstring( _T("CEUSER: ERROR: ") ) + error + _T("\n") ).c_str() );
        return false;
    }

	if( ! _Settings.Init( IniPath, error ) )
        return false;

	INFO_PRINT( _T("CEUSER: [Settings] File: %s\n"), IniPath.c_str() );
	INFO_PRINT( _T("CEUSER: [Settings] Backup directory: %s\n"), _Settings.Destination.c_str() );

    if( ! Utils::CreateDirectory( _Settings.Destination.c_str() ) )
    {
        error = _T("Failed to create Destination directory: ") + _Settings.Destination;
        ERROR_PRINT( (tstring( _T("CEUSER: ERROR: ") ) + error + _T("\n") ).c_str() );
        return false;
    }

	InitializeCriticalSection( &_guardDestFile );

    //  Open a communication channel to the filter
    INFO_PRINT( _T("CEUSER: INFO: Connecting to the filter ...\n") );

    HRESULT hr = ::FilterConnectCommunicationPort( BACKUP_PORT_NAME, 0, NULL, 0, NULL, &_port );
    if( IS_ERROR( hr ) )
    {
        error = _T("Failed connect to filter port");
        ERROR_PRINT( (tstring( _T("CEUSER: ERROR: ") ) + error + _T(": 0x%08x\n") ).c_str(), hr );
        return false;
    }

    //  Create a completion port to associate with this handle.
    _completion = ::CreateIoCompletionPort( _port, NULL, 0, threadCount );
    if( ! _completion )
    {
        error = _T("Failed to creat completion port");
        ERROR_PRINT( (tstring( _T("CEUSER: ERROR: ") ) + error + _T(": %d\n") ).c_str(), ::GetLastError() );
        ::CloseHandle( _port );
        _port = NULL;
        return false;
    }

    INFO_PRINT( _T("CEUSER: INFO: Connected: Port = 0x%p Completion = 0x%p\n"), _port, _completion );

    _Context.Port = _port;
    _Context.Completion = _completion;
    _Context.This = this;

    // Create specified number of threads.
    DWORD i = 0;
    for( i = 0; i < threadCount; i++ )
    {
        DWORD threadId;
        threads[i] = ::CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) _BackupWorker, &_Context, 0, &threadId );
        if( ! threads[i] ) 
        {
            //  Couldn't create thread.
            hr = ::GetLastError();
            error = _T("Couldn't create thread");
            ERROR_PRINT( (tstring( _T("CEUSER: ERROR: ") ) + error + _T(": 0x%08x\n") ).c_str(), hr );
            goto Cleanup;
        }

        for( DWORD j = 0; j < requestCount; j++ )
        {
            //  Allocate the message.
            PBACKUP_MESSAGE msg = (PBACKUP_MESSAGE) malloc( sizeof( BACKUP_MESSAGE ) );
            if( ! msg )
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                error = _T("Couldn't malloc BACKUP_MESSAGE");
                ERROR_PRINT( (tstring( _T("CEUSER: ERROR: ") ) + error + _T(": 0x%08x\n") ).c_str(), hr );
                goto Cleanup;
            }

            memset( msg, 0, sizeof( BACKUP_MESSAGE ) );

            // Request messages from the filter driver.
            hr = ::FilterGetMessage( _port, &msg->MessageHeader, FIELD_OFFSET( BACKUP_MESSAGE, Ovlp ), &msg->Ovlp );

            if( hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING ) )
            {
                error = _T("FilterGetMessage failed");
                free( msg );
                goto Cleanup;
            }
        }
    }

    ret = true;
    OutputDebugString( _T("CEUSER: INFO: Started") );

    if( async )
        return ret;

    ::WaitForMultipleObjectsEx( i, threads, TRUE, INFINITE, FALSE );

Cleanup:
    ::CloseHandle( _port );
    ::CloseHandle( _completion );

    return ret;
}

bool CBackupClient::Stop()
{
    if( ! _completion )
        return false;

    ::CloseHandle( _port );
    ::CloseHandle( _completion );
    _port = NULL;
    _completion = NULL;
    return true;
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
