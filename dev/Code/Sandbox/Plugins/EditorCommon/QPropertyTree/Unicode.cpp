/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include "Unicode.h"

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <vector>
# include <QtCore/QString>
#endif

string fromWideChar(const wchar_t* wstr)
{
  // We have different implementation for windows as Qt for windows 
  // is built with wchar_t of diferent size (4 bytes, as on linux).
  // Therefore we avoid calling any wchar_t functions in Qt.
#ifdef _WIN32
    const unsigned int codepage = CP_UTF8;
    int len = WideCharToMultiByte(codepage, 0, wstr, -1, NULL, 0, 0, 0);
    char* buf = (char*)alloca(len);
    if (len > 1) {
        WideCharToMultiByte(codepage, 0, wstr, -1, buf, len, 0, 0);
        return string(buf, len - 1);
    }
    return string();
#else
    return QString::fromWCharArray(wstr).toUtf8().data();
#endif
}

wstring toWideChar(const char* str)
{
#ifdef _WIN32
    const unsigned int codepage = CP_UTF8;
    int len = MultiByteToWideChar(codepage, 0, str, -1, NULL, 0);
    wchar_t* buf = (wchar_t*)alloca(len * sizeof(wchar_t));
    if (len > 1) {
        MultiByteToWideChar(codepage, 0, str, -1, buf, len);
        return wstring(buf, len - 1);
    }
    return wstring();
#else
    QString s = QString::fromUtf8(str);

    std::vector<wchar_t> result(s.size()+1);
    s.toWCharArray(&result[0]);
    return &result[0];
#endif
}

