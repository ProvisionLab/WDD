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
    #pragma alloc_text(PAGE, BackupPortConnect)
    #pragma alloc_text(PAGE, BackupPortMessage)
    #pragma alloc_text(PAGE, BackupPortDisconnect)
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
    { IRP_MJ_SET_INFORMATION,
      0,
      PreSetInformation,
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
    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES oaBackup, oaRestore;
    UNICODE_STRING uniBackupPortName, uniRestorePortName;
    PSECURITY_DESCRIPTOR sd;

    //DbgBreakPoint();
    INFO_PRINT( "CB: INFO DriverEntry\n" );

    RtlZeroMemory( &g_CeBackupData, sizeof(g_CeBackupData) );
    ExInitializeFastMutex( &g_CeBackupData.Guard );

    g_CeBackupData.RegistryPath.Buffer = ExAllocatePoolWithTag( NonPagedPool, RegistryPath->Length, 'kBeC' );
    g_CeBackupData.RegistryPath.Length = 0;
    g_CeBackupData.RegistryPath.MaximumLength = RegistryPath->Length;
    RtlCopyUnicodeString( &g_CeBackupData.RegistryPath, RegistryPath );
    g_CeBackupData.RegistryPath.Length = RegistryPath->Length;

    ReadDestination( RegistryPath );

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

    status = FltCreateCommunicationPort( g_CeBackupData.Filter, &g_CeBackupData.BackupPort,  &oaBackup,  NULL, BackupPortConnect,  BackupPortDisconnect,  BackupPortMessage, 1 );

    status = FltCreateCommunicationPort( g_CeBackupData.Filter, &g_CeBackupData.RestorePort, &oaRestore, NULL, RestorePortConnect, RestorePortDisconnect, NULL, 1 );

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
    ExFreePoolWithTag( g_CeBackupData.RegistryPath.Buffer, 'kBeC' );
    g_CeBackupData.RegistryPath.Length = 0;

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

FLT_PREOP_CALLBACK_STATUS PreOperationNo ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID* CompletionContext )
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

NTSTATUS BackupPortConnect ( _In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID* ConnectionCookie )
{
    PAGED_CODE();

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

    PAGED_CODE();

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

NTSTATUS RestorePortConnect ( _In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID* ConnectionCookie )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext );
    UNREFERENCED_PARAMETER( ConnectionCookie );

    FLT_ASSERT( g_CeBackupData.ClientRestorePort == NULL );
    FLT_ASSERT( g_CeBackupData.RestoreProcess == NULL );

    ExAcquireFastMutex( &g_CeBackupData.Guard );
    g_CeBackupData.RestoreProcess = PsGetCurrentProcess();
    g_CeBackupData.ClientRestorePort = ClientPort;
	ExReleaseFastMutex( &g_CeBackupData.Guard );

    INFO_PRINT( "CB: INFO RestorePortConnect: port=0x%p PsGetCurrentProcess()=%p\n", ClientPort, PsGetCurrentProcess() );

    return STATUS_SUCCESS;
}

NTSTATUS BackupPortMessage ( _In_opt_ PVOID PortCookie, _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength, _Out_writes_bytes_to_opt_(OutputBufferLength,*ReturnOutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength, _Out_ PULONG ReturnOutputBufferLength )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER( PortCookie );
    UNREFERENCED_PARAMETER( InputBuffer );
    UNREFERENCED_PARAMETER( InputBufferLength );
    UNREFERENCED_PARAMETER( OutputBuffer );
    UNREFERENCED_PARAMETER( OutputBufferLength );
    UNREFERENCED_PARAMETER( ReturnOutputBufferLength );
    E_BACKUP_COMMAND command = 0;
    WCHAR* strSrcPath = NULL, *strDstPath = NULL;

    INFO_PRINT( "CB: INFO BackupPortMessage: InputBufferLength=0x%d\n", InputBufferLength );

    if( InputBuffer == NULL || InputBufferLength < sizeof(BACKUP_COMMAND_MESSAGE) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR BackupPortMessage: InputBuffer == NULL || InputBufferLength < sizeof(BACKUP_COMMAND_MESSAGE)\n" );
        return STATUS_INVALID_PARAMETER;
    }

    try
    {
        // Probe and capture input message: the message is raw user mode buffer, so need to protect with exception handler
        command = ((PBACKUP_COMMAND_MESSAGE)InputBuffer)->Command;
        strSrcPath = ((PBACKUP_COMMAND_MESSAGE)InputBuffer)->Path;

        if( command != E_BACKUP_COMMAND_SET_DESTINATION )
        {
            ERROR_PRINT( "\nCB: !!! ERROR BackupPortMessage: Unsupported command %d\n", command );
            return STATUS_INVALID_PARAMETER;
        }

        if( strSrcPath == NULL )
        {
            ERROR_PRINT( "\nCB: !!! ERROR BackupPortMessage: strPath == NULL\n" );
            return STATUS_INVALID_PARAMETER;
        }

        size_t iLen = 0;
        if( ! NT_SUCCESS( RtlStringCbLengthW( strSrcPath, CE_MAX_PATH_SIZE, &iLen ) ) )
        {
            ERROR_PRINT( "\nCB: !!! ERROR BackupPortMessage: RtlStringCbLengthW failed\n" );
            return STATUS_INVALID_PARAMETER;
        }

        if( iLen == 0 )
        {
            ERROR_PRINT( "\nCB: !!! ERROR BackupPortMessage: iLen == 0\n" );
            return STATUS_INVALID_PARAMETER;
        }

        strDstPath = ExAllocatePoolWithTag( NonPagedPool, (iLen + 1) * sizeof(WCHAR), 'kBeC' );

        if( ! strDstPath )
        {
            ERROR_PRINT( "\nCB: !!! ERROR BackupPortMessage: RtlStringCbLengthW failed\n" );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory( strDstPath, strSrcPath, iLen * sizeof(WCHAR) );
        strDstPath[iLen] = 0;
    }
    except( BackupExceptionFilter( GetExceptionInformation(), TRUE ) )
    {
        ExFreePoolWithTag( strDstPath, 'kBeC' );
        return GetExceptionCode();
    }

    if( strDstPath )
    {
        SetDestination( strDstPath, TRUE );
        ExFreePoolWithTag( strDstPath, 'kBeC' );
    }

    return STATUS_SUCCESS;
}

VOID RestorePortDisconnect( _In_opt_ PVOID ConnectionCookie )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER( ConnectionCookie );

    INFO_PRINT( "CB: INFO RestorePortDisconnect: port=0x%p ENTER\n", g_CeBackupData.ClientRestorePort );

    FltCloseClientPort( g_CeBackupData.Filter, &g_CeBackupData.ClientRestorePort );

	ExAcquireFastMutex( &g_CeBackupData.Guard );
	g_CeBackupData.ClientRestorePort = NULL;
    g_CeBackupData.RestoreProcess = NULL;
	ExReleaseFastMutex( &g_CeBackupData.Guard );
}

LONG BackupExceptionFilter ( _In_ PEXCEPTION_POINTERS ExceptionPointer, _In_ BOOLEAN AccessingUserBuffer )
{
    NTSTATUS Status;

    Status = ExceptionPointer->ExceptionRecord->ExceptionCode;
    if( ! FsRtlIsNtstatusExpected( Status ) && ! AccessingUserBuffer )
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS ReadDestination( PUNICODE_STRING RegistryPath )
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    UNICODE_STRING valueName;
    PKEY_VALUE_PARTIAL_INFORMATION stringValue = NULL;
    ULONG stringValueLength = 0;
    ULONG resultLength = 0;

    InitializeObjectAttributes( &attributes, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );
    NTSTATUS status = ZwOpenKey( &driverRegKey, KEY_READ, &attributes );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ReadDestination: ZwOpenKey failed: err=0x%X\n", status );
        return status;
    }

    RtlInitUnicodeString( &valueName, L"Destination" );
    status = ZwQueryValueKey( driverRegKey, &valueName, KeyValuePartialInformation, NULL, 0, &resultLength );
    if( status != STATUS_BUFFER_TOO_SMALL && status != STATUS_BUFFER_OVERFLOW )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ReadDestination: ZwOpenKey failed: err=0x%X\n", status );
        goto Cleanup;
    }

    if( resultLength == 0 )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ReadDestination: ZwQueryValueKey returned resultLength = 0\n" );
        return status;
    }

    stringValueLength = resultLength;
    stringValue = ExAllocatePoolWithTag( PagedPool, stringValueLength + sizeof(WCHAR), 'kBeC' );
    if( ! stringValue )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ReadDestination: ExAllocatePoolWithTag failed: size: %d\n", stringValueLength );
        goto Cleanup;
    }

    RtlZeroMemory( stringValue, stringValueLength + sizeof(WCHAR) );
    status = ZwQueryValueKey( driverRegKey, &valueName, KeyValuePartialInformation, stringValue, stringValueLength, &resultLength );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ReadDestination: ZwQueryValueKey failed: err=0x%X\n", status );
        goto Cleanup;
    }

    if( stringValue->Type != REG_SZ )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ReadDestination: ZwQueryValueKey Type != REG_SZ: err=0x%X\n", status );
        goto Cleanup;
    }

    if( stringValue->DataLength >= CE_MAX_PATH_SIZE )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ReadDestination: ZwQueryValueKey Too long string: DataLength=%d\n", stringValue->DataLength );
        goto Cleanup;
    }

    INFO_PRINT( "CB: INFO Read Destination from registry: %S \n", stringValue->Data );
    SetDestination( (WCHAR*)stringValue->Data, FALSE );

Cleanup:
    if( stringValue )
        ExFreePoolWithTag( stringValue, 'kBeC' );

    if( driverRegKey != NULL )
        ZwClose( driverRegKey );

    return STATUS_SUCCESS;
}

VOID SetDestination( WCHAR* Destination, BOOLEAN Save )
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    UNICODE_STRING valueName;
    size_t iLen = 0;
    PFLT_VOLUME DestinationVolume = NULL;
    NTSTATUS status;

    if( ! NT_SUCCESS( RtlStringCbLengthW( Destination, 1024, &iLen ) ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR SetDestination: RtlStringCbLengthW failed\n" );
        return;
    }

    if( iLen == 0 )
    {
        ERROR_PRINT( "\nCB: !!! ERROR SetDestination: Empty value provided\n" );
        return;
    }

    iLen /= sizeof(WCHAR);
    if( iLen > 2 )
    {
        LONG res = RtlCompareUnicodeStrings( &Destination[1], 2, L":\\", 2, TRUE );

        if( res == 0 )
        {
            UNICODE_STRING uniName;
            uniName.Buffer = Destination;
            uniName.Length = 3 * sizeof(WCHAR);
            uniName.MaximumLength = 3 * sizeof(WCHAR);

            status = FltGetVolumeFromName( g_CeBackupData.Filter, &uniName, &DestinationVolume );
            if( ! NT_SUCCESS( status ) )
            {
                ERROR_PRINT( "\nCB: !!! ERROR SetDestination: FltGetVolumeFromName(%wZ) failed: err=0x%X\n", uniName, status );
                return;
            }

            Destination += 2;
            iLen -= 2;
        }
    }

    ExAcquireFastMutex( &g_CeBackupData.Guard );
    RtlCopyMemory( g_CeBackupData.Destination, Destination, iLen * sizeof(WCHAR) );
    g_CeBackupData.DestinationLength = (USHORT)iLen;
    g_CeBackupData.DestinationVolume = DestinationVolume;
    ExReleaseFastMutex( &g_CeBackupData.Guard );

    INFO_PRINT( "CB: INFO SetDestination: Destination set: %S Volume=%p\n", g_CeBackupData.Destination, iLen, g_CeBackupData.DestinationVolume );

    if( Save )
    {
        InitializeObjectAttributes( &attributes, &g_CeBackupData.RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );
        status = ZwOpenKey( &driverRegKey, KEY_WRITE, &attributes );
        if( NT_SUCCESS( status ) )
        {
            RtlInitUnicodeString( &valueName, L"Destination" );
            status = ZwSetValueKey( driverRegKey, &valueName, KeyValuePartialInformation, REG_SZ, Destination, (ULONG)iLen * sizeof(WCHAR) );
            if( NT_SUCCESS( status ) )
            {
                TMP_PRINT( "CB: INFO Destination %S saved to registry\n", g_CeBackupData.Destination );
            }

            if( driverRegKey != NULL )
                ZwClose( driverRegKey );
        }
    }
}

BOOLEAN IsOurProcess( PFLT_CALLBACK_DATA Data, PEPROCESS* BackupProcess, HANDLE* UserProcessKernel, BOOLEAN* CeUser )
{
    PEPROCESS RestoreProcess = NULL;

    ExAcquireFastMutex( &g_CeBackupData.Guard ); //Going to APCL IRQL
    *BackupProcess = g_CeBackupData.BackupProcess;
    *UserProcessKernel = g_CeBackupData.BackupProcessKernel;
    RestoreProcess = g_CeBackupData.RestoreProcess;
    ExReleaseFastMutex( &g_CeBackupData.Guard );    

    if( *BackupProcess && IoThreadToProcess( Data->Thread ) == *BackupProcess )
    {
        *CeUser = TRUE;
        return TRUE;
    }

    if( RestoreProcess && IoThreadToProcess( Data->Thread ) == RestoreProcess )
    {
        *CeUser = FALSE;
        return TRUE;
    }

    return FALSE;
}
