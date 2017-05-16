#include "usercommon.h"
#include <winsvc.h>
#include "client.h"
#include "utils.h"

#define SERVICE_NAME _T("Ceedo.Backup.WindowsService")
#define SERVICE_DISPLAY_NAME _T("Ceedo Backup Service")
#define SERVICE_DESCRIPTION_TEXT _T("CeedoBackup is a Service that provide backup of filtered files")

#define SVC_ERROR                        ((DWORD)0xC0020001L)

CBackupClient* _pService = NULL;
SERVICE_STATUS g_Status;
SERVICE_STATUS_HANDLE g_StatusHandle;
LPTSTR g_Name = SERVICE_NAME;
std::wstring g_StrIniPath;

void SetStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode = NO_ERROR, DWORD dwWaitHint = 0)
{
    static DWORD dwCheckPoint = 1;
    g_Status.dwCurrentState    = dwCurrentState;
    g_Status.dwWin32ExitCode   = dwWin32ExitCode;
    g_Status.dwWaitHint        = dwWaitHint;

    g_Status.dwCheckPoint = ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) ? 0 : dwCheckPoint++;

    ::SetServiceStatus(g_StatusHandle, &g_Status);
}

void WriteEventLogEntry( LPCTSTR pszMessage, WORD wType = EVENTLOG_ERROR_TYPE )
{
    // To fix the description for Event ID 0 from source we need to add .mc file
    HANDLE hEventSource    = NULL;
    LPCTSTR lpszStrings[2] = { NULL, NULL };

    hEventSource = ::RegisterEventSourceW( NULL, g_Name );
    if( hEventSource )
    {
        lpszStrings[0] = g_Name;
        lpszStrings[1] = pszMessage;

        ::ReportEventW( hEventSource, // Event log handle
                      wType,        // Event type
                      0,            // Event category
                      SVC_ERROR,    // Event identifier
                      NULL,         // No security identifier
                      2,            // Size of lpszStrings array
                      0,            // No binary data
                      lpszStrings,  // Array of strings
                      NULL          // No binary data
                      );

        ::DeregisterEventSource( hEventSource );
    }
}

void WriteErrorLogEntry( LPCTSTR pszFunction, DWORD dwError = ::GetLastError() )
{
    TCHAR szMessage[260];
    _stprintf_s( szMessage, ARRAYSIZE(szMessage), _T("Service failed with error: %s, status: 0x%08lx"), pszFunction, dwError);
    WriteEventLogEntry( szMessage, EVENTLOG_ERROR_TYPE );
}

void WINAPI ServiceCtrlHandler( DWORD dwCtrl )
{
    DWORD dwOriginalState = g_Status.dwCurrentState;
    switch( dwCtrl )
    {
        case SERVICE_CONTROL_STOP:
            try
            {
                SetStatus( SERVICE_STOP_PENDING );
                _pService->Stop();
                SetStatus( SERVICE_STOPPED );
                delete _pService;
                _pService = NULL;
            }
            catch (...)
            {
                WriteEventLogEntry(_T("Service: Failed to stop."), EVENTLOG_ERROR_TYPE);
                SetStatus(dwOriginalState);
            }
            break;
        case SERVICE_CONTROL_SHUTDOWN:
            _pService->Stop();
            SetStatus( SERVICE_STOPPED );
            delete _pService;
            _pService = NULL;
            break;
        default:
            break;
    }
}

void WINAPI ServiceMain( DWORD dwArgc, LPTSTR* pszArgv )
{
    g_Name                      = SERVICE_NAME;

    g_StatusHandle              = NULL;
    g_Status.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    g_Status.dwCurrentState     = SERVICE_START_PENDING;
    g_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

    g_Status.dwWin32ExitCode           = NO_ERROR;
    g_Status.dwServiceSpecificExitCode = 0;
    g_Status.dwCheckPoint              = 0;
    g_Status.dwWaitHint                = 0;

    g_StatusHandle = RegisterServiceCtrlHandler( g_Name, ServiceCtrlHandler );
    if (g_StatusHandle)
    {
        try
        {
            SetStatus(SERVICE_START_PENDING);
            tstring error;
            if( _pService->Run( g_StrIniPath, error, true ) )
            {
                SetStatus(SERVICE_RUNNING);
            }
            else
            {
                std::string ansiError( error.begin(), error.end() );
                DWORD dwError = ::GetLastError();
                WriteErrorLogEntry( error.c_str(), dwError );
                SetStatus(SERVICE_STOPPED, dwError);
                delete _pService;
                _pService = NULL;
            }
        }
        catch (...)
        {
            WriteEventLogEntry(_T("Service: Failed to start."), EVENTLOG_ERROR_TYPE);
            SetStatus(SERVICE_STOPPED);
        }
    }
}

int _cdecl wmain ( _In_ int argc, _In_reads_(argc) TCHAR* argv[] )
{
    g_StrIniPath = _T("cebackup.ini");

    if( argc > 1 )
    {
        if( _tcscmp( argv[1], _T("-ini") ) != 0 )
        {
            tstring err = tstring( _T("-ini parameter absent, ")) + argv[1] ;
            WriteEventLogEntry( err.c_str() );
            return 1;
        }
        
        if( argc > 3 )
        {
            WriteEventLogEntry( _T("Wrong number of parameters") );
            return 1;
        }

        g_StrIniPath = argv[2];
    }

    _pService = new CBackupClient();
    SERVICE_TABLE_ENTRY serviceTable[] = { { g_Name, ServiceMain }, { NULL, NULL } };
    StartServiceCtrlDispatcher( serviceTable );

    return 0;
}
