// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CEUSERLIB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CEUSERLIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CEUSERLIB_EXPORTS
#define CEUSERLIB_API __declspec(dllexport)
#else
#define CEUSERLIB_API __declspec(dllimport)
#endif

#include <windows.h>

extern "C"
{

    typedef void (*CallBackBackupCallback)( const wchar_t* SrcPath, const wchar_t* DstPath, int Deleted, HANDLE Pid );
    typedef void (*CallBackCleanupEvent)( const wchar_t* SrcPath, const wchar_t* DstPath, int Deleted );

    typedef enum _CeUserLib_Retval
	{
		CEUSERLIB_OK = 0,
        CEUSERLIB_Failed,
        CEUSERLIB_InvalidParameter,
		CEUSERLIB_NotInitialized,
        CEUSERLIB_AlreadyInitialized,
        CEUSERLIB_OneInstanceOnly,
		CEUSERLIB_ConfigurationError,
        CEUSERLIB_DriverIsNotStarted,
        CEUSERLIB_BackupIsNotStarted,
        CEUSERLIB_BackupIsAlreadyStarted,
        CEUSERLIB_BufferOverflow,
        CEUSERLIB_OutOfMemory,
        CEUSERLIB_AlreadySubscribed
	} CeUserLib_Retval;

    CEUSERLIB_API CeUserLib_Retval Init();
    CEUSERLIB_API CeUserLib_Retval Uninit();
    CEUSERLIB_API const wchar_t* GetLastErrorString();
    CEUSERLIB_API CeUserLib_Retval SubscribeForBackupEvents( CallBackBackupCallback CallBack );
    CEUSERLIB_API CeUserLib_Retval UnsubscribeFromBackupEvents();
    CEUSERLIB_API CeUserLib_Retval SubscribeForCleanupEvents( CallBackCleanupEvent CallBack );
    CEUSERLIB_API CeUserLib_Retval UnsubscribeFromCleanupEvents();
    CEUSERLIB_API CeUserLib_Retval ReloadConfig( const wchar_t* IniPath );
    CEUSERLIB_API int IsDriverStarted();
    CEUSERLIB_API int IsBackupStarted();
    CEUSERLIB_API CeUserLib_Retval Start( const wchar_t* IniPath );
    CEUSERLIB_API CeUserLib_Retval Stop();
    CEUSERLIB_API CeUserLib_Retval Restore_Init( const wchar_t* IniPath );
    CEUSERLIB_API CeUserLib_Retval Restore_Uninit();
    CEUSERLIB_API CeUserLib_Retval Restore_ListAll( wchar_t*** ppRestorePath,  unsigned int* PathCount );
    CEUSERLIB_API CeUserLib_Retval Restore_Restore( const wchar_t* BackupPath );
    CEUSERLIB_API CeUserLib_Retval Restore_RestoreTo( const wchar_t* BackupPath, const wchar_t* ToDirectory );
}
