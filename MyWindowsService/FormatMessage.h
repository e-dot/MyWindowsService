#pragma once

#include <iostream>
#include <wchar.h>
#include <windows.h>
#include <comdef.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>

const wchar_t* formatMessage(DWORD dwErrorCode);
class CFormatMessage {
public:
  CFormatMessage(DWORD dwErrorCode);
  ~CFormatMessage();
  const wchar_t* GetText();
  const wchar_t* GetFullText();
protected:
  DWORD m_dwErrorCode;
  wchar_t* m_strErrorText;
  wchar_t* m_strFullErrorText;
};
