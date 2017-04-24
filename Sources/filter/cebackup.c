//TODO: Load at boot time

#include <fltKernel.h>
#include <ntstrsafe.h>
#include <dontuse.h>
#include <suppress.h>
#include "cebackup.h"
#include "cecommon.h"
#include "util.h"

// Assign text sections for each routine.
#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, CbUnload)
    #pragma alloc_text(PAGE, CbInstanceQueryTeardown)
    #pragma alloc_text(PAGE, CbInstanceSetup)
    #pragma alloc_text(PAGE, CbInstanceTeardownStart)
    #pragma alloc_text(PAGE, CbInstanceTeardownComplete)
    #pragma alloc_text(PAGE, BackupPortConnect)
    #pragma alloc_text(PAGE, BackupPortDisconnect)
#endif

// Operation registration
CONST FLT_OPERATION_REGISTRATION Callbacks[] = 
{
    { IRP_MJ_CREATE,
      0,
      PreCreate,
      PostOperation },
/*
    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_CLOSE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_READ,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_WRITE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_SET_EA,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      PreOperationNo,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_CLEANUP,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_PNP,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_MDL_READ,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      PreCreate,
      PostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      PreCreate,
      PostOperation },
*/
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
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uniPortName;
    PSECURITY_DESCRIPTOR sd;

    DEBUG_PRINT( "CB: INFO DriverEntry\n" );

    RtlZeroMemory( &g_CeBackupData, sizeof(g_CeBackupData) );

    //  Register with FltMgr to tell it our callback routines
    status = FltRegisterFilter( DriverObject, &FilterRegistration, &g_CeBackupData.Filter );
	FLT_ASSERT( NT_SUCCESS( status ) );
    if( ! NT_SUCCESS( status ) )
	    goto Cleanup;

    //  Create a communication port.
    RtlInitUnicodeString( &uniPortName, BACKUP_PORT_NAME );

    //  We secure the port so only ADMINs & SYSTEM can access it.
    status = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );
    if( ! NT_SUCCESS( status ) )
        goto Cleanup;

    InitializeObjectAttributes( &oa, &uniPortName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, sd );

    status = FltCreateCommunicationPort( g_CeBackupData.Filter, &g_CeBackupData.ServerPort, &oa, NULL, BackupPortConnect, BackupPortDisconnect, NULL, 1 );

    //  Free the security descriptor in all cases. It is not needed once the call to FltCreateCommunicationPort() is made.
    FltFreeSecurityDescriptor( sd );

    if( ! NT_SUCCESS( status ) )
        goto Cleanup;

    status = FltStartFiltering( g_CeBackupData.Filter );
    if( ! NT_SUCCESS( status ) )
        goto Cleanup;

    return status;

Cleanup:
    if( g_CeBackupData.Filter )
	{
        FltUnregisterFilter( g_CeBackupData.Filter );
        g_CeBackupData.Filter;
    }

    if( g_CeBackupData.ServerPort )
    {
        FltCloseCommunicationPort( g_CeBackupData.ServerPort );
        g_CeBackupData.ServerPort = NULL;
    }

    return status;
}

NTSTATUS CbUnload ( _In_ FLT_FILTER_UNLOAD_FLAGS Flags )
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DEBUG_PRINT( "CB: INFO CbUnload\n" );

    FltCloseCommunicationPort( g_CeBackupData.ServerPort );

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
    PFILE_OBJECT pFileObject = NULL;
    HANDLE TargetHandle = NULL;
    HANDLE SourceProcessHandle = NULL ;
    WCHAR strFileName[MAX_PATH_SIZE] = {0};
    char Buffer[MAX_PATH_SIZE] = {0};
    WCHAR* strProcessName = L"";

    if( Data->Iopb->MajorFunction != IRP_MJ_CREATE )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    //  Directory opens don't need to be scanned
    if( FlagOn( Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE ) )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    BOOLEAN isDir = FALSE;
    status = FltIsDirectory( FltObjects->FileObject, FltObjects->Instance, &isDir );
    if( NT_SUCCESS( status ) && isDir )
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
    if( ! FlagOn( Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_WRITE_DATA ) )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;      

    status = FltGetFileNameInformation( Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo );
    if( ! NT_SUCCESS( status ) )
    {
        //DEBUG_PRINT( "\n!!! ERROR FltGetFileNameInformation failed. Status=%S\n\n", GetStatusString( status ) );
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

// TMP: FILTER OUT ONLY ONE OPERATION FOR NOW / File 777.txt
    UNICODE_STRING usStr777;
    RtlInitUnicodeString( &usStr777, L"777.txt" );
    if( ! RtlEqualUnicodeString( &nameInfo->FinalComponent, &usStr777, FALSE ) )
        goto Cleanup;
//

    DWORD dwRetLen = 0;
	status = ZwQueryInformationProcess( NtCurrentProcess(), ProcessImageFileName, Buffer, MAX_PATH_SIZE, &dwRetLen );
    if( ! NT_SUCCESS(status) || ! dwRetLen )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwQueryInformationProcess ProcessImageFileName failed. status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    PUNICODE_STRING pUniName = (PUNICODE_STRING) Buffer;
	if( pUniName->Length && pUniName->Buffer )
        strProcessName = pUniName->Buffer ;

    ERROR_PRINT( "CB: DEBUG PreCreate ENTER with %S Process=%S\n", strFileName, strProcessName );

    //MjCreatePrint( nameInfo, Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, Data->Iopb->Parameters.Create.ShareAccess, FltObjects->FileObject->Flags );
    //DbgBreakPoint();

    //Skip admin user tool operations
    if( g_CeBackupData.UserProcess && IoThreadToProcess( Data->Thread ) == g_CeBackupData.UserProcess )
    {
        DEBUG_PRINT( "CB: INFO: Skipping CEUSER OP\n" );
        goto Cleanup;
    }

    if( g_CeBackupData.UserProcessKernel == NULL )
    {
        ERROR_PRINT( "CB: WARNING: CEUSER application is not connected to filter\n" );
        goto Cleanup;
    }

    if( ! ObIsKernelHandle( g_CeBackupData.UserProcessKernel ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR CEUSER application handle is not kernel one\n\n" );
        goto Cleanup;
    }

//    if( ! FltObjects->FileObject->WriteAccess ) // Not set in PreCreate yet!
//        goto Cleanup;

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
                            FILE_SYNCHRONOUS_IO_NONALERT, //Waits in the system to synchronize I/O queuing and completion are not subject to alerts //TODO: FILE_SYNCHRONOUS_IO_ALERT,
                            NULL,
                            0L,
                            0 );

    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR FltCreateFile failed status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    ASSERT_BOOL_PRINT( ioStatusCreate.Information == FILE_OPENED );

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
    FILE_BASIC_INFORMATION basicInfo;
    RtlZeroMemory( &basicInfo, sizeof(basicInfo) );
    IO_STATUS_BLOCK ioStatusQuery = {0};
    status = ZwQueryInformationFile( hFile, &ioStatusQuery, &basicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwQueryInformationFile failed status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    // TODO ??? Check FileClose in ceuser.exe

    status = ObReferenceObjectByHandle( hFile, KEY_ALL_ACCESS, *IoFileObjectType, KernelMode, &pFileObject, NULL );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ObReferenceObjectByHandle(%p) failed status=%S\n", hFile, GetStatusString( status ) );
        goto Cleanup;
    }

    PDEVICE_OBJECT pDeviceObject = IoGetRelatedDeviceObject( pFileObject );
    ASSERT_BOOL_PRINT( pDeviceObject != NULL );
    //DEBUG_PRINT( "CB: DEBUG: File: file.Flag=%S device.Flags=%S file.CompletionContext=%p device.AlignmentRequirement=%d\n", GetFileFlagString( pFileObject->Flags ), GetDeviceFlagString( pDeviceObject->Flags ), pFileObject->CompletionContext, pDeviceObject->AlignmentRequirement );

    ASSERT_BOOL_PRINT( FlagOn( pFileObject->Flags, FO_SYNCHRONOUS_IO ) );

    // Duplicate handle
    HANDLE TargetProcessHandle = g_CeBackupData.UserProcessKernel;
    SourceProcessHandle = NtCurrentProcess();
    HANDLE SourceHandle = hFile;

    status = ZwDuplicateObject( SourceProcessHandle, SourceHandle, TargetProcessHandle, &TargetHandle, 0, 0, DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwDuplicateObject failed status=%S. SourceProcessHandle=%p SourceHandle=%p TargetProcessHandle=%p\n", GetStatusString( status ), SourceProcessHandle, SourceHandle, TargetProcessHandle );
        goto Cleanup;
    }

    ASSERT_BOOL_PRINT( ! ObIsKernelHandle( TargetHandle ) );

    ERROR_PRINT( "CB: DEBUG ZwDuplicateObject SourceProcessHandle=%p TargetProcessHandle=%p SourceHandle=%p TargetHandle=%p\n", SourceProcessHandle, TargetProcessHandle, SourceHandle, TargetHandle );

    BOOLEAN OkToOpen = TRUE;
    status = SendHandleToUser( TargetHandle, FltObjects->Volume, &nameInfo->ParentDir, &nameInfo->FinalComponent, basicInfo.CreationTime, basicInfo.LastAccessTime, basicInfo.LastWriteTime, basicInfo.ChangeTime, basicInfo.FileAttributes, &OkToOpen );
    if( ! NT_SUCCESS( status ))
    {
        ERROR_PRINT( "\nCB: !!! ERROR SendHandleToUser failed status=%S\n", GetStatusString( status ) );
        goto Cleanup;
    }

    if( ! OkToOpen )
    {
        DEBUG_PRINT( "CB: INFO: Access denied by admin app %wZ", nameInfo->Name );

        FltCancelFileOpen( FltObjects->Instance, FltObjects->FileObject );

        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;

        filterStatus = FLT_PREOP_COMPLETE;
    }

Cleanup:
//    if( TargetHandle && g_CeBackupData.UserProcessKernel )
//        CloseHandleInProcess( TargetHandle, g_CeBackupData.UserProcessKernel );

    if( pFileObject )
        ObDereferenceObject( pFileObject );

    if( SourceProcessHandle && SourceProcessHandle != NtCurrentProcess() )
        ZwClose( SourceProcessHandle );

    if( hFile )
        FltClose( hFile ) ;

    if( pInstanceContext )
        FltReleaseContext( pInstanceContext );

    if( nameInfo )
        FltReleaseFileNameInformation( nameInfo );

    ERROR_PRINT( "CB: DEBUG PreCreate EXIT %S Process=%S\n", strFileName, strProcessName );

    return filterStatus;
}

NTSTATUS BackupPortConnect ( _In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionCookie )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie = NULL );

    FLT_ASSERT( g_CeBackupData.ClientPort == NULL );
    FLT_ASSERT( g_CeBackupData.UserProcess == NULL );
    FLT_ASSERT( g_CeBackupData.UserProcessKernel == NULL );

    //  Set the user process and port. In a production filter it may be necessary to synchronize access to such fields with port lifetime. For instance, while filter manager will synchronize
    //  FltCloseClientPort with FltSendMessage's reading of the port handle, synchronizing access to the UserProcess would be up to the filter.

    HANDLE hProcess = NULL;
    NTSTATUS status = GetCurrentProcessKernelHandler( &hProcess );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwOpenProcess failed. status=%S\n", GetStatusString( status ) );
        return status;
    }

    g_CeBackupData.UserProcessKernel = hProcess;
    g_CeBackupData.UserProcess = PsGetCurrentProcess();
    g_CeBackupData.UserProcessId = PsGetCurrentProcessId();
    g_CeBackupData.ClientPort = ClientPort;

    DEBUG_PRINT( "CB: DEBUG BackupPortConnect: port=0x%p PsGetCurrentProcess()=%p ZwOpenProcess=%p\n", ClientPort, PsGetCurrentProcess(), hProcess );

    //DbgBreakPoint();

    return STATUS_SUCCESS;
}

VOID BackupPortDisconnect( _In_opt_ PVOID ConnectionCookie )
{
    UNREFERENCED_PARAMETER( ConnectionCookie );

    PAGED_CODE();

    DEBUG_PRINT( "CB: DEBUG BackupPortDisconnect: port=0x%p\n", g_CeBackupData.ClientPort );

    //  Close our handle to the connection: note, since we limited max connections to 1, another connect will not be allowed until we return from the disconnect routine.
    FltCloseClientPort( g_CeBackupData.Filter, &g_CeBackupData.ClientPort );

    //  Reset the user-process field.
    g_CeBackupData.UserProcess = NULL;
    if( g_CeBackupData.UserProcessKernel )
        ZwClose( g_CeBackupData.UserProcessKernel );
    g_CeBackupData.UserProcessKernel = NULL;
    g_CeBackupData.UserProcessId = NULL;
}

NTSTATUS SendHandleToUser ( _In_ HANDLE hFile, _In_ PFLT_VOLUME Volume, _In_ PCUNICODE_STRING ParentDir, _In_ PCUNICODE_STRING FileName, _In_ LARGE_INTEGER CreationTime, _In_ LARGE_INTEGER LastAccessTime, _In_ LARGE_INTEGER LastWriteTime, _In_ LARGE_INTEGER ChangeTime, _In_ ULONG FileAttributes, _Out_ PBOOLEAN OkToOpen )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBACKUP_NOTIFICATION pNotification = NULL;
    ULONG replyLength = 0;

    *OkToOpen = TRUE;

    if( g_CeBackupData.ClientPort == NULL )
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
        ERROR_PRINT( "\nCB: !!! Result Path is too big\n" );
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

    status = FltSendMessage( g_CeBackupData.Filter, &g_CeBackupData.ClientPort, pNotification, sizeof(BACKUP_NOTIFICATION), pNotification, &replyLength, NULL );
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
