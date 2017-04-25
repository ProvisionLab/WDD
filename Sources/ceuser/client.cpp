/*
- timestamp as FA
-filter files from driver, store in dest
*/

#include "common.h"
#include "client.h"
#include "cecommon.h"
#include "ceuser.h"
#include "INIReader.h"

//  Default and Maximum number of threads.
#define BACKUP_REQUEST_COUNT            5
#define BACKUP_DEFAULT_THREAD_COUNT     2
#define BACKUP_MAX_THREAD_COUNT         64

#define ERROR_PRINT _tprintf
#define INFO_PRINT  _tprintf
#define DEBUG_PRINT 

class CPathDetails
{
public:
    CPathDetails();
    bool Parse( bool aMapped, const tstring& Path );

    tstring Directory;
    tstring Name;
    tstring Extension;
    bool Mapped;
    int Index;
};

CPathDetails::CPathDetails()
    : Index(0)
    , Mapped(false)
{
}

bool CPathDetails::Parse( bool aMapped, const tstring& Path )
{
    Mapped = aMapped;
    Directory = _T("");

    size_t pos = Path.rfind( _T('\\') ) ;
    if( pos == -1 )
        return false;

    Directory = Path.substr( 0, pos );
    Name = Path.substr( pos + 1 );
    pos = Name.rfind( _T('.') ) ;
    if( pos != -1 )
        Extension = Name.substr( pos + 1 );

    return true;
}

/*File indexes
    a.txt     -> a.txt.X
    a.txt.1   -> a.txt.1.X
    Directories
    1\1.txt   -> 1\X\1.txt
    1\1\1.txt -> 1\1\X\1.txt
*/
bool CBackupClient::GetLastIndex( const tstring& MappedPath, int& Index )
{
    Index = 0;

    tstring strFinalPath = _strDestination + _T("\\") + MappedPath;

    CPathDetails pd;
    if( ! pd.Parse( true, strFinalPath ) )
    {
        ERROR_PRINT( _T("ERROR: Failed to parse %s\n"), MappedPath.c_str() );
        return false;
    }

    if( DoesDirectoryExists( pd.Directory ) )
    {
        WIN32_FIND_DATA ffd;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        hFind = ::FindFirstFile( (pd.Directory + _T("\\*")).c_str(), &ffd );
        if( hFind == INVALID_HANDLE_VALUE )
        {
            ERROR_PRINT( _T("ERROR: FindFirstFile failed %s\n"), pd.Directory.c_str() );
            return false;
        }

        do
        {
            if( ! (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            {
                 if( _tcsstr( ffd.cFileName, pd.Name.c_str() ) != NULL )
                 {
                     TCHAR* point = _tcsrchr(  ffd.cFileName, _T('.') );
                     if( point )
                     {
                         tstring strIndex = point + 1;
                         if( strIndex.size() )
                         {
                             int iIndex = _tstoi( strIndex.c_str() );
                             if( iIndex > Index )
                                 Index = iIndex;
                         }
                     }
                 }
            }
        } while( ::FindNextFile( hFind, &ffd ) );

        ::FindClose( hFind );
    }

    return true;
}

bool CBackupClient::IsIncluded( const tstring& Path )
{
    CPathDetails pd;
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

    std::vector<tstring>::iterator it = std::find( _arrExcludedFile.begin(), _arrExcludedFile.end(), Path ) ;
    if( it != _arrExcludedFile.end() )
        return false;

    for( std::vector<tstring>::iterator it = _arrExcludedDirectory.begin(); it != _arrExcludedDirectory.end(); it++ )
    {
        if( Path.substr( 0, (*it).size() ) == (*it)  )
            return false;
    }

    it = std::find( _arrIncludedFile.begin(), _arrIncludedFile.end(), Path ) ;
    if( it != _arrIncludedFile.end() )
        return true;

    for( std::vector<tstring>::iterator it = _arrIncludedDirectory.begin(); it != _arrIncludedDirectory.end(); it++ )
    {
        if( Path.substr( 0, (*it).size() ) == (*it)  )
            return true;
    }

    return false;
}

tstring CBackupClient::MapToDestination( const tstring& Path )
{
    tstring strMapped = Path;

    size_t pos = strMapped.find( _T(':') );
    if( pos == -1 )
    {
        ERROR_PRINT( _T("ERROR: MapToDestination failed: Disk not found in the path %s\n"), Path.c_str() );
        return false;
    }

    strMapped = strMapped.substr( 0, pos ) + strMapped.substr( pos + 1 );

    return strMapped;
}

bool CBackupClient::DoBackup( HANDLE hSrcFile, const tstring& SrcPath, DWORD SrcAttribute, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime )
{
    bool ret = false;
    tstring strMappedPath = MapToDestination( SrcPath );

    int iIndex = 0;
    if( ! GetLastIndex( strMappedPath, iIndex ) )
        return false;

    iIndex ++;
    std::wstringstream oss;
    oss << _strDestination << _T("\\") << strMappedPath << _T(".") << iIndex;
    tstring strDestPath = oss.str();

    CPathDetails pd;
    if( ! pd.Parse( true, strDestPath ) )
    {
        ERROR_PRINT( _T("ERROR: Failed to parse %s\n"), strDestPath.c_str() );
        return false;
    }

    if( ! CreateDirectories( pd.Directory ) )
        return false;

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
            ERROR_PRINT( _T("ERROR: ReadFile %s failed, hFile=%p status=%s\n"), SrcPath.c_str(), hSrcFile, GetLastErrorString().c_str() );
            ret = false;
            goto Cleanup;
        }

        if( dwRead > 0 )
        {
            if( hDestFile == NULL )
            {
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
                ERROR_PRINT( _T("ERROR: WriteFile %s failed, error=0x%X\n"), strDestPath.c_str(), ::GetLastError() );
                ret = false;
                goto Cleanup;
            }
        }
        else
        {
            DEBUG_PRINT( _T("DEBUG: File is empty %s. Ignoring\n"), strDestPath.c_str() );
            bRead = FALSE;
        }
    }

    if( hDestFile && ! ::SetFileTime( hDestFile, &CreationTime, &LastAccessTime, &LastWriteTime ) )
    {
        ERROR_PRINT( _T("ERROR: SetFileTime( %s, 0x%X ) failed, error=%s\n"), strDestPath.c_str(), SrcAttribute, GetLastErrorString().c_str() );
        ret = false;
        goto Cleanup;
    }

    //Copy attributes
    if( hDestFile && ! ::SetFileAttributes( strDestPath.c_str(), SrcAttribute ) )
    {
        ERROR_PRINT( _T("ERROR: SetFileAttributes( %s, 0x%X ) failed, error=%s\n"), strDestPath.c_str(), SrcAttribute, GetLastErrorString().c_str() );
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

    tstring strLower = ToLower( Path );
    if( strLower.size() >= _strDestination.size() && strLower.substr( 0, _strDestination.size() ) == _strDestination )
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

    InitializeCriticalSection( &_guardDestFile );

    if( ! ParseIni( IniPath ) )
        return false;

/*TMP
    tstring name = _T("C:\\Max\\Test\\file1.txt");
    HANDLE hFile = ::CreateFile( name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    DWORD dwAttributes = ::GetFileAttributesEx( name.c_str() );
    FILETIME CreationTime, LastAccessTime, LastWriteTime, ChangeTime;
    ::GetFileTime( hFile, &CreationTime, &LastAccessTime, &LastWriteTime );
    BackupFile( hFile, name, dwAttributes, CreationTime, LastAccessTime, LastWriteTime );
    ::CloseHandle( hFile );
*/

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

bool CBackupClient::ParseIni( const tstring& IniPath )
{
    INIReader reader( IniPath );

    if( reader.ParseError() < 0 )
    {
        DEBUG_PRINT( _T("Can't load '%s'\n"), IniPath.c_str() );
        return false;
    }

    _strDestination = ToLower( reader.Get( _T("Destination"), _T("Path"), _T("")) );

    if( _strDestination.length() == 0 )
    {
        DEBUG_PRINT( _T("[Destination]Path not found in '%s'\n"), IniPath.c_str() );
        return false;
    }

    _strDestination = RemoveEndingSlash( _strDestination );

    if( ! CreateDirectories( _strDestination.c_str() ) )
        return false;

    INFO_PRINT( _T("[Settings] Backup directory: %s\n"), _strDestination.c_str() );

    int iCount = reader.GetInteger( _T("IncludeFile"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("File" ) << i;
        tstring strName;
        tstring strFile = ToLower( reader.Get( _T("IncludeFile"), oss.str(), _T("") ) );

        if( strFile.length() == 0 )
        {
            DEBUG_PRINT( _T("[IncludeFile] '%s' not found in '%s'\n"), oss.str().c_str(), IniPath.c_str() );
            return false;
        }

        _arrIncludedFile.push_back( strFile );
    }

    iCount = reader.GetInteger( _T("IncludeDirectory"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("Directory" ) << i;
        tstring strName;
        tstring strDirectory = ToLower( reader.Get( _T("IncludeDirectory"), oss.str(), _T("") ) );

        if( strDirectory.length() == 0 )
        {
            DEBUG_PRINT( _T("[IncludeDirectory] '%s' not found in '%s'\n"), oss.str().c_str(), IniPath.c_str() );
            return false;
        }

        _arrIncludedDirectory.push_back( RemoveEndingSlash( strDirectory ) );
    }

    iCount = reader.GetInteger( _T("ExcludeFile"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("File" ) << i;
        tstring strName;
        tstring strFile = ToLower( reader.Get( _T("ExcludeFile"), oss.str(), _T("") ) );

        if( strFile.length() == 0 )
        {
            DEBUG_PRINT( _T("[ExcludeFile] '%s' not found in '%s'\n"), oss.str().c_str(), IniPath.c_str() );
            return false;
        }

        _arrExcludedFile.push_back( strFile );
    }

    iCount = reader.GetInteger( _T("ExcludeDirectory"), _T("Count"), 0 );
    for( int i=1; i<=iCount; i++ )
    {
        std::wstringstream oss;
        oss << _T("Directory" ) << i;
        tstring strName;
        tstring strDirectory = ToLower( reader.Get( _T("ExcludeDirectory"), oss.str(), _T("") ) );

        if( strDirectory.length() == 0 )
        {
            DEBUG_PRINT( _T("[ExcludeDirectory] '%s' not found in '%s'\n"), oss.str().c_str(), IniPath.c_str() );
            return false;
        }

        _arrExcludedDirectory.push_back( RemoveEndingSlash( strDirectory ) );
    }

    return true;
}

tstring CBackupClient::RemoveEndingSlash( const tstring& Dir )
{
    tstring ret = Dir;
    if( ret[ret.size()-1] == _T('\\') )
        ret.substr( 0, ret.size()-1 );

    return ret;
}

tstring CBackupClient::ToLower( const tstring& str )
{
    tstring data = str;
    std::transform( data.begin(), data.end(), data.begin(), ::tolower ) ;

    return data;
}

bool CBackupClient::DoesDirectoryExists( const tstring& Directory )
{
    return ::PathFileExists( Directory.c_str() ) != 0;
}

bool CBackupClient::CreateDirectories( const tstring& Directory )
{
    size_t pos = 0;
    
    bool bExist = false ;
    while( ! bExist )
    {
        pos = Directory.find( _T('\\'), pos );

        tstring strSubDirectory;
        if( pos == -1 )
        {
            strSubDirectory = Directory;
            bExist = true;
        }
        else
            strSubDirectory = Directory.substr( 0, pos + 1 );

        if( ! DoesDirectoryExists( strSubDirectory ) )
        {
            BOOL ret = ::CreateDirectory( strSubDirectory.c_str(), NULL ) ;
            if( ! ret )
            {
                if( ::GetLastError() != ERROR_ALREADY_EXISTS )
                {
                    ERROR_PRINT( _T("ERROR: CreateDirectory %s failed, error=0x%X\n"), strSubDirectory.c_str(), ::GetLastError() );
                    return false;
                }
            }
        }

        pos ++;
    }

    return true;
}

tstring CBackupClient::GetLastErrorString()
{
    tstring ret;
    if( ::GetLastError() == ERROR_ACCESS_DENIED )
        ret = _T(" ERROR_ACCESS_DENIED");
    else if( ::GetLastError() == ERROR_INVALID_HANDLE )
        ret = _T(" ERROR_INVALID_HANDLE");
    else if( ::GetLastError() == ERROR_INVALID_PARAMETER )
        ret = _T(" ERROR_INVALID_PARAMETER");
    else if( ::GetLastError() == ERROR_INVALID_FUNCTION )
        ret = _T(" ERROR_INVALID_FUNCTION");
    else if( ::GetLastError() == ERROR_FILE_EXISTS )
        ret = _T(" ERROR_FILE_EXISTS");
    else
    {
        std::wstringstream oss;
        oss << _T(" error=%d") << ::GetLastError();
        ret = oss.str();
    }

    return ret;
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
