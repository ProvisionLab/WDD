#include <fltKernel.h>
#include <ntstrsafe.h>
#include <dontuse.h>
#include <suppress.h>
#include "cebackup.h"
#include "drvcommon.h"
#include "util.h"

extern BACKUP_DATA g_CeBackupData;

FLT_PREOP_CALLBACK_STATUS OpenAndSend ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, BOOLEAN Delete )
{
    FLT_PREOP_CALLBACK_STATUS filterStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    NTSTATUS status;
    HANDLE hFile = NULL;
    PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;
    PCB_INSTANCE_CONTEXT pInstanceContext = NULL;
    PFILE_OBJECT pSrcFileObject = NULL;
    HANDLE TargetHandle = NULL;
    HANDLE SourceProcessHandle = NULL;
    HANDLE TargetProcessHandle = NULL;
    IO_STATUS_BLOCK ioStatusCreate = {0};
    BOOLEAN bIsDir = FALSE;
    DWORD dwRetLen = 0;
    HANDLE SourceHandle = NULL;
    IO_STATUS_BLOCK ioStatusQuery = {0};
    PDEVICE_OBJECT pSrcDeviceObject = NULL;
    BOOLEAN OkToOpen = TRUE;
    WCHAR* pStrFileName = NULL;
    char* pBufferProcessName = NULL;
    PUNICODE_STRING pUniProcessName = NULL;
    WCHAR* pProcessName = L"";
    OBJECT_ATTRIBUTES oa = {0};
	FILE_BASIC_INFORMATION fileInfo = {0};
    // g_CeBackupData local copy
    HANDLE UserProcessKernel = NULL;
    PEPROCESS BackupProcess = NULL;
	PEPROCESS RestoreProcess = NULL;

#if METHOD_CLOSE_HANDLE_IN_DRIVER
    PFILE_OBJECT pDstFileObject = NULL;
#endif

    status = FltIsDirectory( FltObjects->FileObject, FltObjects->Instance, &bIsDir );
    if( NT_SUCCESS( status ) && bIsDir )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    //  Skip pre-rename operations which always open a directory
    if( FlagOn( Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY ) )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    
    //  Skip paging files
    if( FlagOn( Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE ) )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    // Check if this is a volume open. Skip scanning DASD opens
    if( FlagOn( FltObjects->FileObject->Flags, FO_VOLUME_OPEN ) )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    if( Data->Iopb->TargetFileObject && Data->Iopb->TargetFileObject->FileName.Length == 0 )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    status = FltGetFileNameInformation( Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &pNameInfo );
    if( ! NT_SUCCESS( status ) )
    {
        DEBUG_PRINT( "\n!!! ERROR FltGetFileNameInformation failed. Status=%S\n\n", GetStatusString( status ) );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if( ! pNameInfo )
    {
        ERROR_PRINT( "\nCB: ERROR pNameInfo is NULL\n\n" );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    status = FltParseFileNameInformation( pNameInfo );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\n!!! ERROR FltParseFileNameInformation failed. Status=%S\n\n", GetStatusString( status ) );
        goto Cleanup;
    }

	if( pNameInfo == NULL )
    {
        ERROR_PRINT( "\n!!! ERROR FltParseFileNameInformation: pNameInfo == NULL\n\n" );
        goto Cleanup;
    }

	if( pNameInfo->FinalComponent.Buffer == NULL )
    {
        ERROR_PRINT( "\n!!! WARNING FltParseFileNameInformation: pNameInfo->FinalComponent.Buffer == NULL\n\n" );
        goto Cleanup;
    }

	if( pNameInfo->FinalComponent.Length == 0 )
    {
        ERROR_PRINT( "\n!!! WARNING FltParseFileNameInformation: pNameInfo->FinalComponent.Length == 0\n\n" );
        goto Cleanup;
    }

	pStrFileName = pNameInfo->FinalComponent.Buffer;

    status = ZwQueryInformationProcess( NtCurrentProcess(), ProcessImageFileName, NULL, 0, &dwRetLen );
    if( ! dwRetLen )
    {
        ERROR_PRINT( "\nCB: !!! ERROR: ZwQueryInformationProcess ProcessImageFileName 0 failed. status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    dwRetLen += 2;
    pBufferProcessName = ExAllocatePoolWithTag( NonPagedPool, dwRetLen, 'kBeC' );
    if( ! pBufferProcessName )
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory( pBufferProcessName, dwRetLen );
	status = ZwQueryInformationProcess( NtCurrentProcess(), ProcessImageFileName, pBufferProcessName, dwRetLen, &dwRetLen );
    if( ! NT_SUCCESS(status) || ! dwRetLen )
    {
        ERROR_PRINT( "\nCB: !!! ERROR: ZwQueryInformationProcess ProcessImageFileName failed. status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    pUniProcessName = (PUNICODE_STRING) pBufferProcessName;
	if( pUniProcessName->Length == 0 || pUniProcessName->Buffer == NULL )
    {
        DEBUG_PRINT( "CB: DEBUG: pProcessName->Length = 0\n" );
    }
    else
        pProcessName = pUniProcessName->Buffer ;

    DEBUG_PRINT( "CB: DEBUG: OpenAndSend ENTER with %S %S Process=%S\n", Delete ? L"!DEL! " : L"", pStrFileName, pProcessName );

    ExAcquireFastMutex( &g_CeBackupData.Guard ); //Going to APCL IRQL
    BackupProcess = g_CeBackupData.BackupProcess;
	RestoreProcess = g_CeBackupData.RestoreProcess;
    UserProcessKernel = g_CeBackupData.BackupProcessKernel;
    ExReleaseFastMutex( &g_CeBackupData.Guard );    

    //Skip restore.exe application operations
    if( BackupProcess && IoThreadToProcess( Data->Thread ) == BackupProcess )
    {
        DEBUG_PRINT( "CB: INFO: Skipping CEUSER write to %S\n", pStrFileName );
        goto Cleanup;
    }

    if( RestoreProcess && IoThreadToProcess( Data->Thread ) == RestoreProcess )
    {
        DEBUG_PRINT( "CB: INFO: Skipping RESTORE write to %S\n", pStrFileName );
        goto Cleanup;
    }

	if( BackupProcess == NULL )
    {
        INFO_PRINT( "CB: INFO: CEUSER application is not connected to filter. Skipping\n" );
        goto Cleanup;
    }

    InitializeObjectAttributes( &oa, &pNameInfo->Name, 0, NULL, NULL );

    status = FltGetInstanceContext( FltObjects->Instance, &pInstanceContext );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR FltGetInstanceContext failed status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    // Open file w/o Recursion
    status = FltCreateFile( g_CeBackupData.Filter,
                            pInstanceContext->Instance,
                            &hFile,
                            GENERIC_READ | SYNCHRONIZE,
                            &oa,
                            &ioStatusCreate,
                            (PLARGE_INTEGER) NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            Delete ? FILE_SHARE_READ | FILE_SHARE_DELETE : FILE_SHARE_READ,
                            FILE_OPEN, // If the file already exists, open it instead of creating a new file. If it does not, fail the request and do not create a new file.
                            FILE_SYNCHRONOUS_IO_NONALERT, //Waits in the system to synchronize I/O queuing and completion are not subject to alerts //FILE_SYNCHRONOUS_IO_ALERT ?
                            NULL,
                            0L,
                            0 );

    if( ! NT_SUCCESS( status ) )
    {
        if( status == STATUS_SHARING_VIOLATION || status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            DEBUG_PRINT( "CB: DEBUG FltCreateFile %wZ failed status=%S\n", pNameInfo->Name, GetStatusString( status ) );
        }
        else
        {
            ERROR_PRINT( "\nCB: !!! ERROR FltCreateFile %wZ failed status=%S\n", pNameInfo->Name, GetStatusString( status ) );
        }
        goto Cleanup;
    }

    if( ! ioStatusCreate.Information == FILE_OPENED )
    {
        ERROR_PRINT( "\nCB: ioStatusCreate.Information != FILE_OPENED\n" );
        goto Cleanup;
    }

    if( hFile == INVALID_HANDLE_VALUE )
    {
        ERROR_PRINT( "\nCB: hFile == INVALID_HANDLE_VALUE\n" );
        goto Cleanup;
    }

    if( hFile == NULL )
    {
        ERROR_PRINT( "\nCB: hFile == NULL\n" );
        goto Cleanup;
    }

    //Get File attributes
    RtlZeroMemory( &fileInfo, sizeof(fileInfo) );
    status = ZwQueryInformationFile( hFile, &ioStatusQuery, &fileInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwQueryInformationFile failed status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

	if( fileInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    {
        DEBUG_PRINT( "CB: DEBUG FileAttributes: Skipping directory\n" );
        goto Cleanup;
    }

    status = ObReferenceObjectByHandle( hFile, KEY_ALL_ACCESS, *IoFileObjectType, KernelMode, &pSrcFileObject, NULL );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ObReferenceObjectByHandle(%p) failed status=%S\n", hFile, GetStatusString( status ) );
        goto Cleanup;
    }

	if( pSrcFileObject == NULL )
    {
        ERROR_PRINT( "\nCB: ERROR: pSrcFileObject == NULL\n" );
        goto Cleanup;
    }

    pSrcDeviceObject = IoGetRelatedDeviceObject( pSrcFileObject );
    if( pSrcDeviceObject == NULL )
    {
        ERROR_PRINT( "\nCB: ERROR: pDeviceObject == NULL\n" );
        goto Cleanup;
    }

    //DEBUG_PRINT( "CB: DEBUG: File: file.Flag=%S device.Flags=%S file.CompletionContext=%p device.AlignmentRequirement=%d\n", GetFileFlagString( pSrcFileObject->Flags ), GetDeviceFlagString( pSrcDeviceObject->Flags ), pSrcFileObject->CompletionContext, pSrcDeviceObject->AlignmentRequirement );

    if( ! FlagOn( pSrcFileObject->Flags, FO_SYNCHRONOUS_IO ) )
    {
        ERROR_PRINT( "\nCB: pSrcFileObject->Flags != FO_SYNCHRONOUS_IO\n" );
        goto Cleanup;
    }

	// Duplicate handle
    SourceHandle = hFile;
    TargetProcessHandle = UserProcessKernel;
    status = GetCurrentProcessKernelHandler( &SourceProcessHandle );
    if( ! NT_SUCCESS(status) )
    {
        status = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto Cleanup;
    }

    if( ObIsKernelHandle( SourceHandle ) )
    {
		// System process write
        DEBUG_PRINT( "CB: DEBUG SourceHandle handle is not user one: %p SourceProcessHandle=%p Process=%S\n", SourceHandle, SourceProcessHandle, pProcessName );
    }

    if( ! ObIsKernelHandle( SourceProcessHandle ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR SourceProcessHandle handle is not kernel one\n\n" );
        goto Cleanup;
    }

    if( ! ObIsKernelHandle( TargetProcessHandle ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR TargetProcessHandle handle is not kernel one\n\n" );
        goto Cleanup;
    }

    status = ZwDuplicateObject( SourceProcessHandle, SourceHandle, TargetProcessHandle, &TargetHandle, 0, 0, DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwDuplicateObject failed status=%S. SourceProcessHandle=%p SourceHandle=%p TargetProcessHandle=%p\n", GetStatusString( status ), SourceProcessHandle, SourceHandle, TargetProcessHandle );
        goto Cleanup;
    }

    if( ObIsKernelHandle( TargetHandle ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR TargetHandle handle is not user one: %p\n\n", TargetHandle );
        goto Cleanup;
    }

    DEBUG_PRINT( "CB: DEBUG ZwDuplicateObject SourceProcessHandle=%p TargetProcessHandle=%p SourceHandle=%p TargetHandle=%p\n", SourceProcessHandle, TargetProcessHandle, SourceHandle, TargetHandle );

#if METHOD_CLOSE_HANDLE_IN_DRIVER
	status = ReferenceHandleInProcess( TargetHandle, BackupProcess, &pDstFileObject );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ReferenceHandleInProcess(%p) failed status=%S\n", TargetHandle, GetStatusString( status, Buffer ) );
        goto Cleanup;
    }

	if( ! pDstFileObject )
	{
		DEBUG_PRINT( "CB: DEBUG pDstFileObject == NULL TargetHandle=%p File: %S Process: %S\n", TargetHandle, pStrFileName, pProcessName );
	}
#endif

    status = SendHandleToUser( TargetHandle, FltObjects->Volume, pNameInfo, &fileInfo, Delete, &OkToOpen );
    if( ! NT_SUCCESS( status ))
    {
        goto Cleanup;
    }

    // Write rejection support
    if( ! OkToOpen )
    {
        INFO_PRINT( "CB: INFO: Access denied by ceuser app %wZ", pNameInfo->Name );

        FltCancelFileOpen( FltObjects->Instance, FltObjects->FileObject );

        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;

        filterStatus = FLT_PREOP_COMPLETE;
    }

Cleanup:
    // Considering possible CloseHandle in ceuser.exe
#if METHOD_CLOSE_HANDLE_IN_DRIVER
    if( TargetHandle && BackupProcess )
	{
		if( status != STATUS_PORT_DISCONNECTED ) // We don't want BSOD
			CloseHandleInProcess( TargetHandle, BackupProcess );
	}

    if( pDstFileObject )
		ObDereferenceObject( pDstFileObject );
#endif

	if( pSrcFileObject )
        ObDereferenceObject( pSrcFileObject );

    if( hFile )
        FltClose( hFile ) ;

    if( SourceProcessHandle && SourceProcessHandle != NtCurrentProcess() )
        ZwClose( SourceProcessHandle );

    if( pInstanceContext )
        FltReleaseContext( pInstanceContext );

    if( pNameInfo )
        FltReleaseFileNameInformation( pNameInfo );

    size_t iLen = 0;
    RtlStringCchLengthW( pProcessName, dwRetLen, &iLen );
    if( iLen ) // ZwQueryInformationProcess returned process name
        DEBUG_PRINT( "CB: DEBUG OpenAndSend EXIT File: %S Process: %S\n", pStrFileName, pProcessName );

    if( pBufferProcessName )
        ExFreePoolWithTag( pBufferProcessName, 'kBeC' );

    return filterStatus;
}

NTSTATUS SendHandleToUser ( _In_ HANDLE hFile, _In_ PFLT_VOLUME Volume, _In_ PFLT_FILE_NAME_INFORMATION FltNameInfo, _In_ PFILE_BASIC_INFORMATION FileInfo, _In_ BOOLEAN DeleteOperation, _Out_ PBOOLEAN OkToOpen )
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
    pNotification->DeleteOperation = DeleteOperation;

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
