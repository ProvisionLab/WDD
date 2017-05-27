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
    CEUSERLIB_API bool Start( const wchar_t* IniPath );
    CEUSERLIB_API bool Stop();
    CEUSERLIB_API const wchar_t* GetErrorString();
}
