#pragma once

#include "usercommon.h"
#include "Settings.h"

//  Default and Maximum number of threads.
#define BACKUP_REQUEST_COUNT            5
#define BACKUP_DEFAULT_THREAD_COUNT     2
#define BACKUP_MAX_THREAD_COUNT         64

typedef void (*CallBackBackupCallback)( const wchar_t* SrcPath, const wchar_t* DstPath, int Deleted, HANDLE Pid );
typedef void (*CallBackCleanupEvent)( const wchar_t* SrcPath, const wchar_t* DstPath, int Deleted );

class CBackupClient;

//  Context passed to worker threads
typedef struct _BACKUP_THREAD_CONTEXT
{
    HANDLE Port;
    HANDLE Completion;
    CBackupClient* This;
} BACKUP_THREAD_CONTEXT, *PBACKUP_THREAD_CONTEXT;

class CBackupCopy
{
public:
    CBackupCopy();
    bool operator < ( const CBackupCopy &rhs ) const { return Index < rhs.Index; }

    bool Deleted;
    int Index;
    __int64 Time;
};

class CBackupFile
{
public:
    CBackupFile();

    tstring DstPath;
    int LastIndex; // last saved
    __int64 LastBackupTime;
    unsigned short LastBackupCRC;
    std::vector<CBackupCopy> Copies; // Copies = size
};

typedef enum _EStatus
{
	EOk = 0,
    EFailed = 1,
    EInvalidParameter = 2,
	ENotInitialized = 3,
    EOneInstanceOnly = 5,
	EConfigurationError = 6,
    EDriverIsNotStarted = 7,
    EBackupIsNotStarted = 8,
    EBackupIsAlreadyStarted = 9,
    EBufferOverflow = 10
} EStatus;

class CBackupClient
{
public:
    CBackupClient();
    virtual ~CBackupClient();
    static bool IsDriverStarted();
    static bool IsAlreadyRunning();

    bool Start( const tstring& IniPath, tstring& Error, bool Async = false );
    bool Stop();
    bool IsStarted();
    bool ReloadConfig( const tstring& IniPath, tstring& Error );
    bool LockAccess();
    bool SetBackupCallback( CallBackBackupCallback BackupEvent );
    bool SetCleanupCallback( CallBackCleanupEvent CleanupEvent );

private:
    bool ScanRepositoryData();
    bool DoBackup( HANDLE hSrcFile, const tstring& SrcPath, int Index, DWORD SrcAttribute, bool Delete, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime, tstring& DstPath, unsigned short& DstCRC, const CSettings& Settings, HANDLE Pid );
    bool BackupFile( HANDLE hFile, const tstring& SrcPath, DWORD SrcAttribute, bool Delete, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime, const CSettings& Settings, HANDLE Pid );
    bool IsIncluded( const tstring& Path, const CSettings& settings );
    bool CleanupFiles( const tstring& SrcPath, const CSettings& Settings );
    bool DeleteBackup( const tstring& SrcPath, const tstring& DstPath, int Index, bool Deleted );
    bool IterateDirectories( const tstring& Destination, const tstring& Directory, std::map<tstring, CBackupFile>& BackupFiles, __int64& BackupFolderSize );
    bool SendDestination( const tstring& Destination );

    static DWORD _BackupWorker( _In_ PBACKUP_THREAD_CONTEXT pContext );
    void BackupWorker( HANDLE Completion, HANDLE Port );
    CSettings& GetSettings();

    HANDLE _hLockMutex;
    bool _bStarted;
	CSettings _Settings;
    CRITICAL_SECTION _guardMap;
    CRITICAL_SECTION _guardSettings;
    CRITICAL_SECTION _guardCallback;
    HANDLE _hPort, _hCompletion;
    BACKUP_THREAD_CONTEXT _Context;
    HANDLE _Threads[BACKUP_MAX_THREAD_COUNT];
    std::map<tstring, CBackupFile> _mapBackupFiles; // <SrcPath, DstFile>
    __int64 _BackupFolderSize;
    CallBackBackupCallback _BackupEvent;
    CallBackCleanupEvent _CleanupEvent;
};
