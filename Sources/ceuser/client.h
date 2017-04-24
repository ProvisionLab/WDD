#pragma once

#include "common.h"

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
    bool Run( const tstring& IniPath );

private:
    bool ParseIni( const tstring& IniPath );
    bool IsIncluded( const tstring& Path );
    bool DoBackup( HANDLE hSrcFile, const tstring& SrcPath, DWORD SrcAttribute, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime );
    bool BackupFile ( HANDLE hFile, const tstring& Path, DWORD SrcAttribute, FILETIME CreationTime, FILETIME LastAccessTime, FILETIME LastWriteTime );
    tstring MapToDestination( const tstring& Path );
    bool GetLastIndex( const tstring& MappedPath, int& Index );
    tstring RemoveEndingSlash( const tstring& Dir );
    bool DoesDirectoryExists( const tstring& Directory );
    bool CreateDirectories( const tstring& Directory );

    static DWORD _BackupWorker( _In_ PBACKUP_THREAD_CONTEXT pContext );
    void BackupWorker( HANDLE Completion, HANDLE Port );

    tstring _strDestination;
    std::vector<tstring> _arrIncludedFile;
    std::vector<tstring> _arrExcludedFile;
    std::vector<tstring> _arrIncludedDirectory;
    std::vector<tstring> _arrExcludedDirectory;
};
