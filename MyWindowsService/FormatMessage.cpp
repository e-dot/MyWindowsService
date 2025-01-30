#include "FormatMessage.h"

CFormatMessage::CFormatMessage(DWORD dwErrorCode) {
  m_dwErrorCode = dwErrorCode;

  // Easy way: use _com_error and its .ErrorMessage() method
  _com_error error(dwErrorCode);
  const wchar_t *strErrorText = error.ErrorMessage();
  // Allocate and copy error message
  size_t intErrorTextSizeInWords = wcslen(strErrorText);
  m_strErrorText = new wchar_t[intErrorTextSizeInWords + 1];
  wcscpy_s(m_strErrorText, intErrorTextSizeInWords + 1, strErrorText);
}

CFormatMessage::~CFormatMessage() {
  m_dwErrorCode = NO_ERROR;
  delete[] m_strErrorText;
}

const wchar_t* CFormatMessage::GetText() {
  return m_strErrorText;
}
