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
wchar_t strLogPrefix[MAX_PATH];
wchar_t strLogFile[MAX_PATH];
wchar_t strErrorFile[MAX_PATH];
HANDLE hEventSource = NULL;
std::wostream *pMyCout = &std::wcout;
std::wostream *pMyCerr = &std::wcerr;
wchar_t strConfigurationFile[MAX_PATH];
std::unordered_map<std::wstring, std::wstring> Configuration;

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR*);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR*);

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
  SvcReportEvent(L"Start service...");

  // Load default configuration
  LogInfo(L"Load configuration..." << L'\n');
  // Get current binary info (path and name)
  GetCurrentExecutableDirectoryAndFileName(strWorkingDirectory, sizeof(strWorkingDirectory) / sizeof(wchar_t), strExecutableName, sizeof(strExecutableName) / sizeof(wchar_t));
  LogInfo(TEXT("executableName = \"") << strExecutableName << TEXT("\"") << L'\n');
  LogInfo(TEXT("workingDirectory = \"") << strWorkingDirectory << TEXT("\"") << L'\n');

  // By default we log into current working directory (unless another directory is specified in configuration file)
  Configuration[L"SERVICE_LOG_FOLDER"] = strWorkingDirectory;

  // By default the service name (id) is the executable name
  Configuration[L"SERVICE_NAME"] = strExecutableName;

  // Build configuration file path
  strConfigurationFile[0] = _T('\0');
  wcscat_s(strConfigurationFile, sizeof(strConfigurationFile) / sizeof(wchar_t), strWorkingDirectory);
  wcscat_s(strConfigurationFile, sizeof(strConfigurationFile) / sizeof(wchar_t), strExecutableName);
  wcscat_s(strConfigurationFile, sizeof(strConfigurationFile) / sizeof(wchar_t), L".config");

  // Build default start command full path : part1 + working directory + '_start.bat'
  strStartCommand[0] = _T('\0');
  wcscat_s(strStartCommand, sizeof(strStartCommand) / sizeof(wchar_t), strDefaultStartCommand1);
  wcscat_s(strStartCommand, sizeof(strStartCommand) / sizeof(wchar_t), strWorkingDirectory);
  wcscat_s(strStartCommand, sizeof(strStartCommand) / sizeof(wchar_t), strDefaultStartCommand2);
  Configuration[L"SERVICE_START_COMMAND"] = strStartCommand;
  // Build default stop command full path : part1 + working directory + '_stop.bat'
  strStopCommand[0] = _T('\0');
  wcscat_s(strStopCommand, sizeof(strStopCommand) / sizeof(wchar_t), strDefaultStopCommand1);
  wcscat_s(strStopCommand, sizeof(strStopCommand) / sizeof(wchar_t), strWorkingDirectory);
  wcscat_s(strStopCommand, sizeof(strStopCommand) / sizeof(wchar_t), strDefaultStopCommand2);
  Configuration[L"SERVICE_STOP_COMMAND"] = strStopCommand;

  // Load configuration file (to override defaults)
  LogInfo(L"Loading configuration file \"" << strConfigurationFile << L"\"..." << L'\n');
  std::wifstream configurationFile(strConfigurationFile);
  if (!configurationFile.is_open()) {
    LogError(TEXT("Can't open configuration file \"") << strConfigurationFile << TEXT("\") : ") << CFormatMessage(GetLastError()).GetFullText() << TEXT(")") << L'\n');
    return 1;
  }
  std::wstring configLine;
  while (std::getline(configurationFile, configLine)) {
    std::wistringstream is_line(configLine);
    std::wstring key, value;

    // Read key and value (separator is the equal sign '=')
    if (std::getline(std::getline(is_line, key, L'='), value)) {
      // Trim value : remove spaces at the beginning and at the end
      trim(value);
      Configuration[key] = value;
    }
  }
  configurationFile.close();

  // Afficher les valeurs lues
  LogInfo(TEXT(".config[") << L"SERVICE_NAME" << TEXT("] = \"") << Configuration[L"SERVICE_NAME"] << TEXT("\"") << L'\n');
  LogInfo(TEXT(".config[") << L"SERVICE_LABEL" << TEXT("] = \"") << Configuration[L"SERVICE_LABEL"] << TEXT("\"") << L'\n');
  LogInfo(TEXT(".config[") << L"SERVICE_PATH" << TEXT("] = \"") << Configuration[L"SERVICE_PATH"] << TEXT("\"") << L'\n');
  LogInfo(TEXT(".config[") << L"SERVICE_LOGIN" << TEXT("] = \"") << Configuration[L"SERVICE_LOGIN"] << TEXT("\"") << L'\n');
  LogInfo(TEXT(".config[") << L"SERVICE_LOG_FOLDER" << TEXT("] = \"") << Configuration[L"SERVICE_LOG_FOLDER"] << TEXT("\"") << L'\n');

  // Build prefix for log and error files : log folder + '\\' + year + month + date + '.' + service name
  wcscpy_s(strLogPrefix, sizeof(strLogPrefix) / sizeof(wchar_t), Configuration[L"SERVICE_LOG_FOLDER"].c_str());
  if (Configuration[L"SERVICE_LOG_FOLDER"].back() != L'\\') {
    wcscat_s(strLogPrefix, sizeof(strLogPrefix) / sizeof(wchar_t), L"\\");
  }
  wcscat_s(strLogPrefix, sizeof(strLogPrefix) / sizeof(wchar_t), getCurrentDate().c_str());
  wcscat_s(strLogPrefix, sizeof(strLogPrefix) / sizeof(wchar_t), L".");
  wcscat_s(strLogPrefix, sizeof(strLogPrefix) / sizeof(wchar_t), Configuration[L"SERVICE_NAME"].c_str());
  // Build log file name by appending '.log'
  wcscpy_s(strLogFile, sizeof(strLogFile) / sizeof(wchar_t), strLogPrefix);
  wcscat_s(strLogFile, sizeof(strLogFile) / sizeof(wchar_t), L".log");
  LogInfo(TEXT("logFile = \"") << strLogFile << TEXT("\"") << L'\n');
  // Build error file name by appending '.err'
  wcscpy_s(strErrorFile, sizeof(strErrorFile) / sizeof(wchar_t), strLogPrefix);
  wcscat_s(strErrorFile, sizeof(strErrorFile) / sizeof(wchar_t), L".err");
  LogInfo(TEXT("errorFile = \"") << strErrorFile << TEXT("\"") << L'\n');

  LogInfo(TEXT(".config[") << L"SERVICE_START_COMMAND" << TEXT("] = \"") << Configuration[L"SERVICE_START_COMMAND"] << TEXT("\"") << L'\n');
  LogInfo(TEXT(".config[") << L"SERVICE_STOP_COMMAND" << TEXT("] = \"") << Configuration[L"SERVICE_STOP_COMMAND"] << TEXT("\"") << L'\n');
  // Reset strStartCommand with loaded configuration
  wcscpy_s(strStartCommand, sizeof(strStartCommand) / sizeof(wchar_t), Configuration[L"SERVICE_START_COMMAND"].c_str());
  // Reset strStopCommand with loaded configuration
  wcscpy_s(strStopCommand, sizeof(strStopCommand) / sizeof(wchar_t), Configuration[L"SERVICE_STOP_COMMAND"].c_str());
  
  std::wofstream logFile(strLogFile);
  if (!logFile.is_open()) {
    LogError(TEXT("Can't open log file \"") << strLogFile << TEXT("\") : ") << CFormatMessage(GetLastError()).GetFullText() << TEXT(")") << L'\n');
    return 1;
  }
  std::wofstream errorFile(strErrorFile);
  if (!errorFile.is_open()) {
    LogError(TEXT("Can't open error file \"") << strErrorFile << TEXT("\") : ") << CFormatMessage(GetLastError()).GetFullText() << TEXT(")") << L'\n');
    return 1;
  }

  // Redirect output and error to files
  LogFileSet(logFile);
  ErrorFileSet(errorFile);

  LogInfo(L"Service " << Configuration[L"SERVICE_NAME"] << L" starting..." << L'\n');
  LogInfo(L"Loaded configuration file \"" << strConfigurationFile << L"\"" << L'\n');

  // Change working directory (to same path as executable, if not specified in configuration file)
  if (_wchdir((const wchar_t*)strWorkingDirectory)) {
    LogError(TEXT("chdir(\"") << strWorkingDirectory << TEXT("\") failed (") << CFormatMessage(GetLastError()).GetFullText() << TEXT(")") << L'\n');
    return 1;
  }
  LogInfo(TEXT("chdir(\"") << strWorkingDirectory << TEXT("\") : OK.") << L'\n');

  LogFileFlush();
  // If command-line parameter is "debug", start the service directly (for debugging)
  if (argc > 1 && lstrcmpi(argv[1], TEXT("debug")) == 0)
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
    SvcReportEvent((LPWSTR)TEXT("StartServiceCtrlDispatcher"), GetLastError());
  }
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
    SvcReportEvent((LPWSTR)TEXT("RegisterServiceCtrlHandler"), GetLastError());
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
  SvcReportEvent(L"Service started.");

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
      LogFileFlush();

      // Check sub-process exit code - if not 0 propagate error
      DWORD exitCode = 0;
      if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
        if (exitCode == 0) {
          LogInfo(TEXT("Service sub-process PID ") << pi.dwProcessId << TEXT(" terminated with exit code ") << exitCode << L'\n');
          LogFileFlush();
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
        LogFileFlush();
      }
    }
    break;

    // ghEvents[1] was signaled (someone requested service to shut down)
    case WAIT_OBJECT_0 + 1:
      LogInfo(TEXT("Service shutdown requested.") << L'\n');
      LogFileFlush();
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

  SvcReportEvent(L"Service stopped.");
  ReportSvcStatus(SERVICE_STOPPED, dwServiceExitCode, 0);
  LogFileFlush();
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
    SvcReportEvent(L"Service stop received...");
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
VOID SvcReportEvent(LPCWSTR szFunction, DWORD reportErrorCode)
{
  HANDLE hEventSource;
  LPCWSTR lpszStrings[2];
  wchar_t Buffer[4096];
  WORD eventType = EVENTLOG_SUCCESS;
  DWORD eventId = SVC_INFO;

  hEventSource = RegisterEventSource(NULL, strServiceName);

  if (NULL != hEventSource)
  {
    if (reportErrorCode == NO_ERROR) {
      StringCchPrintf(Buffer, sizeof(Buffer) / sizeof(wchar_t), L"%ls", szFunction);
    } else {
      eventType = EVENTLOG_ERROR_TYPE;
      eventId = SVC_ERROR;
      StringCchPrintf(Buffer, sizeof(Buffer) / sizeof(wchar_t), L"%ls failed with error %ls", szFunction, CFormatMessage(reportErrorCode).GetFullText());
    }

    lpszStrings[0] = strServiceName;
    lpszStrings[1] = Buffer;

    ReportEvent(hEventSource,        // event log handle
      eventType, // event type
      0,                   // event category
      eventId,           // event identifier
      NULL,                // no security identifier
      2,                   // size of lpszStrings array
      0,                   // no binary data
      lpszStrings,         // array of strings
      NULL);               // no binary data

    DeregisterEventSource(hEventSource);
  }
}

void GetDirectoryName(_TCHAR* strDirNameBuffer, size_t intDirNameBufferSize, const _TCHAR* strFullPathName, const _TCHAR cDirectorySeparator)
{
  if (strFullPathName != NULL)
  {
    const _TCHAR* strFileNameStart = _tcsrchr(strFullPathName, (int)cDirectorySeparator);
    if (strFileNameStart == NULL)
    {
      // Aucun séparateur de répertoire: on utilise le chemin relatif "répertoire courant" = "."
      wcscpy_s(strDirNameBuffer, intDirNameBufferSize, _T(".\\"));
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
    }
  }
}

void GetFileName(_TCHAR* strExecutableFileNameBuffer, size_t intExecutableFileNameBufferSize, const _TCHAR* strFullPathName, const _TCHAR cDirectorySeparator, const _TCHAR cExtensionSeparator)
{
  if (strFullPathName != NULL)
  {
    // Search last folder separator in full path (reverse search)
    const _TCHAR* strFileNameStart = _tcsrchr(strFullPathName, (int)cDirectorySeparator);
    if (strFileNameStart == NULL)
    {
      // No separator found : use default binary name DEFAULT_SVCNAME
      wcscpy_s(strExecutableFileNameBuffer, intExecutableFileNameBufferSize, DEFAULT_SVCNAME);
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
    }
  }
}

void GetCurrentExecutableDirectoryAndFileName(_TCHAR* strDirNameBuffer, size_t intDirNameBufferSize, _TCHAR* strExecutableFileNameBuffer, size_t intExecutableFileNameBufferSize)
{
  const _TCHAR* strDirectory = _T(".\\");
  const _TCHAR* strExecutable = _T("MyWindowsService.exe");

  _TCHAR strModuleFileName[MAX_PATH + 1];
  DWORD dwSize = GetModuleFileName(NULL, strModuleFileName, MAX_PATH);
  if (dwSize > 0)
  {
    GetDirectoryName(strDirNameBuffer, intDirNameBufferSize, strModuleFileName);
    GetFileName(strExecutableFileNameBuffer, intExecutableFileNameBufferSize, strModuleFileName);
  } else {
    wcsncpy_s(strDirNameBuffer, intDirNameBufferSize, strDirectory, wcslen(strDirectory));
    wcsncpy_s(strExecutableFileNameBuffer, intExecutableFileNameBufferSize, strExecutable, wcslen(strExecutable));
  }
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
  LogFileFlush();

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
    LogFileFlush();

    // Check sub-process exit code - if not 0 propagate error
    DWORD exitCode = 0;
    if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
      if (exitCode == 0) {
        LogInfo(TEXT("Stop command sub-process PID ") << pi.dwProcessId << TEXT(" terminated with exit code ") << exitCode << L'\n');
        LogFileFlush();
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
  LogFileFlush();
  return dwServiceExitCode;
}

// trim from start (in place)
inline void ltrim(std::wstring& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
    return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::wstring& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
    return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::wstring& s) {
  rtrim(s);
  ltrim(s);
}

std::wstring getCurrentDate(std::wstring strSeparator) {
  // Obtenir le temps actuel
  std::time_t now = std::time(nullptr);

  // Structure tm pour stocker les composants de la date
  std::tm tmBuffer;

  // Format de sortie : séparateur personnalisable (par défaut : rien!)
  std::wstring dateFormat = L"%Y" + strSeparator + L"%m" + strSeparator + L"%d";

  // Convertir le temps en structure tm de manière sécurisée
  if (localtime_s(&tmBuffer, &now) == 0) {
    // Créer un flux pour formater la date
    std::wostringstream oss;
    oss << std::put_time(&tmBuffer, dateFormat.c_str());
    return oss.str();
  } else {
    return L"";
  }
}
