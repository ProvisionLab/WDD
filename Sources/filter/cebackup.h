#pragma once

#define INVALID_HANDLE_VALUE ((HANDLE) -1)
#define CB_INSTANCE_CONTEXT_SIZE sizeof( CB_INSTANCE_CONTEXT )
#define CB_INSTANCE_CONTEXT_TAG 'cIbC'

typedef struct _FMM_INSTANCE_CONTEXT
{
    PFLT_INSTANCE Instance;
    FLT_FILESYSTEM_TYPE FilesystemType;
    PFLT_VOLUME Volume;
} CB_INSTANCE_CONTEXT, *PCB_INSTANCE_CONTEXT;

typedef struct _BACKUP_DATA
{
    PDRIVER_OBJECT DriverObject;
    PFLT_FILTER Filter;
    PFLT_PORT BackupPort;
	PFLT_PORT RestorePort;
    PFLT_PORT ClientBackupPort;
    PFLT_PORT ClientRestorePort;
    PEPROCESS BackupProcess;
    HANDLE BackupProcessKernel;
    PEPROCESS UserProcessId;
    FAST_MUTEX Guard;
	PEPROCESS RestoreProcess;
} BACKUP_DATA, *PBACKUP_DATA;

/*************************************************************************
    Prototypes
*************************************************************************/
DRIVER_INITIALIZE DriverEntry;

NTSTATUS DriverEntry ( _In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath );
NTSTATUS CbInstanceSetup ( _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType );
VOID CbInstanceTeardownStart ( _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags );
VOID CbInstanceTeardownComplete ( _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags );
NTSTATUS CbUnload ( _In_ FLT_FILTER_UNLOAD_FLAGS Flags );
NTSTATUS CbInstanceQueryTeardown ( _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags );
FLT_PREOP_CALLBACK_STATUS PreCreate ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext );
FLT_PREOP_CALLBACK_STATUS PreSetInformation ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext );
FLT_POSTOP_CALLBACK_STATUS PostOperation ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _In_opt_ PVOID CompletionContext, _In_ FLT_POST_OPERATION_FLAGS Flags );
FLT_PREOP_CALLBACK_STATUS PreOperationNo ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID *CompletionContext );
VOID CbContextCleanup ( _In_ PFLT_CONTEXT Context, _In_ FLT_CONTEXT_TYPE ContextType );
NTSTATUS BackupPortConnect ( _In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionCookie );
NTSTATUS RestorePortConnect ( _In_ PFLT_PORT ClientPort, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID *ConnectionCookie );
VOID BackupPortDisconnect ( _In_opt_ PVOID ConnectionCookie );
VOID RestorePortDisconnect ( _In_opt_ PVOID ConnectionCookie );
FLT_PREOP_CALLBACK_STATUS OpenAndSend ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, BOOLEAN Delete );
NTSTATUS SendHandleToUser ( _In_ HANDLE hFile, _In_ PFLT_VOLUME Volume, _In_ PFLT_FILE_NAME_INFORMATION FltNameInfo, _In_ PFILE_BASIC_INFORMATION FileInfo, _In_ BOOLEAN DeleteOperation, _Out_ PBOOLEAN OkToOpen );

//Undocumented DDK
NTSTATUS NTAPI ZwQueryInformationProcess( IN HANDLE	hProcessHandle, IN PROCESSINFOCLASS nProcessInformationClass, OUT PVOID pProcessInformation, IN ULONG ulProcessInformationLength, OUT PULONG pulReturnLength OPTIONAL );
