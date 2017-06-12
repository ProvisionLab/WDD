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
    BOOLEAN isDelete = FALSE;
    BOOLEAN bReject = FALSE;

    if( ! Data || ! Data->Iopb || Data->Iopb->MajorFunction != IRP_MJ_CREATE )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    // Backup directory protection
    USHORT iFileNameLen = Data->Iopb->TargetFileObject->FileName.Length / sizeof(WCHAR);
    if( Data->Iopb->TargetFileObject && Data->Iopb->TargetFileObject->FileName.Buffer && Data->Iopb->TargetFileObject->FileName.Length && g_CeBackupData.DestinationLength && (g_CeBackupData.DestinationVolume ? g_CeBackupData.DestinationVolume == FltObjects->Volume : TRUE) && iFileNameLen >= g_CeBackupData.DestinationLength )
    {
        LONG res = RtlCompareUnicodeStrings( g_CeBackupData.Destination, g_CeBackupData.DestinationLength, Data->Iopb->TargetFileObject->FileName.Buffer, g_CeBackupData.DestinationLength, TRUE );
        bReject = res == 0;
        if( bReject )
        {
            BOOLEAN bCeUser = TRUE;
            PEPROCESS BackupProcess = NULL;
            HANDLE UserProcessKernel = NULL;
            if( IsOurProcess( Data, &BackupProcess, &UserProcessKernel, &bCeUser ) )
                return FLT_PREOP_SUCCESS_NO_CALLBACK;

            TMP_PRINT( "CB: DEBUG: Access to %wZ was REJECTED\n", Data->Iopb->TargetFileObject->FileName );

            FltCancelFileOpen( FltObjects->Instance, FltObjects->FileObject );

            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            Data->IoStatus.Information = 0;

            return FLT_PREOP_COMPLETE;
        }
    }

    //MjCreatePrint( &Data->Iopb->TargetFileObject->FileName, Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, Data->Iopb->Parameters.Create.ShareAccess, FltObjects->FileObject->Flags );

    //  Directory opens don't need to be scanned
    if( FlagOn( Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE ) )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    // msdn: The DeleteFile function fails if an application attempts to delete a file that has other handles open for normal I/O 
    // or as a memory-mapped file (FILE_SHARE_DELETE must have been specified when other handles were opened).
    // The DeleteFile function marks a file for deletion on close. Therefore, the file deletion does not occur until the last handle to the file is closed.
    if( FlagOn( Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE ) && FlagOn( Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, DELETE ) )
    {
        isDelete = TRUE;
        DEBUG_PRINT( "CB: DEBUG: DELETE_ON_CLOSE detected %wZ", Data->Iopb->TargetFileObject->FileName );
    }

    // Skip if its opened for read
    if( ! isDelete && ! FlagOn( Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_WRITE_DATA ) ) // FltObjects->FileObject->WriteAccess is not set in PreCreate yet!
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    if( ! Data->Iopb->Parameters.Create.SecurityContext )
    {
        ERROR_PRINT( "\nCB: ERROR Data->Iopb->Parameters.Create.SecurityContext is NULL\n" );
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    return OpenAndSend( Data, FltObjects, isDelete );
}
