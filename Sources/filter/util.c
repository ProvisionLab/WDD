#include <fltKernel.h>
#include <ntstrsafe.h>
#include "cecommon.h"
#include "util.h"

#define PROCESS_QUERY_INFORMATION (0x0400)

#define CHECK_STATUS(s) \
if( ! NT_SUCCESS( (s) ) ) \
{ \
    DbgPrint( "\n!CB ERROR:" ); \
    DbgPrint( #s ); \
    return; \
}

VOID MjCreatePrint( PFLT_FILE_NAME_INFORMATION NameInfo, ACCESS_MASK DesiredAccess, USHORT ShareAccess, ULONG Flags )
{
    //DbgPrint( "!CB PreCreate Name=%wZ\n", NameInfo->FinalComponent );

    DECLARE_UNICODE_STRING_SIZE( FinalBuffer, MAX_PATH_SIZE );
    DECLARE_UNICODE_STRING_SIZE( DaBuffer, MAX_PATH_SIZE );
    DECLARE_UNICODE_STRING_SIZE( SaBuffer, MAX_PATH_SIZE );
    DECLARE_UNICODE_STRING_SIZE( FlgBuffer, MAX_PATH_SIZE );

    CHECK_STATUS( RtlUnicodeStringPrintf( &FinalBuffer, L"!CB PreCreate Name=%wZ", &NameInfo->FinalComponent ) );
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

     // FO_OPENED_CASE_SENSITIVE FO_FILE_OPEN
    CHECK_STATUS( RtlUnicodeStringCatString( &FlgBuffer, L")" ) );

    CHECK_STATUS( RtlUnicodeStringCat( &FinalBuffer, &DaBuffer ) );
    CHECK_STATUS( RtlUnicodeStringCat( &FinalBuffer, &SaBuffer ) );
    CHECK_STATUS( RtlUnicodeStringCat( &FinalBuffer, &FlgBuffer ) );

    DbgPrint( "%wZ", FinalBuffer );
}

static WCHAR g_StatusString[MAX_PATH_SIZE];

const WCHAR* GetStatusString( NTSTATUS Status )
{
    if( Status == STATUS_OBJECT_NAME_INVALID )
        RtlStringCbCopyW( g_StatusString, sizeof(g_StatusString), L"STATUS_OBJECT_NAME_INVALID" );
    else if( Status == STATUS_OBJECT_PATH_NOT_FOUND )
        RtlStringCbCopyW( g_StatusString, sizeof(g_StatusString), L"STATUS_OBJECT_PATH_NOT_FOUND" );
    else if( Status == STATUS_INVALID_PARAMETER )
        RtlStringCbCopyW( g_StatusString, sizeof(g_StatusString), L"STATUS_INVALID_PARAMETER" );
    else if( Status == STATUS_ACCESS_DENIED )
        RtlStringCbCopyW( g_StatusString, sizeof(g_StatusString), L"STATUS_ACCESS_DENIED" );
    else if( Status == STATUS_INVALID_HANDLE )
        RtlStringCbCopyW( g_StatusString, sizeof(g_StatusString), L"STATUS_INVALID_HANDLE" );
    else if( Status == STATUS_FLT_INVALID_NAME_REQUEST )
        RtlStringCbCopyW( g_StatusString, sizeof(g_StatusString), L"STATUS_FLT_INVALID_NAME_REQUEST" );
    else
        RtlStringCbPrintfW( g_StatusString, sizeof(g_StatusString), L"%X", Status );

    return g_StatusString;
}

static WCHAR g_FileFlagString[MAX_PATH_SIZE];

const WCHAR* GetFileFlagString( ULONG Flags )
{
    RtlStringCbPrintfW( g_FileFlagString, sizeof(g_FileFlagString), L"0x%X(", Flags ) ;

    if( Flags & FO_FILE_OPEN )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_FILE_OPEN " );
    if( Flags & FO_SYNCHRONOUS_IO )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_SYNCHRONOUS_IO " );
    if( Flags & FO_ALERTABLE_IO )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_ALERTABLE_IO " );
    if( Flags & FO_NO_INTERMEDIATE_BUFFERING )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_NO_INTERMEDIATE_BUFFERING " );
    if( Flags & FO_WRITE_THROUGH )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_WRITE_THROUGH " );
    if( Flags & FO_SEQUENTIAL_ONLY )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_SEQUENTIAL_ONLY " );
    if( Flags & FO_CACHE_SUPPORTED )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_CACHE_SUPPORTED " );
    if( Flags & FO_NAMED_PIPE )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_NAMED_PIPE " );
    if( Flags & FO_STREAM_FILE )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_STREAM_FILE " );
    if( Flags & FO_MAILSLOT )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_MAILSLOT " );
    if( Flags & FO_GENERATE_AUDIT_ON_CLOSE )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_GENERATE_AUDIT_ON_CLOSE " );
    if( Flags & FO_DIRECT_DEVICE_OPEN )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_DIRECT_DEVICE_OPEN " );
    if( Flags & FO_FILE_MODIFIED )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_FILE_MODIFIED " );
    if( Flags & FO_FILE_SIZE_CHANGED )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_FILE_SIZE_CHANGED " );
    if( Flags & FO_CLEANUP_COMPLETE )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_CLEANUP_COMPLETE " );
    if( Flags & FO_TEMPORARY_FILE )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_TEMPORARY_FILE " );
    if( Flags & FO_DELETE_ON_CLOSE )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_DELETE_ON_CLOSE " );
    if( Flags & FO_OPENED_CASE_SENSITIVE )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_OPENED_CASE_SENSITIVE " );
    if( Flags & FO_HANDLE_CREATED )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_HANDLE_CREATED " );
    if( Flags & FO_FILE_FAST_IO_READ )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_FILE_FAST_IO_READ " );
    if( Flags & FO_RANDOM_ACCESS )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_RANDOM_ACCESS " );
    if( Flags & FO_FILE_OPEN_CANCELLED )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_FILE_OPEN_CANCELLED " );
    if( Flags & FO_VOLUME_OPEN )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_VOLUME_OPEN " );
    if( Flags & FO_REMOTE_ORIGIN )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_REMOTE_ORIGIN " );
    if( Flags & FO_DISALLOW_EXCLUSIVE )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_DISALLOW_EXCLUSIVE " );
    if( Flags & FO_SKIP_SET_EVENT )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_SKIP_SET_EVENT " );
    if( Flags & FO_SKIP_SET_FAST_IO )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_SKIP_SET_FAST_IO " );
    if( Flags & FO_INDIRECT_WAIT_OBJECT )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_INDIRECT_WAIT_OBJECT " );
    if( Flags & FO_SECTION_MINSTORE_TREATMENT )
        RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L"FO_SECTION_MINSTORE_TREATMENT " );

    RtlStringCbCatW( g_FileFlagString, sizeof(g_FileFlagString), L")" );

    return g_FileFlagString;
}


static WCHAR g_DeviceFlagString[MAX_PATH_SIZE];

const WCHAR* GetDeviceFlagString( ULONG Flags )
{
    RtlStringCbPrintfW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"0x%X(", Flags ) ;

    if( Flags & DO_VERIFY_VOLUME )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_VERIFY_VOLUME " );
    if( Flags & DO_BUFFERED_IO )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"ZZ DO_BUFFERED_IO " );
    if( Flags & DO_EXCLUSIVE )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_EXCLUSIVE " );
    if( Flags & DO_DIRECT_IO )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_DIRECT_IO " );
    if( Flags & DO_MAP_IO_BUFFER )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_MAP_IO_BUFFER " );
    if( Flags & DO_DEVICE_INITIALIZING )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_DEVICE_INITIALIZING " );
    if( Flags & DO_SHUTDOWN_REGISTERED )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_SHUTDOWN_REGISTERED " );
    if( Flags & DO_BUS_ENUMERATED_DEVICE )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_BUS_ENUMERATED_DEVICE " );
    if( Flags & DO_POWER_PAGABLE )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_POWER_PAGABLE " );
    if( Flags & DO_POWER_INRUSH )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_POWER_INRUSH " );
    if( Flags & DO_DEVICE_TO_BE_RESET )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_DEVICE_TO_BE_RESET " );
    if( Flags & DO_DAX_VOLUME )
        RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L"DO_DAX_VOLUME " );

    RtlStringCbCatW( g_DeviceFlagString, sizeof(g_DeviceFlagString), L")" );

    return g_DeviceFlagString;
}

static WCHAR g_PreopCallbackStatusString[MAX_PATH_SIZE];

const WCHAR* GetPreopCallbackStatusString( FLT_PREOP_CALLBACK_STATUS PreopStatus )
{
    if( PreopStatus == FLT_PREOP_SUCCESS_WITH_CALLBACK )
        RtlStringCbCopyW( g_PreopCallbackStatusString, sizeof(g_PreopCallbackStatusString), L"FLT_PREOP_SUCCESS_WITH_CALLBACK " );
    else if( PreopStatus == FLT_PREOP_SUCCESS_NO_CALLBACK )
        RtlStringCbCopyW( g_PreopCallbackStatusString, sizeof(g_PreopCallbackStatusString), L"FLT_PREOP_SUCCESS_NO_CALLBACK " );
    else if( PreopStatus == FLT_PREOP_PENDING )
        RtlStringCbCopyW( g_PreopCallbackStatusString, sizeof(g_PreopCallbackStatusString), L"FLT_PREOP_PENDING " );
    else if( PreopStatus == FLT_PREOP_DISALLOW_FASTIO )
        RtlStringCbCopyW( g_PreopCallbackStatusString, sizeof(g_PreopCallbackStatusString), L"FLT_PREOP_DISALLOW_FASTIO " );
    else if( PreopStatus == FLT_PREOP_COMPLETE )
        RtlStringCbCopyW( g_PreopCallbackStatusString, sizeof(g_PreopCallbackStatusString), L"FLT_PREOP_COMPLETE " );
    else if( PreopStatus == FLT_PREOP_SYNCHRONIZE )
        RtlStringCbCopyW( g_PreopCallbackStatusString, sizeof(g_PreopCallbackStatusString), L"FLT_PREOP_SYNCHRONIZE " );

    return g_PreopCallbackStatusString;
}

NTSTATUS GetCurrentProcessKernelHandler( HANDLE* phProcess )
{
    HANDLE hProcess = NULL;
    NTSTATUS status = GetCurrentProcessHandler( &hProcess );
    if( ! NT_SUCCESS(status) )
        return status;

    status = UserHandleToKernelHandle( hProcess, phProcess );

    // ??? ZwClose( hProcess ) ;

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
    InitializeObjectAttributes( &objAttribs, NULL, OBJ_KERNEL_HANDLE | PROCESS_QUERY_INFORMATION, NULL, NULL );

    NTSTATUS status = ZwOpenProcess( phProcess, PROCESS_DUP_HANDLE, &objAttribs, &clientId );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\nCB: !!! ZwOpenProcess failed. status=%S\n", GetStatusString( status ) );
        return status;
    }

    return status;
}

 NTSTATUS UserHandleToKernelHandle( HANDLE hSrc, HANDLE* phDest )
 {
    PVOID pObject = NULL;
    NTSTATUS status = ObReferenceObjectByHandle( hSrc, 0x8, NULL, KernelMode, &pObject, NULL);
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\nCB: !!! UserHandleToKernelHandle: ObReferenceObjectByHandle failed. status=%S\n", GetStatusString( status ) );
        return status;
    }

	ACCESS_MASK nDesiredAccess = STANDARD_RIGHTS_REQUIRED | SPECIFIC_RIGHTS_ALL | GENERIC_ALL;
	status = ObOpenObjectByPointer( pObject, OBJ_KERNEL_HANDLE, 0, nDesiredAccess, NULL, KernelMode, phDest );
    if( ! NT_SUCCESS(status) )
    {
        ERROR_PRINT( "\nCB: !!! UserHandleToKernelHandle: ObOpenObjectByPointer failed. status=%S\n", GetStatusString( status ) );
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

NTSTATUS CloseHandleInProcess( HANDLE hFile, PEPROCESS hProcess )
{
    KAPC_STATE kApcSt;
    KeStackAttachProcess( (PKPROCESS)hProcess, &kApcSt );

    NTSTATUS status = ZwClose( hFile );

    KeUnstackDetachProcess( &kApcSt );

    return status;
}

/*
    HANDLE hFile2 = NULL;
    status = ZwCreateFile( &hFile2, FILE_GENERIC_READ, &objectAttributes );
    ERROR_PRINT( "CB: TMP ZwCreate hFile = %p status=0x%X\n", hFile2, status );
    ZwClose( hFile2 );
*/


/* RECURSION!
    status = ZwCreateFile( &hFile,
                            FILE_ALL_ACCESS,
                            &oa,
                            &ioStatus,
                            (PLARGE_INTEGER) NULL,
                            0,
                            FILE_SHARE_READ,
                            FILE_OPEN, // If the file already exists, open it instead of creating a new file. If it does not, fail the request and do not create a new file.
                            0L,
                            NULL,
                            0L );
*/


    //CHECK hFile
    //STATUS_INVALID_PARAMETER
    /*
    IO_STATUS_BLOCK ioStatusRead = {0};
    char TmpBuffer[512] = {0};
    ULONG TmpBufferLength = sizeof(TmpBuffer);
    status = ZwReadFile( hFile, NULL, NULL, NULL, &ioStatusRead, TmpBuffer, TmpBufferLength, 0, NULL );
    if( ! NT_SUCCESS( status ) )
    {
        ERROR_PRINT( "\nCB: !!! ERROR ZwReadFile(%p) failed status=%S\n", hFile, GetStatusString( status ) );
        status = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto Cleanup;
    }
    */

/* Possible fail
    NtReadFile()
    if( pFileObject->Flags & FO_SYNCHRONOUS_IO ) {}
    else if (!ARGUMENT_PRESENT( ByteOffset ) && !(fileObject->Flags & (FO_NAMED_PIPE | FO_MAILSLOT))) 
    {
        // The file is not open for synchronous I/O operations, but the
        // caller did not specify a ByteOffset parameter.
        return STATUS_INVALID_PARAMETER;
    }
*/


 /*  It's not necessary to provide source Process and Handle to ZwDuplicateObject as a kernel
    status = GetCurrentProcessKernelHandler( &SourceProcessHandle );
    if( ! NT_SUCCESS(status) )
    {
        status = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto Cleanup;
    }

    status = UserHandleToKernelHandle( hFile, &SourceHandle );
    if( ! NT_SUCCESS(status) )
    {
        status = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto Cleanup;
    }

    ASSERT_BOOL_PRINT( ObIsKernelHandle( SourceProcessHandle ) );
    ASSERT_BOOL_PRINT( ObIsKernelHandle( TargetProcessHandle ) );
    ASSERT_BOOL_PRINT( ObIsKernelHandle( SourceHandle ) );
*/
