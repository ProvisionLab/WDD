#pragma once

#define ERROR_PRINT DbgPrint
#define INFO_PRINT  DbgPrint
#define DEBUG_PRINT
#define TMP_PRINT   DbgPrint

#define ASSERT_BOOL_PRINT(x) if( !(x) ) DbgPrint( "ERROR: ASSERT: "## #x );

VOID MjCreatePrint( PFLT_FILE_NAME_INFORMATION NameInfo, ACCESS_MASK DesiredAccess, USHORT shareAccess, ULONG Flags );
const WCHAR* GetStatusString( NTSTATUS Status );
const WCHAR* GetFileFlagString( ULONG Flags );
const WCHAR* GetDeviceFlagString( ULONG Flags );
const WCHAR* GetPreopCallbackStatusString( FLT_PREOP_CALLBACK_STATUS PreopStatus );
NTSTATUS GetCurrentProcessKernelHandler( HANDLE* phProcess );
NTSTATUS GetCurrentProcessHandler( HANDLE* phProcess );
NTSTATUS UserHandleToKernelHandle( HANDLE hProcess, HANDLE* phProcess ) ;
NTSTATUS CloseHandleInProcess( HANDLE hFile, PEPROCESS peProcess );
NTSTATUS ReferenceHandleInProcess( HANDLE hFile, PEPROCESS peProcess, PFILE_OBJECT* ppDstFileObject );
