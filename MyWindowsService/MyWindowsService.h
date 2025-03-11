#pragma once

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <direct.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <chrono>
#include <ctime>

#include "MyWindowsServiceMessage.h"
#include "FormatMessage.h"

#define LogInfo(x) \
  if (pMyCout) { \
    *pMyCout << L"[" << getCurrentDateTime(L"-", L" ", L":") << L"] " << x; \
  }
#define LogError(x) \
  if (pMyCerr) { \
    *pMyCerr << L"[" << getCurrentDateTime(L"-", L" ", L":") << L"] " << x; \
  }
#define LogFileFlush() \
  if (pMyCout) { \
    pMyCout->flush(); \
  }
#define LogFileSet(f) \
  if (pMyCout) { \
    pMyCout = &f; \
  }
#define ErrorFileSet(f) \
  if (pMyCerr) { \
    pMyCerr = &f; \
  }

void GetFileName(_TCHAR * strDirNameBuffer, size_t intDirNameBufferSize, const _TCHAR * strFullPathName, const _TCHAR cDirectorySeparator = _T('\\'), const _TCHAR cExtensionSeparator = _T('.'));
void GetDirectoryName(_TCHAR * strDirNameBuffer, size_t intDirNameBufferSize, const _TCHAR * strFullPathName, const _TCHAR cDirectorySeparator = _T('\\'));
void GetCurrentExecutableDirectoryAndFileName(_TCHAR * strDirNameBuffer, size_t intDirNameBufferSize, _TCHAR * strExecutableFileName, size_t intExecutableFileName);
DWORD CallStopCommand(DWORD dwRunningProcessId);
class logStream {
public:
  logStream(std::wostream& os1, std::wostream& os2) : m_os1(os1), m_os2(os2) {
  }

  template<class T>
  logStream& operator<<(const T& x) {
    m_os1 << x;
    m_os2 << x;
    return *this;
  }
  ~logStream() {
    // TODO flush/close
  }
private:
  std::wostream& m_os1;
  std::wostream& m_os2;
};

inline void ltrim(std::wstring& s);
inline void rtrim(std::wstring& s);
inline void trim(std::wstring& s);

std::wstring getCurrentDate(std::wstring strSeparator = L"");
std::wstring getCurrentDateTime(std::wstring strDateSeparator = L"", std::wstring strDateTimeSeparator = L"", std::wstring strTimeSeparator = L"");
VOID SvcReportEvent(LPCWSTR, DWORD = NO_ERROR);
