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

class CBackupClient
{
public:
    CBackupClient();
    bool Run( const tstring& IniPath, tstring& error, bool async = false );
    bool Stop();

private:
    bool IsRunning();
    bool DoBackup( HANDLE hSrcFile, const tstring& SrcPath, DWORD SrcAttribute, bool Delete, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime );
    bool BackupFile ( HANDLE hFile, const tstring& Path, DWORD SrcAttribute, bool Delete, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime );
    bool IsIncluded( const tstring& Path );

    static DWORD _BackupWorker( _In_ PBACKUP_THREAD_CONTEXT pContext );
    void BackupWorker( HANDLE Completion, HANDLE Port );

	CSettings _Settings;
    CRITICAL_SECTION _guardDestFile;
    HANDLE _hPort, _hCompletion;
    BACKUP_THREAD_CONTEXT _Context;
    HANDLE _Threads[BACKUP_MAX_THREAD_COUNT];
};
