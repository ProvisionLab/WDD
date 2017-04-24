#pragma once

#define DEBUG_PRINT DbgPrint
#define ERROR_PRINT DbgPrint
#define TMP_PRINT

#define ASSERT_BOOL_PRINT(x) if( !(x) ) DbgPrint( "ERROR: ASSERT: "## #x );

VOID MjCreatePrint( PFLT_FILE_NAME_INFORMATION NameInfo, ACCESS_MASK DesiredAccess, USHORT shareAccess, ULONG Flags );
const WCHAR* GetStatusString( NTSTATUS Status );
const WCHAR* GetFileFlagString( ULONG Flags );
const WCHAR* GetDeviceFlagString( ULONG Flags );
const WCHAR* GetPreopCallbackStatusString( FLT_PREOP_CALLBACK_STATUS PreopStatus );
NTSTATUS GetCurrentProcessKernelHandler( HANDLE* phProcess );
NTSTATUS GetCurrentProcessHandler( HANDLE* phProcess );
NTSTATUS UserHandleToKernelHandle( HANDLE hProcess, HANDLE* phProcess ) ;
NTSTATUS CloseHandleInProcess( HANDLE hFile, HANDLE hProcess );



NTSTATUS NTAPI ZwQueryInformationProcess(			IN HANDLE						hProcessHandle,
													IN PROCESSINFOCLASS				nProcessInformationClass,
													OUT PVOID						pProcessInformation,
													IN ULONG						ulProcessInformationLength,
													OUT PULONG						pulReturnLength OPTIONAL
													);
