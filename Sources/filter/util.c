#include <fltKernel.h>
#include <ntstrsafe.h>
#include "drvcommon.h"
#include "util.h"

#define CHECK_STATUS(s) \
if( ! NT_SUCCESS( s ) ) \
{ \
    DbgPrint( "\n!!! CB ERROR %x:", s ); \
    DbgPrint( #s ); \
    return; \
}

VOID MjCreatePrint( PFLT_FILE_NAME_INFORMATION NameInfo, ACCESS_MASK DesiredAccess, USHORT ShareAccess, ULONG Flags )
{
    DECLARE_UNICODE_STRING_SIZE( FinalBuffer, MAX_PATH_SIZE );
    DECLARE_UNICODE_STRING_SIZE( DaBuffer, MAX_PATH_SIZE );
    DECLARE_UNICODE_STRING_SIZE( SaBuffer, MAX_PATH_SIZE );
    DECLARE_UNICODE_STRING_SIZE( FlgBuffer, MAX_PATH_SIZE );

    CHECK_STATUS( RtlUnicodeStringPrintf( &FinalBuffer, L"CB: PreCreate Name=%wZ", &NameInfo->FinalComponent ) );
    CHECK_STATUS( RtlUnicodeStringPrintf( &DaBuffer, L" DesiredAccess=0x%X(", DesiredAccess ) );
    CHECK_STATUS( RtlUnicodeStringPrintf( &SaBuffer, L" ShareAccess=0x%X(", ShareAccess ) );
    CHECK_STATUS( RtlUnicodeStringPrintf( &FlgBuffer, L" Flags=0x%X(", Flags ) );

    // DesiredAccess=0x120196 = FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_READ_DATA | FILE_READ_EA | FILE_WRITE_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE
    if( FlagOn( DesiredAccess, FILE_WRITE_DATA ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"WRITE_DATA " ) );
    }
    if( FlagOn( DesiredAccess, FILE_READ_DATA ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"READ_DATA " ) );
    }
    if( FlagOn( DesiredAccess, FILE_READ_ATTRIBUTES ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"READ_ATTRIBUTES " ) );
    }
    if( FlagOn( DesiredAccess, FILE_WRITE_ATTRIBUTES ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"WRITE_ATTRIBUTES " ) );
    }
    if( FlagOn( DesiredAccess, FILE_APPEND_DATA ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"FILE_APPEND_DATA " ) );
    }
    if( FlagOn( DesiredAccess, FILE_READ_EA ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"READ_EA " ) );
    }
    if( FlagOn( DesiredAccess, FILE_WRITE_EA ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"WRITE_EA " ) );
    }
    if( FlagOn( DesiredAccess, READ_CONTROL ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"READ_CONTROL " ) );
    }
    if( FlagOn( DesiredAccess, SYNCHRONIZE ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"SYNCHRONIZE " ) );
    }
    if( FlagOn( DesiredAccess, DELETE ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L"DELETE " ) );
    }

    CHECK_STATUS( RtlUnicodeStringCatString( &DaBuffer, L")" ) );

    if( FlagOn( ShareAccess, FILE_SHARE_READ ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &SaBuffer, L"SHARE_READ " ) );
    }
    if( FlagOn( ShareAccess, FILE_SHARE_WRITE ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &SaBuffer, L"SHARE_WRITE " ) );
    }
    if( FlagOn( ShareAccess, FILE_SHARE_DELETE ) )
    {
        CHECK_STATUS( RtlUnicodeStringCatString( &SaBuffer, L"SHARE_DELETE " ) );
    }

    CHECK_STATUS( RtlUnicodeStringCatString( &SaBuffer, L")" ) );

    CHECK_STATUS( RtlUnicodeStringCatString( &FlgBuffer, L")" ) );

    CHECK_STATUS( RtlUnicodeStringCat( &FinalBuffer, &DaBuffer ) );
    CHECK_STATUS( RtlUnicodeStringCat( &FinalBuffer, &SaBuffer ) );
    WCHAR Buffer[MAX_PATH_SIZE];
    CHECK_STATUS( RtlUnicodeStringCatString( &FinalBuffer, GetFileFlagString( Flags, Buffer ) ) );

    DEBUG_PRINT( "%wZ", FinalBuffer );
}

const WCHAR* GetStatusString( NTSTATUS Status, WCHAR* Buffer )
{
    if( Status == STATUS_OBJECT_NAME_INVALID )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_OBJECT_NAME_INVALID" );
    else if( Status == STATUS_OBJECT_PATH_NOT_FOUND )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_OBJECT_PATH_NOT_FOUND" );
    else if( Status == STATUS_INVALID_PARAMETER )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_INVALID_PARAMETER" );
    else if( Status == STATUS_ACCESS_DENIED )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_ACCESS_DENIED" );
    else if( Status == STATUS_INVALID_HANDLE )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_INVALID_HANDLE" );
    else if( Status == STATUS_FLT_INVALID_NAME_REQUEST )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_FLT_INVALID_NAME_REQUEST" );
    else if( Status == STATUS_SHARING_VIOLATION )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_SHARING_VIOLATION" );
    else if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_OBJECT_NAME_NOT_FOUND" );
    else if( Status == STATUS_PORT_DISCONNECTED )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_PORT_DISCONNECTED" );
    else if( Status == STATUS_INFO_LENGTH_MISMATCH )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"STATUS_INFO_LENGTH_MISMATCH" );
    else
        RtlStringCbPrintfW( Buffer, MAX_PATH_SIZE, L"%X", Status );

    return Buffer;
}

const WCHAR* GetFileFlagString( ULONG Flags, WCHAR* Buffer )
{
    RtlStringCbPrintfW( Buffer, MAX_PATH_SIZE, L"0x%X(", Flags ) ;

    if( Flags & FO_FILE_OPEN )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_FILE_OPEN " );
    if( Flags & FO_SYNCHRONOUS_IO )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_SYNCHRONOUS_IO " );
    if( Flags & FO_ALERTABLE_IO )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_ALERTABLE_IO " );
    if( Flags & FO_NO_INTERMEDIATE_BUFFERING )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_NO_INTERMEDIATE_BUFFERING " );
    if( Flags & FO_WRITE_THROUGH )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_WRITE_THROUGH " );
    if( Flags & FO_SEQUENTIAL_ONLY )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_SEQUENTIAL_ONLY " );
    if( Flags & FO_CACHE_SUPPORTED )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_CACHE_SUPPORTED " );
    if( Flags & FO_NAMED_PIPE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_NAMED_PIPE " );
    if( Flags & FO_STREAM_FILE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_STREAM_FILE " );
    if( Flags & FO_MAILSLOT )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_MAILSLOT " );
    if( Flags & FO_GENERATE_AUDIT_ON_CLOSE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_GENERATE_AUDIT_ON_CLOSE " );
    if( Flags & FO_DIRECT_DEVICE_OPEN )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_DIRECT_DEVICE_OPEN " );
    if( Flags & FO_FILE_MODIFIED )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_FILE_MODIFIED " );
    if( Flags & FO_FILE_SIZE_CHANGED )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_FILE_SIZE_CHANGED " );
    if( Flags & FO_CLEANUP_COMPLETE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_CLEANUP_COMPLETE " );
    if( Flags & FO_TEMPORARY_FILE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_TEMPORARY_FILE " );
    if( Flags & FO_DELETE_ON_CLOSE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_DELETE_ON_CLOSE " );
    if( Flags & FO_OPENED_CASE_SENSITIVE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_OPENED_CASE_SENSITIVE " );
    if( Flags & FO_HANDLE_CREATED )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_HANDLE_CREATED " );
    if( Flags & FO_FILE_FAST_IO_READ )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_FILE_FAST_IO_READ " );
    if( Flags & FO_RANDOM_ACCESS )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_RANDOM_ACCESS " );
    if( Flags & FO_FILE_OPEN_CANCELLED )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_FILE_OPEN_CANCELLED " );
    if( Flags & FO_VOLUME_OPEN )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_VOLUME_OPEN " );
    if( Flags & FO_REMOTE_ORIGIN )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_REMOTE_ORIGIN " );
    if( Flags & FO_DISALLOW_EXCLUSIVE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_DISALLOW_EXCLUSIVE " );
    if( Flags & FO_SKIP_SET_EVENT )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_SKIP_SET_EVENT " );
    if( Flags & FO_SKIP_SET_FAST_IO )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_SKIP_SET_FAST_IO " );
    if( Flags & FO_INDIRECT_WAIT_OBJECT )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_INDIRECT_WAIT_OBJECT " );
    if( Flags & FO_SECTION_MINSTORE_TREATMENT )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"FO_SECTION_MINSTORE_TREATMENT " );

    RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L")" );

    return Buffer;
}

const WCHAR* GetDeviceFlagString( ULONG Flags, WCHAR* Buffer )
{
    RtlStringCbPrintfW( Buffer, MAX_PATH_SIZE, L"0x%X(", Flags ) ;

    if( Flags & DO_VERIFY_VOLUME )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_VERIFY_VOLUME " );
    if( Flags & DO_BUFFERED_IO )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"ZZ DO_BUFFERED_IO " );
    if( Flags & DO_EXCLUSIVE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_EXCLUSIVE " );
    if( Flags & DO_DIRECT_IO )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_DIRECT_IO " );
    if( Flags & DO_MAP_IO_BUFFER )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_MAP_IO_BUFFER " );
    if( Flags & DO_DEVICE_INITIALIZING )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_DEVICE_INITIALIZING " );
    if( Flags & DO_SHUTDOWN_REGISTERED )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_SHUTDOWN_REGISTERED " );
    if( Flags & DO_BUS_ENUMERATED_DEVICE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_BUS_ENUMERATED_DEVICE " );
    if( Flags & DO_POWER_PAGABLE )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_POWER_PAGABLE " );
    if( Flags & DO_POWER_INRUSH )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_POWER_INRUSH " );
    if( Flags & DO_DEVICE_TO_BE_RESET )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_DEVICE_TO_BE_RESET " );
    if( Flags & DO_DAX_VOLUME )
        RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L"DO_DAX_VOLUME " );

    RtlStringCbCatW( Buffer, MAX_PATH_SIZE, L")" );

    return Buffer;
}

const WCHAR* GetPreopCallbackStatusString( FLT_PREOP_CALLBACK_STATUS PreopStatus, WCHAR* Buffer )
{
    if( PreopStatus == FLT_PREOP_SUCCESS_WITH_CALLBACK )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"FLT_PREOP_SUCCESS_WITH_CALLBACK " );
    else if( PreopStatus == FLT_PREOP_SUCCESS_NO_CALLBACK )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"FLT_PREOP_SUCCESS_NO_CALLBACK " );
    else if( PreopStatus == FLT_PREOP_PENDING )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"FLT_PREOP_PENDING " );
    else if( PreopStatus == FLT_PREOP_DISALLOW_FASTIO )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"FLT_PREOP_DISALLOW_FASTIO " );
    else if( PreopStatus == FLT_PREOP_COMPLETE )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"FLT_PREOP_COMPLETE " );
    else if( PreopStatus == FLT_PREOP_SYNCHRONIZE )
        RtlStringCbCopyW( Buffer, MAX_PATH_SIZE, L"FLT_PREOP_SYNCHRONIZE " );

    return Buffer;
}

NTSTATUS GetCurrentProcessKernelHandler( HANDLE* phProcess )
{
    HANDLE hProcess = NULL;
    NTSTATUS status = GetCurrentProcessHandler( &hProcess );
    if( ! NT_SUCCESS(status) )
        return status;

    status = UserHandleToKernelHandle( hProcess, phProcess );

    ZwClose( hProcess ) ;

    return status;
}

NTSTATUS GetCurrentProcessHandler( HANDLE* phProcess )
{
    // NtCurrentProcess() = -1 = pseudo-handler
    // PsGetCurrentProcess() = current process user handler

    CLIENT_ID clientId;
    OBJECT_ATTRIBUTES objAttribs;
    clientId.UniqueThread = PsGetCurrentThreadId();
    clientId.UniqueProcess = PsGetCurrentProcessId();
    InitializeObjectAttributes( &objAttribs, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );

    NTSTATUS status = ZwOpenProcess( phProcess, PROCESS_DUP_HANDLE, &objAttribs, &clientId );
    if( ! NT_SUCCESS(status) )
    {
        WCHAR Buffer[MAX_PATH_SIZE];
        ERROR_PRINT( "\nCB: !!! ZwOpenProcess failed. status=%S\n", GetStatusString( status, Buffer ) );
        return status;
    }

    return status;
}

 NTSTATUS UserHandleToKernelHandle( HANDLE hSrc, HANDLE* phDest )
 {
    PVOID pObject = NULL;
    NTSTATUS status = ObReferenceObjectByHandle( hSrc, 0, NULL, KernelMode, &pObject, NULL);
    if( ! NT_SUCCESS(status) )
    {
        WCHAR Buffer[MAX_PATH_SIZE];
        ERROR_PRINT( "\nCB: !!! UserHandleToKernelHandle: ObReferenceObjectByHandle failed. status=%S\n", GetStatusString( status, Buffer ) );
        return status;
    }

	ACCESS_MASK nDesiredAccess = STANDARD_RIGHTS_REQUIRED | SPECIFIC_RIGHTS_ALL | GENERIC_ALL;
	status = ObOpenObjectByPointer( pObject, OBJ_KERNEL_HANDLE, 0, nDesiredAccess, NULL, KernelMode, phDest );
    if( ! NT_SUCCESS(status) )
    {
        WCHAR Buffer[MAX_PATH_SIZE];
        ERROR_PRINT( "\nCB: !!! UserHandleToKernelHandle: ObOpenObjectByPointer failed. status=%S\n", GetStatusString( status, Buffer ) );
        return status;
    }

    ObDereferenceObject(pObject);

    if( ! ObIsKernelHandle( *phDest ) )
    {
        ERROR_PRINT( "\nCB: !!! ObIsKernelHandle(ZwOpenProcess): The HANDLE is NOT kernel one: %p\n", *phDest );
        return STATUS_INVALID_HANDLE;
    }

    return status;
}

NTSTATUS CloseHandleInProcess( HANDLE hFile, PEPROCESS peProcess )
{
	NTSTATUS status = STATUS_SUCCESS;
    KAPC_STATE kApcSt;
    KeStackAttachProcess( (PKPROCESS)peProcess, &kApcSt );
	if( peProcess == PsGetCurrentProcess() )
	{
		// We may have referenced kernel object, but still fail here on KiPageFault SYSTEM_SERVICE_EXCEPTION
		status = ObCloseHandle( hFile, UserMode ); // PASSIVE_LEVEL
	}
	else
	{
		//Process has exit already?
		ERROR_PRINT( "\nCB: !!! ERROR CloseHandleInProcess: KeStackAttachProcess failed to switch to process: %p\n\n", peProcess );
	}

    KeUnstackDetachProcess( &kApcSt );

    return status;
}

NTSTATUS ReferenceHandleInProcess( HANDLE hFile, PEPROCESS peProcess, PFILE_OBJECT* ppDstFileObject )
{
	NTSTATUS status = STATUS_SUCCESS;
    KAPC_STATE kApcSt;
    KeStackAttachProcess( (PKPROCESS)peProcess, &kApcSt );
	if( peProcess == PsGetCurrentProcess() )
	{
		status = ObReferenceObjectByHandle( hFile, KEY_ALL_ACCESS, *IoFileObjectType, KernelMode, ppDstFileObject, NULL ); // UserMode = STATUS_ACCESS_DENIED
	}
	else
	{
		//Process has exit already?
		ERROR_PRINT( "\nCB: !!! ERROR ReferenceHandleInProcess: KeStackAttachProcess failed to switch process to: %p\n\n", peProcess );
	}

    KeUnstackDetachProcess( &kApcSt );

    return status;
}
