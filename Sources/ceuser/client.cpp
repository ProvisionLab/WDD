#include "usercommon.h"
#include "client.h"
#include "drvcommon.h"
#include "ceuser.h"
#include "Settings.h"
#include "Utils.h"

#define METHOD_CLOSE_HANDLE_IN_DRIVER 0
//#define TRACE_DEBUG_ TRACE_DEBUG
#define TRACE_DEBUG_ 
#define UNIQUE_CEUSER_MUTEX L"____CE_USER_APPLICATION____"

CBackupCopy::CBackupCopy()
    : Deleted(false)
{
}

CBackupFile::CBackupFile()
    : LastIndex(0)
    , LastBackupTime(0)
{
}

CBackupClient::CBackupClient()
    : _hCompletion(NULL)
    , _hPort(NULL)
    , _bStarted(false)
    , _hLockMutex(NULL)
    , _BackupEvent(NULL)
    , _CleanupEvent(NULL)
{
    InitializeCriticalSection( &_guardMap );
    InitializeCriticalSection( &_guardSettings );
    InitializeCriticalSection( &_guardCallback );
}

CBackupClient::~CBackupClient()
{
    Stop();

    if( _hLockMutex )
    {
        ::CloseHandle( _hLockMutex );
        _hLockMutex = NULL;
    }
}

bool CBackupClient::LockAccess()
{
    if( _hLockMutex )
        return true;

    _hLockMutex = ::CreateMutex( NULL, FALSE, UNIQUE_CEUSER_MUTEX );
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

bool CBackupClient::IsAlreadyRunning()
{
    HANDLE hMutex = ::OpenMutex( NULL, FALSE, UNIQUE_CEUSER_MUTEX );
    bool ret = ::GetLastError() == ERROR_ALREADY_EXISTS;
    return ret;
}

bool CBackupClient::IsStarted()
{
    return _bStarted;
}

bool CBackupClient::IsDriverStarted()
{
    bool ret = false;
    SC_HANDLE schSCManager =  NULL, schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};
    
    schSCManager = ::OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
    if( schSCManager == NULL )
        goto Cleanup;

    schService = ::OpenService( schSCManager, L"cebackup", SERVICE_QUERY_STATUS );
    if( schService == NULL )
        goto Cleanup;

    if( ! ::QueryServiceStatus( schService, &ssSvcStatus ) )
        goto Cleanup;

    ret = ssSvcStatus.dwCurrentState == SERVICE_RUNNING;

Cleanup:
    if( schService )
        ::CloseServiceHandle( schService );
    if( schSCManager )
        ::CloseServiceHandle( schSCManager );

    return ret;
}

bool CBackupClient::Start( const tstring& IniPath, tstring& error, bool async )
{
    TRACE_INFO( _T("CEUSER: Start(): Path: %s"), IniPath.c_str() );
    bool ret = false;
	DWORD requestCount = BACKUP_REQUEST_COUNT;
    DWORD threadCount = BACKUP_DEFAULT_THREAD_COUNT;

    if( ! LockAccess() )
    {
        error = _T("One instance of ceuser driver client is already running");
        TRACE_ERROR( (tstring( _T("CEUSER: ") ) + error).c_str() );
        return false;
    }

    if( ! IsDriverStarted() )
    {
        error = _T("Driver is not running");
        TRACE_ERROR( (tstring( _T("CEUSER: ") ) + error).c_str() );
        return false;
    }

    if( ! ReloadConfig( IniPath, error ) )
        return false;

    tstring strDestination = GetSettings().Destination;
	TRACE_INFO( _T("CEUSER: [Settings] File: %s"), IniPath.c_str() );
	TRACE_INFO( _T("CEUSER: [Settings] Backup directory: %s"), strDestination.c_str() );

    //  Open a communication channel to the filter
    TRACE_INFO( _T("CEUSER: Connecting to the filter ...") );

    HRESULT hr = ::FilterConnectCommunicationPort( BACKUP_PORT_NAME, 0, NULL, 0, NULL, &_hPort );
    if( IS_ERROR( hr ) )
    {
        LPTSTR errorText = NULL;
        FormatMessage(
           // use system message tables to retrieve error text
           FORMAT_MESSAGE_FROM_SYSTEM
           // allocate buffer on local heap for error text
           |FORMAT_MESSAGE_ALLOCATE_BUFFER
           // Important! will fail otherwise, since we're not 
           // (and CANNOT) pass insertion parameters
           |FORMAT_MESSAGE_IGNORE_INSERTS,  
           NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
           hr,
           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
           (LPTSTR)&errorText,  // output 
           0, // minimum size for output buffer
           NULL);   // arguments - see note 

        error = _T("Failed connect to filter port: ");
        if( NULL != errorText )
            error += errorText;
        TRACE_ERROR( (tstring( _T("CEUSER: ") ) + error + _T(" code: 0x%08x") ).c_str(), hr );

        if( NULL != errorText )
           LocalFree( errorText );
        return false;
    }

    //  Create a completion port to associate with this handle.
    _hCompletion = ::CreateIoCompletionPort( _hPort, NULL, 0, threadCount );
    if( ! _hCompletion )
    {
        error = _T("Failed to creat completion port");
        TRACE_ERROR( (tstring( _T("CEUSER: ") ) + error + _T(": %d") ).c_str(), ::GetLastError() );
        ::CloseHandle( _hPort );
        _hPort = NULL;
        return false;
    }

    TRACE_INFO( _T("CEUSER: Connected: Port = 0x%p Completion = 0x%p"), _hPort, _hCompletion );

    if( ! SendDestination( _Settings.Destination ) )
        return false;

    if( ! Utils::CreateDirectory( strDestination.c_str() ) )
    {
        error = _T("Failed to create Destination directory: ") + strDestination + _T(", error=") + Utils::GetLastErrorString();
        TRACE_ERROR( (tstring( _T("CEUSER: ") ) + error).c_str() );
        return false;
    }

    if( ! ScanRepositoryData() )
        return false;

    _Context.Port = _hPort;
    _Context.Completion = _hCompletion;
    _Context.This = this;

    // Create specified number of threads.
    DWORD i = 0;
    for( i = 0; i < threadCount; i++ )
    {
        DWORD threadId;
        _Threads[i] = ::CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) _BackupWorker, &_Context, 0, &threadId );
        if( ! _Threads[i] ) 
        {
            //  Couldn't create thread.
            hr = ::GetLastError();
            error = _T("Couldn't create thread");
            TRACE_ERROR( (tstring( _T("CEUSER: ") ) + error + _T(": 0x%08x") ).c_str(), hr );
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
                TRACE_ERROR( (tstring( _T("CEUSER: ") ) + error + _T(": 0x%08x") ).c_str(), hr );
                goto Cleanup;
            }

            memset( msg, 0, sizeof( BACKUP_MESSAGE ) );

            // Request messages from the filter driver.
            hr = ::FilterGetMessage( _hPort, &msg->MessageHeader, FIELD_OFFSET( BACKUP_MESSAGE, Ovlp ), &msg->Ovlp );

            if( hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING ) )
            {
                error = _T("FilterGetMessage failed");
                free( msg );
                goto Cleanup;
            }
        }
    }

    _bStarted = ret = true;
    TRACE_DEBUG_( _T("CEUSER: Started") );

    if( async )
        return ret;

    ::WaitForMultipleObjectsEx( i, _Threads, TRUE, INFINITE, FALSE );

Cleanup:
    ::CloseHandle( _hPort );
    ::CloseHandle( _hCompletion );

    return ret;
}

bool CBackupClient::SendDestination( const tstring& Destination )
{
    if( ! _hPort || ! _hCompletion )
    {
        TRACE_ERROR( _T("CEUSER: SendDestination: Not connected to driver. Skipping") );
        return false;
    }

    BACKUP_COMMAND_MESSAGE commandMessage = {};

    commandMessage.Command = E_BACKUP_COMMAND_SET_DESTINATION;
    _tcscpy_s( commandMessage.Path, CE_MAX_PATH_SIZE, Destination.c_str() );

    ULONG bytesReturned = 0;
    HRESULT hr = FilterSendMessage( _hPort, &commandMessage, sizeof( BACKUP_COMMAND_MESSAGE ), NULL, 0, &bytesReturned );
    if( FAILED(hr) )
    {
        TRACE_ERROR( _T("CEUSER: FilterSendMessage failed: 0x%08x"), hr );
        return false;
    }

    return true;
}

bool CBackupClient::Stop()
{
    if( ! _bStarted )
        return false;

    if( ! _hPort )
        return false;

    TRACE_INFO( _T("CEUSER: Stopping") );
    if( _hPort )
    {
        ::CloseHandle( _hPort );
       _hPort = NULL;
    }

    if( _hCompletion )
    {
        ::CloseHandle( _hCompletion );
        _hCompletion = NULL;
    }

    return true;
}

bool CBackupClient::IsIncluded( const tstring& Path, const CSettings& Settings )
{
	Utils::CPathDetails pd;

    if( ! pd.Parse( false, Path ) )
    {
        TRACE_ERROR( _T("CEUSER: Failed to parse %s"), Path.c_str() );
        return false;
    }

    // Logic is next:
    // 1. If file is explicitly set in [ExcludeFile] - ignore it right away
    // 2. If file's directory is explicitly set in [ExcludeDirectory] - ignore it right away
    // 3. If If file is explicitly set in [IncludeFile] - backup it
    // 4. If If file's directory is explicitly set in [IncludeFile] - backup it

    std::vector<tstring>::const_iterator it = std::find( Settings.ExcludedExtensions.begin(), Settings.ExcludedExtensions.end(), pd.Extension ) ;
    if( it != Settings.ExcludedExtensions.end() )
        return false;

    it = std::find( Settings.ExcludedFiles.begin(), Settings.ExcludedFiles.end(), Path ) ;
    if( it != Settings.ExcludedFiles.end() )
        return false;

    for( it = Settings.ExcludedDirectories.begin(); it != Settings.ExcludedDirectories.end(); it++ )
    {
        if( Path.substr( 0, (*it).size() ) == (*it)  )
            return false;
    }

    it = std::find( Settings.IncludedFiles.begin(), Settings.IncludedFiles.end(), Path ) ;
    if( it != Settings.IncludedFiles.end() )
        return true;

    for( it = Settings.IncludedDirectories.begin(); it != Settings.IncludedDirectories.end(); it++ )
    {
        if( Path.substr( 0, (*it).size() ) == (*it)  )
            return true;
    }

    return false;
}

bool CBackupClient::CleanupFiles( const tstring& SrcPath, const CSettings& Settings )
{
    bool ret = false;
    CAutoLock lock( &_guardMap );

    std::map<tstring, CBackupFile>::iterator it = _mapBackupFiles.find( SrcPath );

    if( it == _mapBackupFiles.end() )
        goto Cleanup;

    CBackupFile& bf = it->second;

    //1. Delete copies > Count
    int delCount = bf.Copies.size() - Settings.NumberOfCopies;
    if( delCount > 0 )
    {
        for( int i=0; i < delCount; i++ )
        {
            TRACE_INFO( _T("CEUSER: CleanupFiles. Too much copies. Deleting: %s Index: %d"), bf.DstPath.c_str(), bf.Copies[i].Index );
            DeleteBackup( SrcPath, bf.DstPath, bf.Copies[i].Index, bf.Copies[i].Deleted );
        }
        
        for( int i=0; i < delCount; i++ )
            bf.Copies.erase( bf.Copies.begin() );
    }

    //2. Delete expired copies > Time
    __int64 int64Now = Utils::NowTime();
    __int64 int64ExpiredTime = Utils::SubstructDays( int64Now, Settings.DeleteAfterDays );
    for( size_t i=0; i < bf.Copies.size(); i++ )
    {
        __int64 int64FileTime = bf.Copies[i].Time;
        if( int64FileTime < int64ExpiredTime )
        {
            TRACE_INFO( _T("CEUSER: CleanupFiles. Too old copy. Deleting: %s Index: %d"), bf.DstPath.c_str(), bf.Copies[i].Index );
            DeleteBackup( SrcPath, bf.DstPath, bf.Copies[i].Index, bf.Copies[i].Deleted );
            bf.Copies.erase( bf.Copies.begin() + i );
            i --;
        }
    }

    ret = true;

Cleanup:
    return ret;
}

bool CBackupClient::DoBackup( HANDLE hSrcFile, const tstring& SrcPath, int Index, DWORD SrcAttribute, bool Delete, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime, tstring& DstPath, unsigned short& DstCRC, const CSettings& Settings, HANDLE Pid )
{
    bool ret = false;
    tstring strMappedPathNoIndex = Utils::MapToDestination( Settings.Destination, SrcPath );

    std::wstringstream oss;
    oss << strMappedPathNoIndex << _T(".");
    if( Delete )
        oss << _T("DELETED.");
    oss << Index;
    tstring strDestPath = oss.str();
    DstPath = strDestPath;

    Utils::CPathDetails pd;
    if( ! pd.Parse( false, strDestPath ) )
    {
        TRACE_ERROR( _T("CEUSER: Failed to parse %s"), strDestPath.c_str() );
        return false;
    }

    HANDLE hDestFile = NULL;

    unsigned char Buffer[4*1024];
    DWORD BufferLength = sizeof(Buffer);
    BOOL bRead = TRUE;
    unsigned short crc = 0xFFFF;
    while( bRead )
    {
        DWORD dwRead = 0;

        bRead = ::ReadFile( hSrcFile, Buffer, BufferLength, &dwRead, NULL ); // ERROR_INVALID_PARAMETER

        if( ! bRead )
        {
            TRACE_ERROR( _T("CEUSER: ReadFile %s failed, hFile=%p status=%s"), SrcPath.c_str(), hSrcFile, Utils::GetLastErrorString().c_str() );
            goto Cleanup;
        }

        if( dwRead > 0 )
        {
            if( hDestFile == NULL )
            {
                //Lets create directory only if file is not empty
                if( ! Utils::CreateDirectory( pd.Directory ) )
                {
                    goto Cleanup;
                }

                hDestFile = ::CreateFile( strDestPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
                if( hDestFile == INVALID_HANDLE_VALUE )
                {
                    TRACE_ERROR( _T("CEUSER: CreateFile failed to open destination file %s, error=%s"), strDestPath.c_str(), Utils::GetLastErrorString().c_str() );
                    goto Cleanup;
                }
            }

            crc = Utils::Crc16( Buffer, BufferLength, crc );

            DWORD dwWritten = 0;
            BOOL bWrite = ::WriteFile( hDestFile, Buffer, dwRead, &dwWritten, NULL );
            if( ! bWrite )
            {
                TRACE_ERROR( _T("CEUSER: WriteFile %s failed, error=%s"), SrcPath.c_str(), Utils::GetLastErrorString().c_str() );
                goto Cleanup;
            }
        }
		else
		{
			bRead = FALSE;
		}
    }

    if( hDestFile == NULL )
    {
        TRACE_INFO( _T("CEUSER: Skipping empty file %s"), SrcPath.c_str() );
        goto Cleanup;
    }

    if( ! ::SetFileTime( hDestFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        TRACE_ERROR( _T("CEUSER: SetFileTime( %s ) failed, error=%s"), strDestPath.c_str(), Utils::GetLastErrorString().c_str() );
        goto Cleanup;
    }

    //Copy attributes
    if( ! ::SetFileAttributes( strDestPath.c_str(), SrcAttribute ) )
    {
        TRACE_ERROR( _T("CEUSER: SetFileAttributes( %s, 0x%X ) failed, error=%s"), strDestPath.c_str(), SrcAttribute, Utils::GetLastErrorString().c_str() );
        goto Cleanup;
    }

    ret = true;
    DstCRC = crc;

Cleanup:
    if( hDestFile != NULL && hDestFile != INVALID_HANDLE_VALUE )
    {
        if( ! ::CloseHandle( hDestFile ) )
            TRACE_ERROR( _T("CEUSER: CloseHandle hDestFile failed, hDestFile=%p"), hDestFile );
    }

    return ret;
}

bool CBackupClient::BackupFile ( HANDLE hFile, const tstring& SrcPath, DWORD SrcAttribute, bool Delete, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime, const CSettings& Settings, HANDLE Pid )
{
    if( SrcAttribute & FILE_ATTRIBUTE_DIRECTORY )
    {
        TRACE_ERROR( _T("CEUSER: WARNING: Directory attribute detected for %s. Skipping"), SrcPath.c_str() );
        return true;
    }

    tstring strLower = Utils::ToLower( SrcPath );
    if( strLower.substr( 0, Settings.Destination.size() ) == Settings.Destination )
    {
        TRACE_INFO( _T("CEUSER: Skipping write to Destination=%s"), SrcPath.c_str() );
        return true;
    }

    if( IsIncluded( strLower, Settings ) )
    {
        int Index = 1;

        __int64 LastBackupTime = 0;
        unsigned short LastCRC = 0;

        {
            CAutoLock lock( &_guardMap );
            std::map<tstring, CBackupFile>::iterator it = _mapBackupFiles.find( strLower );
            if( it != _mapBackupFiles.end() )
            {
                Index = it->second.LastIndex + 1;
                if( it->second.LastBackupTime )
                    LastBackupTime = it->second.LastBackupTime;
                LastCRC = it->second.LastBackupCRC;
            }
        }

        if( LastBackupTime && Utils::AddMinutes( LastBackupTime, Settings.IgnoreSaveForMinutes ) > Utils::NowTime() )
        {
            TRACE_INFO( _T("CEUSER: Skipping write %s. Too frequently. Wait %d minute(s)"), SrcPath.c_str(), Settings.IgnoreSaveForMinutes + 1 - Utils::TimeToMinutes( Utils::NowTime() - LastBackupTime ) );
            return true;
        }

        if( _BackupFolderSize/1024/1024 >= Settings.MaxBackupSizeMB )
        {
            TRACE_INFO( _T("CEUSER: Skipping write %s. Max backup folder size (%d MB) reached"), SrcPath.c_str(), Settings.MaxBackupSizeMB );
            return true;
        }

        LARGE_INTEGER FileSize;
        BOOL ok = ::GetFileSizeEx( hFile, &FileSize );
        if( ! ok )
        {
            TRACE_ERROR( _T("CEUSER: GetFileSizeEx( %s ) failed, error=%s"), SrcPath.c_str(), Utils::GetLastErrorString().c_str() );
            return false;
        }

        if( FileSize.QuadPart > Settings.MaxFileSizeBytes )
        {
            TRACE_INFO( _T("CEUSER: Skipping write %s. The file is too big ( %ld > %d bytes)"), SrcPath.c_str(), FileSize.QuadPart, Settings.MaxFileSizeBytes );
            return true;
        }

        tstring DstPath;
        unsigned short DstCRC = 0;
        bool ret = DoBackup( hFile, strLower, Index, SrcAttribute, Delete, CreationTime, LastAccessTime, LastWriteTime, DstPath, DstCRC, Settings, Pid );
        if( ret )
        {
            if( LastCRC == DstCRC )
            {
                if( ! ::DeleteFile( DstPath.c_str() ) )
                {
                    TRACE_ERROR( _T("CEUSER: BackupFile: Duplication/DeleteFile: Failed to delete %s"), DstPath.c_str() );
		            return false;
                }
                TRACE_INFO( _T("CEUSER: Skipping write %s. Same content. CRC: %x"), SrcPath.c_str(), LastCRC );
                return true;
            }
            else
            {
                TRACE_INFO( _T("CEUSER: Backup done OK: %s, index=%d%s"), SrcPath.c_str(), Index, Delete ? _T(" DELETED") : _T("") );
            }

            CBackupCopy bc;
            bc.Index = Index;
            bc.Time = Utils::FileTimeToInt64( LastWriteTime );
            bc.Deleted = Delete;

            {
                CAutoLock lock( &_guardMap );
                std::map<tstring, CBackupFile>::iterator it = _mapBackupFiles.find( strLower );
                if( it == _mapBackupFiles.end() )
                {
                    CBackupFile bfNew;
                    bfNew.DstPath = Utils::ToLower( Utils::MapToDestination( Settings.Destination, strLower ) );
                    _mapBackupFiles[strLower] = bfNew;
                }

                it = _mapBackupFiles.find( strLower );
                CBackupFile& bf = it->second;

                if( Index > bf.LastIndex )
                    bf.LastIndex = Index;
                bf.Copies.push_back( bc );
                std::sort( bf.Copies.begin(), bf.Copies.end() );
                bf.LastBackupTime = Utils::NowTime();
                bf.LastBackupCRC = DstCRC;

                _BackupFolderSize += FileSize.QuadPart;
            }

            {
                CAutoLock lock( &_guardCallback );
                if( _BackupEvent )
                    _BackupEvent( SrcPath.c_str(), DstPath.c_str(), Delete, Pid );
            }

            CleanupFiles( strLower, Settings );
        }

        return ret;
    }
    else
    {
        TRACE_DEBUG_( _T("CEUSER: Skipping NOT Included file=%s"), SrcPath.c_str() );
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
    PBACKUP_NOTIFICATION pNotification = NULL;
    BACKUP_REPLY_MESSAGE replyMessage;
    PBACKUP_MESSAGE pMessage = NULL;
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
        pMessage = CONTAINING_RECORD( pOvlp, BACKUP_MESSAGE, Ovlp );

        if( ! result )
        {
            //  An error occurred.
            hr = HRESULT_FROM_WIN32( ::GetLastError() );
            break;
        }

        pNotification = &pMessage->Notification;
        TRACE_DEBUG_( _T("CEUSER: File write notification: Path=%s HANDLE=0x%p"), pNotification->Path, pNotification->hFile );

        FILETIME CreationTime, LastAccessTime, LastWriteTime;
        CreationTime.dwLowDateTime = pNotification->CreationTime.LowPart;
        CreationTime.dwHighDateTime = pNotification->CreationTime.HighPart;
        LastAccessTime.dwLowDateTime = pNotification->LastAccessTime.LowPart;
        LastAccessTime.dwHighDateTime = pNotification->LastAccessTime.HighPart;
        LastWriteTime.dwLowDateTime = pNotification->LastWriteTime.LowPart;
        LastWriteTime.dwHighDateTime = pNotification->LastWriteTime.HighPart;

        CSettings settings = GetSettings();
        result = BackupFile( pNotification->hFile, pNotification->Path, pNotification->FileAttributes, pNotification->DeleteOperation != 0, CreationTime, LastAccessTime, LastWriteTime, settings, pNotification->ProcessId );

#if METHOD_CLOSE_HANDLE_IN_DRIVER

#else
		::CloseHandle( pNotification->hFile );
#endif

        replyMessage.ReplyHeader.Status = 0;
        replyMessage.ReplyHeader.MessageId = pMessage->MessageHeader.MessageId;

        replyMessage.Reply.OkToOpen = TRUE;

        hr = ::FilterReplyMessage( Port, (PFILTER_REPLY_HEADER) &replyMessage, sizeof( replyMessage ) );
        if( SUCCEEDED( hr ) )
        {
            //TRACE_DEBUG_( _T("Replied back to driver") );
        }
        else
        {
            TRACE_ERROR( _T("CEUSER: Error replying message. Error = 0x%X"), hr );
            break;
        }

        memset( &pMessage->Ovlp, 0, sizeof( OVERLAPPED ) );

        hr = ::FilterGetMessage( Port, &pMessage->MessageHeader, FIELD_OFFSET( BACKUP_MESSAGE, Ovlp ), &pMessage->Ovlp );
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
            TRACE_INFO( _T("CEUSER: Port is disconnected, probably due to scanner filter unloading") );
        }
        else
        {
            TRACE_DEBUG( _T("CEUSER: Port Get: Unknown error occurred. Error = 0x%X"), hr );
        }
    }
    else if( pMessage )
    {
        try
        {
            free( pMessage );
        }
        catch( ... ) {}
    }
}

bool CBackupClient::IterateDirectories( const tstring& Destination, const tstring& Directory, std::map<tstring, CBackupFile>& BackupFiles, __int64& BackupFolderSize )
{	
    if( Utils::DoesDirectoryExists( Directory ) )
    {
		WIN32_FIND_DATA ffd = {0};
        HANDLE hFind = INVALID_HANDLE_VALUE;
		tstring strSearchFor = Directory + _T("\\*");

        hFind = ::FindFirstFile( strSearchFor.c_str(), &ffd );
        if( hFind == INVALID_HANDLE_VALUE )
        {
			DWORD status = ::GetLastError();
			if( status != ERROR_FILE_NOT_FOUND )
			{
				TRACE_ERROR( _T("CEUSER: FindFirstFile failed in %s, error=%s"), Directory.c_str(), Utils::GetLastErrorString().c_str() );
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
				if(  strName != _T(".") && strName != _T("..") )
					if( ! IterateDirectories( Destination, Directory + _T("\\") + strName, BackupFiles, BackupFolderSize ) )
						return false;
			}
			else
			{
				tstring DstPath = Directory + _T("\\") + strName;
                DstPath = Utils::ToLower( DstPath );
                Utils::CPathDetails pd;
                if( ! pd.Parse( true, DstPath ) )
                {
                    TRACE_ERROR( _T("CEUSER: IterateDirectories: Failed to parse %s"), DstPath.c_str() );
                    return false;
                }
                int Index = std::stoi(pd.Index);

                tstring SrcPath = Utils::ToLower( Utils::MapToOriginal( Destination, Directory + _T("\\") + pd.Name ) );
                tstring DstPathNoIndex = Utils::ToLower( Directory + _T("\\") + pd.Name );
                std::map<tstring, CBackupFile>::iterator it = BackupFiles.find( SrcPath );
                CBackupFile bf;
                if( it != BackupFiles.end() )
                {
                    bf = BackupFiles[SrcPath];
                }
                else
                {
                    bf.DstPath = DstPathNoIndex;
                }

                HANDLE hSrcFile = ::CreateFile( DstPath.c_str(), FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL );
                if( hSrcFile == INVALID_HANDLE_VALUE )
                {
                    TRACE_ERROR( _T("CEUSER: IterateDirectories: CreateFile: Failed to delete %s"), DstPath.c_str() );
		            return false;
                }

                FILETIME CreationTime, LastAccessTime, LastWriteTime;
                BOOL ret = ::GetFileTime( hSrcFile, &CreationTime, &LastAccessTime, &LastWriteTime );
                if( ! ret )
                {
                    TRACE_ERROR( _T("CEUSER: IterateDirectories: GetFileTime( %s ) failed, error=%s"), DstPath.c_str(), Utils::GetLastErrorString().c_str() );
                    ::CloseHandle( hSrcFile );
                    return false;
                }

                LARGE_INTEGER FileSize;
                ret = ::GetFileSizeEx( hSrcFile, &FileSize );
                ::CloseHandle( hSrcFile );
                if( ! ret )
                {
                    TRACE_ERROR( _T("CEUSER: IterateDirectories: GetFileSizeEx( %s ) failed, error=%s"), DstPath.c_str(), Utils::GetLastErrorString().c_str() );
                    return false;
                }

                __int64 iFileSize = FileSize.QuadPart;

                CBackupCopy bc;
                bc.Index = Index;
                bc.Time = Utils::FileTimeToInt64( LastWriteTime );
                bc.Deleted = pd.Deleted;
                bf.Copies.push_back( bc );
                if( Index > bf.LastIndex )
                    bf.LastIndex = Index;

                BackupFiles[SrcPath] = bf;
                BackupFolderSize += iFileSize;
            }
        } while( ::FindNextFile( hFind, &ffd ) );

        ::FindClose( hFind );
    }

	return true;
}

bool CBackupClient::ScanRepositoryData()
{
    _BackupFolderSize = 0;

    //Fill internal map existing backup files information
    if( ! IterateDirectories( GetSettings().Destination, GetSettings().Destination, _mapBackupFiles, _BackupFolderSize ) )
        return false;

    return true;
}

bool CBackupClient::DeleteBackup( const tstring& SrcPath, const tstring& DstPath, int Index, bool Deleted )
{
    std::wstringstream oss;
    oss << DstPath << _T(".");
    if( Deleted )
        oss << _T("DELETED.");
    oss << Index;
    tstring strDestPath = oss.str();

    if( ! ::DeleteFile( strDestPath.c_str() ) )
    {
        TRACE_ERROR( _T("CEUSER: Delete: DeleteFile: Failed to delete %s"), strDestPath.c_str() );
		return false;
    }

    {
        CAutoLock lock( &_guardCallback );
        if( _CleanupEvent )
            _CleanupEvent( SrcPath.c_str(), strDestPath.c_str(), Deleted );
    }

    return true;
}

bool CBackupClient::ReloadConfig( const tstring& IniPath, tstring& error )
{
    CAutoLock lock( &_guardSettings );

	if( ! _Settings.Init( IniPath, error ) )
        return false;

    if( _hPort && _hCompletion )
        SendDestination( _Settings.Destination );

    return true;
}

CSettings& CBackupClient::GetSettings()
{
    CAutoLock lock( &_guardSettings );

    return _Settings;
}

bool CBackupClient::SetBackupCallback( CallBackBackupCallback BackupEvent )
{
    CAutoLock lock( &_guardCallback );
    _BackupEvent = BackupEvent;

    return true;
}

bool CBackupClient::SetCleanupCallback( CallBackCleanupEvent CleanupEvent )
{
    CAutoLock lock( &_guardCallback );
    _CleanupEvent = CleanupEvent;

    return true;
}
