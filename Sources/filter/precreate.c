#include <fltKernel.h>
#include <ntstrsafe.h>
#include <dontuse.h>
#include <suppress.h>
#include "cebackup.h"
#include "drvcommon.h"
#include "util.h"

extern BACKUP_DATA g_CeBackupData;

FLT_PREOP_CALLBACK_STATUS PreCreate ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext )
{
    UNREFERENCED_PARAMETER( CompletionContext );
    FLT_PREOP_CALLBACK_STATUS filterStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    NTSTATUS status;
    HANDLE hFile = NULL;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    PCB_INSTANCE_CONTEXT pInstanceContext = NULL;
    PFILE_OBJECT pSrcFileObject = NULL;
    HANDLE UserProcessKernel = NULL;
    PEPROCESS BackupProcess = NULL;
	PEPROCESS RestoreProcess = NULL;
    HANDLE TargetHandle = NULL;
    HANDLE SourceProcessHandle = NULL;
    HANDLE TargetProcessHandle = NULL;
    WCHAR strFileName[MAX_PATH_SIZE] = {0};
    char BufferUniName[MAX_PATH_SIZE] = {0};
    WCHAR* strProcessName = L"";
#if METHOD_CLOSE_HANDLE_IN_DRIVER
    PFILE_OBJECT pDstFileObject = NULL;
#endif

    if( Data->Iopb->MajorFunction != IRP_MJ_CREATE )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    //  Directory opens don't need to be scanned
    if( FlagOn( Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE ) )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    BOOLEAN bIsDir = FALSE;
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

    if( Data->Iopb->TargetFileObject->FileName.Length == 0 )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;      

    if( ! Data->Iopb->Parameters.Create.SecurityContext )
    {
        ERROR_PRINT( "\nCB: ERROR Data->Iopb->Parameters.Create.SecurityContext is NULL\n" );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;      
    }

    // Skip if its opened for read
    if( ! FlagOn( Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_WRITE_DATA ) ) // FltObjects->FileObject->WriteAccess is not set in PreCreate yet!
        return FLT_PREOP_SUCCESS_NO_CALLBACK;      

    status = FltGetFileNameInformation( Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo );
    if( ! NT_SUCCESS( status ) )
    {
        DEBUG_PRINT( "\n!!! ERROR FltGetFileNameInformation failed. Status=%S\n\n", GetStatusString( status ) );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if( ! nameInfo )
    {
        ERROR_PRINT( "\nCB: ERROR nameInfo is NULL\n\n" );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    status = FltParseFileNameInformation( nameInfo );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\n!!! ERROR FltParseFileNameInformation failed. Status=%S\n\n", GetStatusString( status ) );
        goto Cleanup;
    }

    RtlStringCbCopyW( strFileName, sizeof(strFileName), nameInfo->FinalComponent.Buffer );

// TMP: FILTER OUT ONLY WRITE TO File 777.txt
/*
    UNICODE_STRING usStr777;
    RtlInitUnicodeString( &usStr777, L"777.txt" );
    if( ! RtlEqualUnicodeString( &nameInfo->FinalComponent, &usStr777, FALSE ) )
        goto Cleanup;
*/

    DWORD dwRetLen = 0;
	status = ZwQueryInformationProcess( NtCurrentProcess(), ProcessImageFileName, BufferUniName, MAX_PATH_SIZE, &dwRetLen );
    if( ! NT_SUCCESS(status) || ! dwRetLen )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwQueryInformationProcess ProcessImageFileName failed. status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    PUNICODE_STRING pUniName = (PUNICODE_STRING) BufferUniName;
	if( pUniName->Length == 0 || pUniName->Buffer == NULL )
    {
        DEBUG_PRINT( "\nCB: !!! pUniName->Length %d && pUniName->Buffer\n", pUniName->Length );
    }
    else
        strProcessName = pUniName->Buffer ;

    DEBUG_PRINT( "CB: DEBUG PreCreate ENTER with %S Process=%S\n", strFileName, strProcessName );

    MjCreatePrint( nameInfo, Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, Data->Iopb->Parameters.Create.ShareAccess, FltObjects->FileObject->Flags );
    //DbgBreakPoint();

    ExAcquireFastMutex( &g_CeBackupData.Guard ); //Going to APCL IRQL
    BackupProcess = g_CeBackupData.BackupProcess;
	RestoreProcess = g_CeBackupData.RestoreProcess;
    UserProcessKernel = g_CeBackupData.BackupProcessKernel;
    ExReleaseFastMutex( &g_CeBackupData.Guard );    

    //Skip restore.exe application operations
    if( BackupProcess && IoThreadToProcess( Data->Thread ) == BackupProcess )
    {
        DEBUG_PRINT( "CB: INFO: Skipping CEUSER write to %S\n", strFileName );
        goto Cleanup;
    }

    if( RestoreProcess && IoThreadToProcess( Data->Thread ) == RestoreProcess )
    {
        INFO_PRINT( "CB: INFO: Skipping RESTORE write to %S\n", strFileName );
        goto Cleanup;
    }

	if( BackupProcess == NULL )
    {
        INFO_PRINT( "CB: INFO: CEUSER application is not connected to filter. Skipping\n" );
        goto Cleanup;
    }

    OBJECT_ATTRIBUTES oa = {0};
    InitializeObjectAttributes( &oa, &nameInfo->Name, 0, NULL, NULL );

    status = FltGetInstanceContext( FltObjects->Instance, &pInstanceContext );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR FltGetInstanceContext failed status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    // Open file w/o Recursion
    IO_STATUS_BLOCK ioStatusCreate = {0};
    status = FltCreateFile( g_CeBackupData.Filter,
                            pInstanceContext->Instance,
                            &hFile,
                            GENERIC_READ | SYNCHRONIZE,
                            &oa,
                            &ioStatusCreate,
                            (PLARGE_INTEGER) NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ,
                            FILE_OPEN, // If the file already exists, open it instead of creating a new file. If it does not, fail the request and do not create a new file.
                            FILE_SYNCHRONOUS_IO_NONALERT, //Waits in the system to synchronize I/O queuing and completion are not subject to alerts //FILE_SYNCHRONOUS_IO_ALERT ?
                            NULL,
                            0L,
                            0 );

    if( ! NT_SUCCESS( status ) )
    {
        if( status == STATUS_SHARING_VIOLATION || status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            DEBUG_PRINT( "CB: DEBUG FltCreateFile %wZ failed status=%S\n", nameInfo->Name, GetStatusString( status ) );
        }
        else
        {
            ERROR_PRINT( "\nCB: !!! ERROR FltCreateFile %wZ failed status=%S\n", nameInfo->Name, GetStatusString( status ) );
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
	FILE_BASIC_INFORMATION basicInfo = {0};
    RtlZeroMemory( &basicInfo, sizeof(basicInfo) );
    IO_STATUS_BLOCK ioStatusQuery = {0};
    status = ZwQueryInformationFile( hFile, &ioStatusQuery, &basicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwQueryInformationFile failed status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

	if( basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
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

    PDEVICE_OBJECT pSrcDeviceObject = IoGetRelatedDeviceObject( pSrcFileObject );
    if( pSrcDeviceObject == NULL )
    {
        ERROR_PRINT( "\nCB: ERROR: pDeviceObject == NULL\n" );
        goto Cleanup;
    }

    DEBUG_PRINT( "CB: DEBUG: File: file.Flag=%S device.Flags=%S file.CompletionContext=%p device.AlignmentRequirement=%d\n", GetFileFlagString( pSrcFileObject->Flags ), GetDeviceFlagString( pSrcDeviceObject->Flags ), pSrcFileObject->CompletionContext, pSrcDeviceObject->AlignmentRequirement );

    if( ! FlagOn( pSrcFileObject->Flags, FO_SYNCHRONOUS_IO ) )
    {
        ERROR_PRINT( "\nCB: pSrcFileObject->Flags != FO_SYNCHRONOUS_IO\n" );
        goto Cleanup;
    }

	// Duplicate handle
    HANDLE SourceHandle = hFile;
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
        DEBUG_PRINT( "CB: DEBUG SourceHandle handle is not user one: %p SourceProcessHandle=%p Process=%S\n", SourceHandle, SourceProcessHandle, strProcessName );
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
        ERROR_PRINT( "\nCB: !!! ERROR ReferenceHandleInProcess(%p) failed status=%S\n", TargetHandle, GetStatusString( status ) );
        goto Cleanup;
    }

	if( ! pDstFileObject )
	{
		DEBUG_PRINT( "CB: DEBUG pDstFileObject == NULL TargetHandle=%p File: %S Process: %S\n", TargetHandle, strFileName, strProcessName );
	}
#endif

	BOOLEAN OkToOpen = TRUE;
    status = SendHandleToUser( TargetHandle, FltObjects->Volume, &nameInfo->ParentDir, &nameInfo->FinalComponent, basicInfo.CreationTime, basicInfo.LastAccessTime, basicInfo.LastWriteTime, basicInfo.ChangeTime, basicInfo.FileAttributes, &OkToOpen );
    if( ! NT_SUCCESS( status ))
    {
        goto Cleanup;
    }

    // Write rejection support
    if( ! OkToOpen )
    {
        INFO_PRINT( "CB: INFO: Access denied by ceuser app %wZ", nameInfo->Name );

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
		//TODO: Consider closing in CEUSER process ...
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

    if( nameInfo )
        FltReleaseFileNameInformation( nameInfo );

    size_t iLen = 0;
    RtlStringCchLengthW( strProcessName, MAX_PATH_SIZE, &iLen );
    if( iLen ) // ZwQueryInformationProcess returned process name
        DEBUG_PRINT( "CB: DEBUG PreCreate EXIT File: %S Process: %S\n", strFileName, strProcessName );

    return filterStatus;
}
