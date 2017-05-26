#pragma once

#include "usercommon.h"
#include "Settings.h"

//  Default and Maximum number of threads.
#define BACKUP_REQUEST_COUNT            5
#define BACKUP_DEFAULT_THREAD_COUNT     2
#define BACKUP_MAX_THREAD_COUNT         64

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

class CBackupClient
{
public:
    CBackupClient();
    bool Run( const tstring& IniPath, tstring& error, bool async = false );
    bool Stop();

private:
    bool IsRunning();
    bool ScanRepositoryData();
    bool DoBackup( HANDLE hSrcFile, const tstring& SrcPath, int Index, DWORD SrcAttribute, bool Delete, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime, tstring& DstPath, unsigned short& DstCRC );
    bool BackupFile ( HANDLE hFile, const tstring& SrcPath, DWORD SrcAttribute, bool Delete, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime );
    bool IsIncluded( const tstring& Path );
    bool CleanupFiles( const tstring& SrcPath );
    bool DeleteBackup( const tstring& SrcPath, int Index, bool Deleted );
    bool IterateDirectories( const tstring& Destination, const tstring& Directory );

    static DWORD _BackupWorker( _In_ PBACKUP_THREAD_CONTEXT pContext );
    void BackupWorker( HANDLE Completion, HANDLE Port );

	CSettings _Settings;
    CRITICAL_SECTION _guardMap;
    HANDLE _hPort, _hCompletion;
    BACKUP_THREAD_CONTEXT _Context;
    HANDLE _Threads[BACKUP_MAX_THREAD_COUNT];
    std::map<tstring, CBackupFile> _mapBackupFiles; // <SrcPath, DstFile>
};
