#include <fltKernel.h>
#include <ntstrsafe.h>
#include <dontuse.h>
#include <suppress.h>
#include "cebackup.h"
#include "drvcommon.h"
#include "util.h"

extern BACKUP_DATA g_CeBackupData;

NTSTATUS SendHandleToUser ( _In_ HANDLE hFile, _In_ PFLT_VOLUME Volume, _In_ PFLT_FILE_NAME_INFORMATION FltNameInfo, _In_ PFILE_BASIC_INFORMATION FileInfo, _Out_ PBOOLEAN OkToOpen )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG replyLength = 0;
    PBACKUP_NOTIFICATION pNotification = NULL;
    PDEVICE_OBJECT pDiskDeviceObject = NULL;
    DWORD dwPathLength = 0;
    WCHAR* pStrPath = NULL;
    DECLARE_UNICODE_STRING_SIZE( uniDisk, 10 );

    *OkToOpen = TRUE;

    if( g_CeBackupData.ClientBackupPort == NULL )
        return STATUS_PORT_DISCONNECTED;

    //Getting Disk Letter:
    status = FltGetDiskDeviceObject( Volume, &pDiskDeviceObject );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\n!!!! CB: ERROR FltGetDiskDeviceObject failed. status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    status = IoVolumeDeviceToDosName( pDiskDeviceObject, &uniDisk );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\n!!! CB: ERROR IoVolumeDeviceToDosName failed. status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    USHORT lenRequired = uniDisk.Length + FltNameInfo->ParentDir.Length + FltNameInfo->FinalComponent.Length + 1;
    if( lenRequired > MAX_PATH_SIZE )
    {
        ERROR_PRINT( "\n!!! CB: ERROR Result Path is too big (%d) > %d\n", lenRequired, MAX_PATH_SIZE );
        goto Cleanup;
    }
    
    dwPathLength = uniDisk.Length + FltNameInfo->ParentDir.Length + FltNameInfo->FinalComponent.Length + 2;

    pStrPath = ExAllocatePoolWithTag( NonPagedPool, dwPathLength, 'kBeC' );
    if( ! pStrPath )
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //RtlUnicodeStringPrintf( &uniPath, L"%wZ%wZ%wZ", uniDisk, FltNameInfo->ParentDir, FltNameInfo->FinalComponent ) ;
    RtlZeroMemory( pStrPath, dwPathLength );
    RtlStringCbPrintfW( pStrPath, dwPathLength, L"%s%s%s", uniDisk.Buffer, FltNameInfo->ParentDir.Buffer, FltNameInfo->FinalComponent.Buffer ) ;

    DEBUG_PRINT( "CB: INFO Sending message Path=%S HANDLE=0x%p\n", pStrPath, hFile );

    pNotification = ExAllocatePoolWithTag( NonPagedPool, sizeof( BACKUP_NOTIFICATION ), 'kBeC' );
    if( ! pNotification )
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory( pNotification, sizeof(*pNotification) );
    pNotification->hFile = hFile;
    RtlCopyMemory( &pNotification->Path, pStrPath, dwPathLength );
    pNotification->CreationTime = FileInfo->CreationTime;
    pNotification->LastAccessTime = FileInfo->LastAccessTime;
    pNotification->LastWriteTime = FileInfo->LastWriteTime;
    pNotification->ChangeTime = FileInfo->ChangeTime;
    pNotification->FileAttributes = FileInfo->FileAttributes;

    replyLength = sizeof( BACKUP_REPLY );

    status = FltSendMessage( g_CeBackupData.Filter, &g_CeBackupData.ClientBackupPort, pNotification, sizeof(BACKUP_NOTIFICATION), pNotification, &replyLength, NULL );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR FltSendMessage failed status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    *OkToOpen = ((PBACKUP_REPLY) pNotification)->OkToOpen;

Cleanup:
    if( pNotification )
        ExFreePoolWithTag( pNotification, 'kBeC' );

    if( pStrPath )
        ExFreePoolWithTag( pStrPath, 'kBeC' );

    return status;
}
