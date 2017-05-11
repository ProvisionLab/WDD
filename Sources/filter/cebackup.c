//TODO: Load at boot time

#include <fltKernel.h>
#include <ntstrsafe.h>
#include <dontuse.h>
#include <suppress.h>
#include "cebackup.h"
#include "drvcommon.h"
#include "util.h"

#define METHOD_CLOSE_HANDLE_IN_DRIVER 0

// Assign text sections for each routine.
#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, CbUnload)
    #pragma alloc_text(PAGE, CbInstanceQueryTeardown)
    #pragma alloc_text(PAGE, CbInstanceSetup)
    #pragma alloc_text(PAGE, CbInstanceTeardownStart)
    #pragma alloc_text(PAGE, CbInstanceTeardownComplete)
//    #pragma alloc_text(PAGE, BackupPortConnect)
//    #pragma alloc_text(PAGE, BackupPortDisconnect)
#endif

#pragma data_seg("NONPAGE")
BACKUP_DATA g_CeBackupData;
#pragma data_seg()

// Operation registration
CONST FLT_OPERATION_REGISTRATION Callbacks[] = 
{
    { IRP_MJ_CREATE,
      0,
      PreCreate,
      PostOperation },
    { IRP_MJ_OPERATION_END }
};

const FLT_CONTEXT_REGISTRATION ContextRegistration[] =
{
    { FLT_INSTANCE_CONTEXT,
      0,
      //CbContextCleanup, Unless we have some MUTEX to free
      NULL,
      CB_INSTANCE_CONTEXT_SIZE,
      CB_INSTANCE_CONTEXT_TAG },

    { FLT_CONTEXT_END }
};

//  This defines what we want to filter with FltMgr
CONST FLT_REGISTRATION FilterRegistration = 
{
    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    ContextRegistration,                //  Context
    Callbacks,                          //  Operation callbacks

    CbUnload,                           //  MiniFilterUnload

    CbInstanceSetup,                    //  InstanceSetup
    CbInstanceQueryTeardown,            //  InstanceQueryTeardown
    CbInstanceTeardownStart,            //  InstanceTeardownStart
    CbInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent
};

// This routine is called whenever a new instance is created on a volume. This gives us a chance to decide if we need to attach to this volume or not.
// If this routine is not defined in the registration structure, automatic instances are always created.
NTSTATUS CbInstanceSetup ( _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isWritable = FALSE;

    DEBUG_PRINT( "CB: DEBUG CbInstanceSetup: Entered VolumeFilesystemType=0x%X VolumeDeviceType=0x%X\n", VolumeFilesystemType, VolumeDeviceType );

    //We got VolumeFilesystemType: FLT_FSTYPE_MUP / FLT_FSTYPE_RDPDR, FLT_FSTYPE_INCD and FLT_FSTYPE_NTFS / FLT_FSTYPE_RDPDR

    //  Don't attach to network volumes.
    if( VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM )
    {
        DEBUG_PRINT( "CB: DEBUG CbInstanceSetup: VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM. Skipping\n" );
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    status = FltIsVolumeWritable( FltObjects->Volume, &isWritable );
    if( ! NT_SUCCESS( status ) )
    {
        DEBUG_PRINT( "CB: DEBUG CbInstanceSetup: FltIsVolumeWritable failed. Skipping\n" );
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //  Attaching to read-only volumes is pointless as you should not be able to delete files on such a volume.
    if( ! isWritable )
    {
        DEBUG_PRINT( "CB: DEBUG CbInstanceSetup: ! FltIsVolumeWritable volume. Skipping\n" );
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    switch( VolumeFilesystemType )
    {
        case FLT_FSTYPE_NTFS:
        case FLT_FSTYPE_REFS:
            status = STATUS_SUCCESS;
            break;
        default:
        {
            DEBUG_PRINT( "CB: DEBUG CbInstanceSetup: VolumeFilesystemType is not FLT_FSTYPE_NTFS or FLT_FSTYPE_REFS. Skipping\n" );
            return STATUS_FLT_DO_NOT_ATTACH;
        }
    }

    PCB_INSTANCE_CONTEXT instanceContext = NULL;
    status = FltAllocateContext( FltObjects->Filter, FLT_INSTANCE_CONTEXT, CB_INSTANCE_CONTEXT_SIZE, NonPagedPool, &instanceContext );
    if( ! NT_SUCCESS( status ))
    {
        ERROR_PRINT( "\nCB: !!! ERROR CbInstanceSetup: FltAllocateContext failed status=%S\n", GetStatusString( status ) );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if( instanceContext == NULL )
    {
        ERROR_PRINT( "\nCB: !!! ERROR CbInstanceSetup: instanceContext == NULL\n" );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    RtlZeroMemory( instanceContext, CB_INSTANCE_CONTEXT_SIZE );

    instanceContext->Instance = FltObjects->Instance;
    instanceContext->FilesystemType = VolumeFilesystemType;
    instanceContext->Volume = FltObjects->Volume;

    status = FltSetInstanceContext( FltObjects->Instance, FLT_SET_CONTEXT_KEEP_IF_EXISTS, instanceContext, NULL );

    FltReleaseContext( instanceContext );

    if( ! NT_SUCCESS( status ))
    {
        ERROR_PRINT( "\nCB: !!! ERROR FltSetInstanceContext failed status=%S\n", GetStatusString( status ) );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    return status;
}

// This is called when an instance is being manually deleted by a call to FltDetachVolume or FilterDetach thereby giving us a chance to fail that detach request.
// If this routine is not defined in the registration structure, explicit detach requests via FltDetachVolume or FilterDetach will always be failed.
NTSTATUS CbInstanceQueryTeardown ( _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DEBUG_PRINT( "CB: DEBUG CbInstanceQueryTeardown\n" );

    return STATUS_SUCCESS;
}

// This routine is called at the start of instance teardown.
VOID CbInstanceTeardownStart ( _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DEBUG_PRINT( "CB: DEBUG CbInstanceTeardownStart\n" );
}

// This routine is called at the end of instance teardown.
VOID CbInstanceTeardownComplete ( _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DEBUG_PRINT( "CB: DEBUG CbInstanceTeardownComplete\n" );
}

/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/
NTSTATUS DriverEntry ( _In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath )
{
    UNREFERENCED_PARAMETER( RegistryPath );
    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES oaBackup, oaRestore;
    UNICODE_STRING uniBackupPortName, uniRestorePortName;
    PSECURITY_DESCRIPTOR sd;

    INFO_PRINT( "CB: INFO DriverEntry\n" );

    RtlZeroMemory( &g_CeBackupData, sizeof(g_CeBackupData) );

    //  Register with FltMgr to tell it our callback routines
    status = FltRegisterFilter( DriverObject, &FilterRegistration, &g_CeBackupData.Filter );
	FLT_ASSERT( NT_SUCCESS( status ) );
    if( ! NT_SUCCESS( status ) )
	    goto Cleanup;

    //  Create a communication port.
    RtlInitUnicodeString( &uniBackupPortName, BACKUP_PORT_NAME );
	RtlInitUnicodeString( &uniRestorePortName, RESTORE_PORT_NAME );

    //  We secure the port so only ADMINs & SYSTEM can access it.
    status = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );
    if( ! NT_SUCCESS( status ) )
        goto Cleanup;

    InitializeObjectAttributes( &oaBackup, &uniBackupPortName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, sd );
	InitializeObjectAttributes( &oaRestore, &uniRestorePortName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, sd );

    status = FltCreateCommunicationPort( g_CeBackupData.Filter, &g_CeBackupData.BackupPort,  &oaBackup,  NULL, BackupPortConnect,  BackupPortDisconnect,  NULL, 1 );

	status = FltCreateCommunicationPort( g_CeBackupData.Filter, &g_CeBackupData.RestorePort, &oaRestore, NULL, RestorePortConnect, RestorePortDisconnect, NULL, 1 );

	//  Free the security descriptor in all cases. It is not needed once the call to FltCreateCommunicationPort() is made.
    FltFreeSecurityDescriptor( sd );
    if( ! NT_SUCCESS( status ) )
        goto Cleanup;

    status = FltStartFiltering( g_CeBackupData.Filter );
    if( ! NT_SUCCESS( status ) )
        goto Cleanup;

    ExInitializeFastMutex( &g_CeBackupData.Guard );

    return status;

Cleanup:
    if( g_CeBackupData.Filter )
	{
        FltUnregisterFilter( g_CeBackupData.Filter );
        g_CeBackupData.Filter;
    }

    if( g_CeBackupData.BackupPort )
    {
        FltCloseCommunicationPort( g_CeBackupData.BackupPort );
        g_CeBackupData.BackupPort = NULL;
    }

    if( g_CeBackupData.RestorePort )
    {
        FltCloseCommunicationPort( g_CeBackupData.RestorePort );
        g_CeBackupData.RestorePort = NULL;
    }

	return status;
}

NTSTATUS CbUnload ( _In_ FLT_FILTER_UNLOAD_FLAGS Flags )
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    INFO_PRINT( "CB: INFO CbUnload\n" );

    FltCloseCommunicationPort( g_CeBackupData.BackupPort );
	FltCloseCommunicationPort( g_CeBackupData.RestorePort );

    FltUnregisterFilter( g_CeBackupData.Filter );

    return STATUS_SUCCESS;
}

FLT_POSTOP_CALLBACK_STATUS PostOperation ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags )
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    //DEBUG_PRINT( "CeBackup!PostOperation\n" );

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS PreOperationNo ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext )
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    DEBUG_PRINT( "CB: DEBUG PreOperationNo\n" );

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

VOID CbContextCleanup ( _In_ PFLT_CONTEXT Context, _In_ FLT_CONTEXT_TYPE ContextType )
{
    PCB_INSTANCE_CONTEXT instanceContext;

    PAGED_CODE();

    DEBUG_PRINT( "CB: DEBUG CbContextCleanup\n" );

    switch( ContextType )
    {
    case FLT_INSTANCE_CONTEXT:
        instanceContext = Context;
        DEBUG_PRINT( "CB: DEBUG Cleaning up instance context for volume (Context = %p)\n", instanceContext );
        //ExDeleteResourceLite( &instanceContext->MetadataResource );
        break;
    }
}

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

    ExAcquireFastMutex( &g_CeBackupData.Guard ); //APCL IRQL
    BackupProcess = g_CeBackupData.BackupProcess;
	RestoreProcess = g_CeBackupData.RestoreProcess;
    UserProcessKernel = g_CeBackupData.BackupProcessKernel;
    ExReleaseFastMutex( &g_CeBackupData.Guard );    

    //Skip admin user tool operations
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
        INFO_PRINT( "CB: INFO: CEUSER application is not connected to filter\n" );
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

NTSTATUS BackupPortConnect ( _In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionCookie )
{
    //PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie = NULL );

    FLT_ASSERT( g_CeBackupData.ClientBackupPort == NULL );
    FLT_ASSERT( g_CeBackupData.BackupProcess == NULL );
    FLT_ASSERT( g_CeBackupData.BackupProcessKernel == NULL );

	//  Set the user process and port. In a production filter it may be necessary to synchronize access to such fields with port lifetime. For instance, while filter manager will synchronize
    //  FltCloseClientPort with FltSendMessage's reading of the port handle, synchronizing access to the UserProcess would be up to the filter.

    HANDLE hProcess = NULL;
    NTSTATUS status = GetCurrentProcessKernelHandler( &hProcess );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwOpenProcess failed. status=%S\n", GetStatusString( status ) );
        return status;
    }

    ExAcquireFastMutex( &g_CeBackupData.Guard );
    g_CeBackupData.BackupProcessKernel = hProcess;
    g_CeBackupData.BackupProcess = PsGetCurrentProcess();
    g_CeBackupData.UserProcessId = PsGetCurrentProcessId();
    g_CeBackupData.ClientBackupPort = ClientPort;
    ExReleaseFastMutex( &g_CeBackupData.Guard );

	//DbgBreakPoint();
    INFO_PRINT( "CB: INFO BackupPortConnect: port=0x%p PsGetCurrentProcess()=%p ZwOpenProcess=%p\n", ClientPort, PsGetCurrentProcess(), hProcess );

    return STATUS_SUCCESS;
}

VOID BackupPortDisconnect( _In_opt_ PVOID ConnectionCookie )
{
    UNREFERENCED_PARAMETER( ConnectionCookie );

    //PAGED_CODE();

    INFO_PRINT( "CB: INFO BackupPortDisconnect: port=0x%p ENTER\n", g_CeBackupData.ClientBackupPort );

	//  Close our handle to the connection: note, since we limited max connections to 1, another connect will not be allowed until we return from the disconnect routine.
    FltCloseClientPort( g_CeBackupData.Filter, &g_CeBackupData.ClientBackupPort );

    //  Reset the user-process field.
	HANDLE BackupProcessKernel = NULL;
    ExAcquireFastMutex( &g_CeBackupData.Guard );
	g_CeBackupData.ClientBackupPort = NULL;
    g_CeBackupData.BackupProcess = NULL;
	BackupProcessKernel = g_CeBackupData.BackupProcessKernel;
    g_CeBackupData.BackupProcessKernel = NULL;
    g_CeBackupData.UserProcessId = NULL;
    ExReleaseFastMutex( &g_CeBackupData.Guard );

	if( BackupProcessKernel )
        ZwClose( BackupProcessKernel );
}

NTSTATUS RestorePortConnect ( _In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionCookie )
{
    //PAGED_CODE();
    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie = NULL );

    FLT_ASSERT( g_CeBackupData.ClientRestorePort == NULL );
    FLT_ASSERT( g_CeBackupData.RestoreProcess == NULL );

    ExAcquireFastMutex( &g_CeBackupData.Guard );
    g_CeBackupData.RestoreProcess = PsGetCurrentProcess();
    g_CeBackupData.ClientRestorePort = ClientPort;
	ExReleaseFastMutex( &g_CeBackupData.Guard );

    INFO_PRINT( "CB: INFO RestorePortConnect: port=0x%p PsGetCurrentProcess()=%p\n", ClientPort, PsGetCurrentProcess() );

    return STATUS_SUCCESS;
}

VOID RestorePortDisconnect( _In_opt_ PVOID ConnectionCookie )
{
    //PAGED_CODE();
    UNREFERENCED_PARAMETER( ConnectionCookie );

    INFO_PRINT( "CB: INFO RestorePortDisconnect: port=0x%p ENTER\n", g_CeBackupData.ClientRestorePort );

    FltCloseClientPort( g_CeBackupData.Filter, &g_CeBackupData.ClientRestorePort );

	ExAcquireFastMutex( &g_CeBackupData.Guard );
	g_CeBackupData.ClientRestorePort = NULL;
    g_CeBackupData.RestoreProcess = NULL;
	ExReleaseFastMutex( &g_CeBackupData.Guard );
}

NTSTATUS SendHandleToUser ( _In_ HANDLE hFile, _In_ PFLT_VOLUME Volume, _In_ PCUNICODE_STRING ParentDir, _In_ PCUNICODE_STRING FileName, _In_ LARGE_INTEGER CreationTime, _In_ LARGE_INTEGER LastAccessTime, _In_ LARGE_INTEGER LastWriteTime, _In_ LARGE_INTEGER ChangeTime, _In_ ULONG FileAttributes, _Out_ PBOOLEAN OkToOpen )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBACKUP_NOTIFICATION pNotification = NULL;
    ULONG replyLength = 0;

    *OkToOpen = TRUE;

    if( g_CeBackupData.ClientBackupPort == NULL )
        return STATUS_PORT_DISCONNECTED;

    //Getting Disk Letter:
    PDEVICE_OBJECT devObj = NULL;
    status = FltGetDiskDeviceObject( Volume, &devObj );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR FltGetDiskDeviceObject failed. status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    DECLARE_UNICODE_STRING_SIZE( uniDisk, MAX_PATH_SIZE );
    status = IoVolumeDeviceToDosName( devObj, &uniDisk );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR IoVolumeDeviceToDosName failed. status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    if( uniDisk.Length + ParentDir->Length + FileName->Length + 1 > MAX_PATH_SIZE )
    {
        ERROR_PRINT( "\nCB: !!! ERROR Result Path is too big\n" );
        goto Cleanup;
    }

    DECLARE_UNICODE_STRING_SIZE( uniPath, MAX_PATH_SIZE );
    RtlUnicodeStringPrintf( &uniPath, L"%wZ%wZ%wZ", uniDisk, ParentDir, FileName ) ;

    DEBUG_PRINT( "CB: INFO Sending message Path=%wZ HANDLE=0x%p\n", uniPath, hFile );

    pNotification = ExAllocatePoolWithTag( NonPagedPool, sizeof( BACKUP_NOTIFICATION ), 'kBeC' );

    if( ! pNotification )
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory( pNotification, sizeof(*pNotification) );
    pNotification->hFile = hFile;
    RtlCopyMemory( &pNotification->Path, uniPath.Buffer, uniPath.Length + 1 );
    pNotification->CreationTime = CreationTime;
    pNotification->LastAccessTime = LastAccessTime;
    pNotification->LastWriteTime = LastWriteTime;
    pNotification->ChangeTime = ChangeTime;
    pNotification->FileAttributes = FileAttributes;

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
        ExFreePoolWithTag( pNotification, 'nacS' );

    return status;
}
