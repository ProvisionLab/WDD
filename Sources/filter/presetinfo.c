#include <fltKernel.h>
#include <ntstrsafe.h>
#include <dontuse.h>
#include <suppress.h>
#include "cebackup.h"
#include "drvcommon.h"
#include "util.h"

extern BACKUP_DATA g_CeBackupData;

FLT_PREOP_CALLBACK_STATUS PreSetInformation ( _Inout_ PFLT_CALLBACK_DATA Data, _In_ PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID* CompletionContext )
{
    UNREFERENCED_PARAMETER( CompletionContext );
    BOOLEAN IsDeleteFile = FALSE;

    if( ! Data || ! Data->Iopb )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    FILE_INFORMATION_CLASS fileInformationClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

    if( fileInformationClass == FileDispositionInformation )
    {
        PFILE_DISPOSITION_INFORMATION pDisposition = (PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
        IsDeleteFile = pDisposition->DeleteFile;
    }
    if( fileInformationClass == FileDispositionInformationEx )
    {
        PFILE_DISPOSITION_INFORMATION_EX pDisposition = (PFILE_DISPOSITION_INFORMATION_EX)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
        IsDeleteFile = BooleanFlagOn( pDisposition->Flags, FILE_DISPOSITION_DELETE );
    }

    if( ! IsDeleteFile )
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

   DEBUG_PRINT( "CB: DEBUG: PreSetInformation: DeleteFile %wZ\n", Data->Iopb->TargetFileObject->FileName );

   return OpenAndSend( Data, FltObjects, IsDeleteFile );
}
