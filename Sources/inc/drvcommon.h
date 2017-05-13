#pragma once

#define MAX_PATH_SIZE 270
#define MAX_UNI_PATH_SIZE 1024

#define BACKUP_PORT_NAME L"\\CeBackupPort"
#define RESTORE_PORT_NAME L"\\CeRestorePort"

typedef struct _BACKUP_NOTIFICATION
{
    HANDLE hFile;
    WCHAR Path[MAX_PATH_SIZE];
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} BACKUP_NOTIFICATION, *PBACKUP_NOTIFICATION;

typedef struct _BACKUP_REPLY
{
    BOOLEAN OkToOpen;
} BACKUP_REPLY, *PBACKUP_REPLY;
