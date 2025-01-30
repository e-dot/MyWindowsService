// $Id: WindowsService.h 88962 2025-01-28 16:22:18Z emmanuelka $
#pragma once

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <direct.h>
#include <stdarg.h>
#include <iostream>

#include "MyWindowsServiceMessage.h"
#include "FormatMessage.h"

#define LogInfo(x) \
  if (pMyCout) { \
    *pMyCout << x; \
  }
#define LogError(x) \
  if (pMyCout) { \
    *pMyCout << x; \
  }
const _TCHAR* GetCurrentExecutableDirectory(_TCHAR * strDirNameBuffer, size_t intDirNameBufferSize);
const _TCHAR* GetDirectoryName(_TCHAR * strDirNameBuffer, size_t intDirNameBufferSize, const _TCHAR * strFullPathName, const _TCHAR cDirectorySeparator);
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
