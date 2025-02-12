#include "FormatMessage.h"

CFormatMessage::CFormatMessage(DWORD dwErrorCode) : m_dwErrorCode(NO_ERROR), m_strFullErrorText(NULL), m_strErrorText(NULL) {
  m_dwErrorCode = dwErrorCode;

  // Easy way: use _com_error and its .ErrorMessage() method
  _com_error error(dwErrorCode);
  const wchar_t *strErrorText = error.ErrorMessage();
  // Allocate and copy error message
  size_t intErrorTextSizeInWords = wcslen(strErrorText);
  m_strErrorText = new wchar_t[intErrorTextSizeInWords + 1];
  wcscpy_s(m_strErrorText, intErrorTextSizeInWords + 1, strErrorText);

  // Allocate and copy full error message (error code and message)
  std::wstring strErrorCode = std::to_wstring(m_dwErrorCode);
  std::wstring strFullErrorCode = L"Error Code ";
  strFullErrorCode += strErrorCode + L" : \"" + m_strErrorText + L"\"";

  size_t intFullErrorTextLength = wcslen(strFullErrorCode.c_str());
  m_strFullErrorText = new wchar_t[intFullErrorTextLength + 1];
  wcscpy_s(m_strFullErrorText, intFullErrorTextLength + 1, strFullErrorCode.c_str());
}

CFormatMessage::~CFormatMessage() {
  m_dwErrorCode = NO_ERROR;
  delete[] m_strErrorText;
  delete[] m_strFullErrorText;
}

const wchar_t* CFormatMessage::GetText() {
  return m_strErrorText;
}

const wchar_t* CFormatMessage::GetFullText() {
  return m_strFullErrorText;
}
