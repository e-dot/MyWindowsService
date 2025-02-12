// $Id: MyWindowsService.cpp 88962 2025-01-28 16:22:18Z emmanuelka $

#include "MyWindowsService.h"

#pragma comment(lib, "advapi32.lib")

// Default Configuration
#define DEFAULT_SVCNAME (LPWSTR)TEXT("MyWindowsService")
wchar_t* strServiceName = DEFAULT_SVCNAME;
wchar_t* strServiceLabel = DEFAULT_SVCNAME;
wchar_t strStartCommand[32767];
wchar_t* strDefaultStartCommand1 = (LPWSTR)TEXT("cmd.exe /C ");
wchar_t* strDefaultStartCommand2 = (LPWSTR)TEXT("_start.bat");
wchar_t strStopCommand[32767];
wchar_t* strDefaultStopCommand1 = (LPWSTR)TEXT("cmd.exe /C ");
wchar_t* strDefaultStopCommand2 = (LPWSTR)TEXT("_stop.bat");
wchar_t strWorkingDirectory[MAX_PATH];
wchar_t strExecutableName[MAX_PATH];
wchar_t* strLogDirectory = (LPWSTR)TEXT(".");
HANDLE hEventSource = NULL;
std::wostream *pMyCout = &std::wcout;
std::wostream *pMyCerr = &std::wcerr;
wchar_t strConfigurationFile[MAX_PATH];
std::unordered_map<std::string, std::string> Configuration;

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

VOID SvcInstall(void);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR*);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR*);
VOID SvcReportEvent(LPTSTR);

//
// Purpose: 
//   Entry point for the process
//
// Parameters:
//   None
// 
// Return value:
//   None, defaults to 0 (zero)
//
int __cdecl _tmain(int argc, wchar_t* argv[])
{
  LogInfo(L"Start service..." << L'\n');

  // Load default configuration
  LogInfo(L"Load configuration..." << L'\n');
  // Get current binary info (path and name)
  GetCurrentExecutableDirectoryAndFileName(strWorkingDirectory, sizeof(strWorkingDirectory) / sizeof(wchar_t), strExecutableName, sizeof(strExecutableName) / sizeof(wchar_t));
  LogInfo(TEXT("executableName = \"") << strExecutableName << TEXT("\"") << L'\n');
  LogInfo(TEXT("workingDirectory = \"") << strWorkingDirectory << TEXT("\"") << L'\n');

  // Build configuration file path
  strConfigurationFile[0] = _T('\0');
  wcscat_s(strConfigurationFile, sizeof(strConfigurationFile) / sizeof(wchar_t), strWorkingDirectory);
  wcscat_s(strConfigurationFile, sizeof(strConfigurationFile) / sizeof(wchar_t), strExecutableName);
  wcscat_s(strConfigurationFile, sizeof(strConfigurationFile) / sizeof(wchar_t), L".config");

  // Load configuration file
  std::ifstream configurationFile(strConfigurationFile);
  if (!configurationFile.is_open()) {
    LogError(TEXT("Can't open configuration file \"") << strConfigurationFile << TEXT("\") : ") << CFormatMessage(GetLastError()).GetFullText() << TEXT(")") << L'\n');
    return 1;
  }

  // Build start command full path : part1 + working directory + '_start.bat'
  strStartCommand[0] = _T('\0');
  wcscat_s(strStartCommand, sizeof(strStartCommand) / sizeof(wchar_t), strDefaultStartCommand1);
  wcscat_s(strStartCommand, sizeof(strStartCommand) / sizeof(wchar_t), strWorkingDirectory);
  wcscat_s(strStartCommand, sizeof(strStartCommand) / sizeof(wchar_t), strDefaultStartCommand2);
  // Build stop command full path : part1 + working directory + '_stop.bat'
  strStopCommand[0] = _T('\0');
  wcscat_s(strStopCommand, sizeof(strStopCommand) / sizeof(wchar_t), strDefaultStopCommand1);
  wcscat_s(strStopCommand, sizeof(strStopCommand) / sizeof(wchar_t), strWorkingDirectory);
  wcscat_s(strStopCommand, sizeof(strStopCommand) / sizeof(wchar_t), strDefaultStopCommand2);
  // TODO Load configuration file

  LogInfo(TEXT("startCommand = \"") << strStartCommand << TEXT("\"") << L'\n');
  LogInfo(TEXT("stopCommand = \"") << strStopCommand << TEXT("\"") << L'\n');

  // Change working directory (to same path as executable, if not specified in configuration file)
  LogInfo(L"Change working directory..." << L'\n');
  if (_wchdir((const wchar_t*)strWorkingDirectory)) {
    LogError(TEXT("chdir(\"") << strWorkingDirectory << TEXT("\") failed (") << CFormatMessage(GetLastError()).GetFullText() << TEXT(")") << L'\n');
    return 1;
  }
  LogInfo(TEXT("chdir(\"") << strWorkingDirectory << TEXT("\") : OK.") << L'\n');

  // If command-line parameter is "install", install the service. 
  // Otherwise, the service is probably being started by the SCM.
  if (lstrcmpi(argv[1], TEXT("install")) == 0)
  {
    SvcInstall();
    return 0;
  }

  // If command-line parameter is "debug", start the service directly (for debugging)
  if (lstrcmpi(argv[1], TEXT("debug")) == 0)
  {
    SvcInit(argc, argv);
    return 0;
  }
  // Otherwise, the service is probably being started by the SCM.

  // This call returns when the service has stopped. 
  // The process should simply terminate when the call returns.

  SERVICE_TABLE_ENTRY DispatchTable[] =
  {
      { strServiceName, (LPSERVICE_MAIN_FUNCTION)SvcMain },
      { NULL, NULL }
  };
  if (!StartServiceCtrlDispatcher(DispatchTable))
  {
    SvcReportEvent((LPWSTR)TEXT("StartServiceCtrlDispatcher"));
  }
}

//
// Purpose: 
//   Installs a service in the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID SvcInstall()
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  wchar_t szUnquotedPath[MAX_PATH];

  if (!GetModuleFileName(NULL, szUnquotedPath, MAX_PATH))
  {
    LogError(TEXT("Cannot install service (") << CFormatMessage(GetLastError()).GetFullText() << L'\n');
    return;
  }

  // In case the path contains a space, it must be quoted so that
  // it is correctly interpreted. For example,
  // "d:\my share\myservice.exe" should be specified as
  // ""d:\my share\myservice.exe"".
  wchar_t szPath[MAX_PATH];
  StringCbPrintf(szPath, MAX_PATH, TEXT("\"%s\""), szUnquotedPath);

  // Get a handle to the SCM database. 

  schSCManager = OpenSCManager(
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    SC_MANAGER_ALL_ACCESS);  // full access rights 

  if (NULL == schSCManager)
  {
    LogError(TEXT("OpenSCManager failed (") << CFormatMessage(GetLastError()).GetFullText() << TEXT(").") << L'\n');
    return;
  }

  // Create the service

  schService = CreateService(
    schSCManager,              // SCM database 
    strServiceName,                   // name of service 
    strServiceLabel,                   // service name to display 
    SERVICE_ALL_ACCESS,        // desired access 
    SERVICE_WIN32_OWN_PROCESS, // service type 
    SERVICE_DEMAND_START,      // start type 
    SERVICE_ERROR_NORMAL,      // error control type 
    szPath,                    // path to service's binary 
    NULL,                      // no load ordering group 
    NULL,                      // no tag identifier 
    NULL,                      // no dependencies 
    NULL,                      // LocalSystem account 
    NULL);                     // no password 

  if (schService == NULL)
  {
    LogError(TEXT("CreateService failed (") << CFormatMessage(GetLastError()).GetFullText() << TEXT(").") << L'\n');
    CloseServiceHandle(schSCManager);
    return;
  }
  else {
    LogInfo(TEXT("Service installed successfully") << L'\n');
  }

  CloseServiceHandle(schService);
  CloseServiceHandle(schSCManager);
}

//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
  // Register the handler function for the service

  gSvcStatusHandle = RegisterServiceCtrlHandler(
    strServiceName,
    SvcCtrlHandler);

  if (!gSvcStatusHandle)
  {
    SvcReportEvent((LPWSTR)TEXT("RegisterServiceCtrlHandler"));
    return;
  }

  // These SERVICE_STATUS members remain as set here

  gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  gSvcStatus.dwServiceSpecificExitCode = 0;

  // Report initial status to the SCM

  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

  // Perform service-specific initialization and work.

  SvcInit(dwArgc, lpszArgv);
}

//
// Purpose: 
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None
//
VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  // TO_DO: Declare and set any required variables.
  //   Be sure to periodically call ReportSvcStatus() with 
  //   SERVICE_START_PENDING. If initialization fails, call
  //   ReportSvcStatus with SERVICE_STOPPED.

  // Create an event. The control handler function, SvcCtrlHandler,
  // signals this event when it receives the stop control code.

  ghSvcStopEvent = CreateEvent(
    NULL,    // default security attributes
    TRUE,    // manual reset event
    FALSE,   // not signaled
    NULL);   // no name

  if (ghSvcStopEvent == NULL)
  {
    ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
    return;
  }

  // Report running status when initialization is complete.

  ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  // Start the child process. 
  if (!CreateProcess(NULL,   // No module name (use command line)
    strStartCommand,        // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    TRUE,           // Set handle inheritance to TRUE
    0,              // No creation flags
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory 
    &si,            // Pointer to STARTUPINFO structure
    &pi)           // Pointer to PROCESS_INFORMATION structure
    )
  {
    DWORD dwCreateError = GetLastError();
    LogError(TEXT("CreateProcess(\"") << strStartCommand << TEXT("\") failed (") << CFormatMessage(dwCreateError).GetFullText() << TEXT(").") << L'\n');
    ReportSvcStatus(SERVICE_STOPPED, dwCreateError, 0);
    return;
  }

  LogInfo(TEXT("CreateProcess(\"") << strStartCommand << TEXT("\") : PID=") << pi.dwProcessId << L'\n');

  const size_t nCount = 2;
  HANDLE ghEvents[nCount];
  ghEvents[0] = pi.hProcess;
  ghEvents[1] = ghSvcStopEvent;
  BOOL bWaitAll = FALSE; // Stop waiting whenever ANY event occurs (sub-process ends OR service shutdown requested)
  BOOL bContinue = TRUE;
  DWORD dwServiceExitCode = NO_ERROR;
  while (bContinue)
  {

    // Wait until child process exits AND Check whether to stop the service
    DWORD dwEvent = WaitForMultipleObjects(nCount, ghEvents, bWaitAll, INFINITE);

    switch (dwEvent)
    {
      // ghEvents[0] was signaled (sub-process terminated)
    case WAIT_OBJECT_0 + 0:
    {
      LogInfo(TEXT("Service sub-process PID ") << pi.dwProcessId << TEXT(" terminated.") << L'\n');

      // Check sub-process exit code - if not 0 propagate error
      DWORD exitCode = 0;
      if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        if (exitCode == 0) {
          LogInfo(TEXT("Service sub-process PID ") << pi.dwProcessId << TEXT(" terminated with exit code ") << exitCode << L'\n');
        }
        else {
          LogError(TEXT("Service sub-process PID ") << pi.dwProcessId << TEXT(" failed with exit code ") << exitCode << L'\n');
          // Propagate error : exit service
          dwServiceExitCode = exitCode;
          bContinue = FALSE;
        }
      }
      else {
        LogInfo(TEXT("Can't get exit code for service sub-process PID ") << pi.dwProcessId << TEXT(" (") << CFormatMessage(GetLastError()).GetFullText() << L'\n');
      }
    }
    break;

    // ghEvents[1] was signaled (someone requested service to shut down)
    case WAIT_OBJECT_0 + 1:
      LogInfo(TEXT("Service shutdown requested.") << L'\n');
      // if service stop is request, call the strStopCommand
      CallStopCommand(pi.dwProcessId);
      // stop infinite loop
      bContinue = FALSE;
      break;

      // Return value is invalid.
    default:
      LogError(TEXT("Wait error: ") << CFormatMessage(GetLastError()).GetFullText() << L'\n');
    }


  } // while

  // Close process and thread handles. 
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  ReportSvcStatus(SERVICE_STOPPED, dwServiceExitCode, 0);
  return;
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState,
  DWORD dwWin32ExitCode,
  DWORD dwWaitHint)
{
  static DWORD dwCheckPoint = 1;

  // Fill in the SERVICE_STATUS structure.

  gSvcStatus.dwCurrentState = dwCurrentState;
  gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
  gSvcStatus.dwWaitHint = dwWaitHint;

  if (dwCurrentState == SERVICE_START_PENDING)
    gSvcStatus.dwControlsAccepted = 0;
  else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

  if ((dwCurrentState == SERVICE_RUNNING) ||
    (dwCurrentState == SERVICE_STOPPED))
    gSvcStatus.dwCheckPoint = 0;
  else gSvcStatus.dwCheckPoint = dwCheckPoint++;

  // Report the status of the service to the SCM.
  SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
  // Handle the requested control code. 

  switch (dwCtrl)
  {
  case SERVICE_CONTROL_STOP:
    ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

    // Signal the service to stop.

    SetEvent(ghSvcStopEvent);
    ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

    return;

  case SERVICE_CONTROL_INTERROGATE:
    break;

  default:
    break;
  }

}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPTSTR szFunction)
{
  HANDLE hEventSource;
  LPCTSTR lpszStrings[2];
  wchar_t Buffer[80];

  hEventSource = RegisterEventSource(NULL, strServiceName);

  if (NULL != hEventSource)
  {
    StringCchPrintf(Buffer, 80, TEXT("%s failed with %S"), szFunction, CFormatMessage(GetLastError()).GetFullText());

    lpszStrings[0] = strServiceName;
    lpszStrings[1] = Buffer;

    ReportEvent(hEventSource,        // event log handle
      EVENTLOG_ERROR_TYPE, // event type
      0,                   // event category
      SVC_ERROR,           // event identifier
      NULL,                // no security identifier
      2,                   // size of lpszStrings array
      0,                   // no binary data
      lpszStrings,         // array of strings
      NULL);               // no binary data

    DeregisterEventSource(hEventSource);
  }
}

const _TCHAR* GetDirectoryName(_TCHAR* strDirNameBuffer, size_t intDirNameBufferSize, const _TCHAR* strFullPathName, const _TCHAR cDirectorySeparator)
{
  const _TCHAR* strDirName = NULL;

  if (strFullPathName != NULL)
  {
    const _TCHAR* strFileNameStart = _tcsrchr(strFullPathName, (int)cDirectorySeparator);
    if (strFileNameStart == NULL)
    {
      // Aucun séparateur de répertoire: on utilise le chemin relatif "répertoire courant" = "."
      wcscpy_s(strDirNameBuffer, intDirNameBufferSize, _T(".\\"));
      strDirName = strDirNameBuffer;
    }
    else
    {
      // On copie jusqu'avant le dernier séparateur de répertoire
      size_t nDirectorySize = strFileNameStart + 1 - strFullPathName;
      if (nDirectorySize > intDirNameBufferSize)
      {
        nDirectorySize = intDirNameBufferSize;
      }
      wcsncpy_s(strDirNameBuffer, intDirNameBufferSize, strFullPathName, nDirectorySize);
      strDirNameBuffer[nDirectorySize] = _T('\0');
      strDirName = strDirNameBuffer;
    }
  }

  return(strDirName);
}

const _TCHAR* GetFileName(_TCHAR* strExecutableFileNameBuffer, size_t intExecutableFileNameBufferSize, const _TCHAR* strFullPathName, const _TCHAR cDirectorySeparator, const _TCHAR cExtensionSeparator)
{
  const _TCHAR* strFileName = NULL;

  if (strFullPathName != NULL)
  {
    // Search last folder separator in full path (reverse search)
    const _TCHAR* strFileNameStart = _tcsrchr(strFullPathName, (int)cDirectorySeparator);
    if (strFileNameStart == NULL)
    {
      // No separator found : use default binary name DEFAULT_SVCNAME
      wcscpy_s(strExecutableFileNameBuffer, intExecutableFileNameBufferSize, DEFAULT_SVCNAME);
      strFileName = strExecutableFileNameBuffer;
    }
    else
    {
      // Search last extension separator (dot = ".")
      const _TCHAR *strFileExtension = _tcsrchr(strFullPathName, (int)cExtensionSeparator);
      // Copy between last separator to last dot
      size_t nFileNameSize = strFileExtension - strFileNameStart - 1;
      if (nFileNameSize <= 0)
      {
        // Last dot is before last backslash : no extension in file name, copy up to the end of string
        nFileNameSize = wcsnlen_s(strFileNameStart, intExecutableFileNameBufferSize);
      }
      if (nFileNameSize >= intExecutableFileNameBufferSize) {
        nFileNameSize = intExecutableFileNameBufferSize - 1;
      }
      wcsncpy_s(strExecutableFileNameBuffer, intExecutableFileNameBufferSize, strFileNameStart + 1, nFileNameSize);
      strExecutableFileNameBuffer[nFileNameSize + 1] = _T('\0');
      strFileName = strExecutableFileNameBuffer;
    }
  }

  return(strFileName);
}

const _TCHAR* GetCurrentExecutableDirectoryAndFileName(_TCHAR* strDirNameBuffer, size_t intDirNameBufferSize, _TCHAR* strExecutableFileNameBuffer, size_t intExecutableFileNameBufferSize)
{
  const _TCHAR* strDirectory = _T(".\\");
  const _TCHAR* strExecutable = _T("MyWindowsService.exe");

  _TCHAR strModuleFileName[MAX_PATH + 1];
  DWORD dwSize = GetModuleFileName(NULL, strModuleFileName, MAX_PATH);
  if (dwSize > 0)
  {
    strDirectory = GetDirectoryName(strDirNameBuffer, intDirNameBufferSize, strModuleFileName);
    strExecutable = GetFileName(strExecutableFileNameBuffer, intExecutableFileNameBufferSize, strModuleFileName);
  }

  return(strDirectory);
}

DWORD CallStopCommand(DWORD dwRunningProcessId) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  // Start the child process. 
  if (!CreateProcess(NULL,   // No module name (use command line)
    strStopCommand,        // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    TRUE,           // Set handle inheritance to TRUE
    0,              // No creation flags
    NULL,           // Use parent's environment block
    NULL,           // Use parent's starting directory 
    &si,            // Pointer to STARTUPINFO structure
    &pi)           // Pointer to PROCESS_INFORMATION structure
    )
  {
    DWORD dwCreateError = GetLastError();
    LogError(TEXT("CreateProcess(\"") << strStopCommand << TEXT("\") failed (") << CFormatMessage(dwCreateError).GetFullText() << TEXT(").") << L'\n');
    ReportSvcStatus(SERVICE_STOPPED, dwCreateError, 0);
    return dwCreateError;
  }

  LogInfo(TEXT("CreateProcess(\"") << strStopCommand << TEXT("\") : PID=") << pi.dwProcessId);

  DWORD dwStopCommandWaitTimeout = 500000; // TODO use a parameter from configuration file
  DWORD dwServiceExitCode = NO_ERROR;

  // Wait until child process exits AND Check whether to stop the service
  DWORD dwEvent = WaitForSingleObject(pi.hProcess, dwStopCommandWaitTimeout);

  switch (dwEvent)
  {
    // process handle was signaled (sub-process terminated)
  case WAIT_OBJECT_0:
  {
    LogInfo(TEXT("Stop command sub-process PID ") << pi.dwProcessId << TEXT(" terminated.") << L'\n');

    // Check sub-process exit code - if not 0 propagate error
    DWORD exitCode = 0;
    if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
      if (exitCode == 0) {
        LogInfo(TEXT("Stop command sub-process PID ") << pi.dwProcessId << TEXT(" terminated with exit code ") << exitCode << L'\n');
      }
      else {
        LogError(TEXT("Stop command sub-process PID ") << pi.dwProcessId << TEXT("failed with exit code ") << exitCode << L'\n');
        // Propagate error : exit service
        dwServiceExitCode = exitCode;
      }
    }
    else {
      LogError(TEXT("Can't get exit code for service sub-process PID ") << pi.dwProcessId << TEXT("(") << CFormatMessage(GetLastError()).GetFullText() << L'\n');
    }
  }
  break;

  case WAIT_TIMEOUT:
    LogError(TEXT("Stop command sub-process PID ") << pi.dwProcessId << TEXT(" did not exit before timeout of ") << dwStopCommandWaitTimeout << TEXT(" ms") << L'\n');
    // Propagate error : exit service
    dwServiceExitCode = 0xFF;
    break;

    // Return value is invalid.
  default:
    LogError(TEXT("Wait error: ") << CFormatMessage(GetLastError()).GetFullText() << L'\n');
  }

  // Close process and thread handles. 
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  // TODO TerminateProcess(dwRunningProcessId); to workaround stop failure - kill process and all its children

  return dwServiceExitCode;
}
