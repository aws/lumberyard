// UiClipboard is responsible setting and getting clipboard data for the UI elements in a platform-independent way.

#include "StdAfx.h"
#include "UiClipboard.h"
#include <AzCore/PlatformIncl.h>
#include <StringUtils.h>

bool UiClipboard::SetText(const AZStd::string& text)
{
    bool success = false;
#if defined(WIN32)
    if (OpenClipboard(nullptr))
    {
        if (EmptyClipboard())
        {
            if (text.length() > 0)
            {
                auto wstr = CryStringUtils::UTF8ToWStr(text.c_str());
                const SIZE_T buffSize = (wstr.size() + 1) * sizeof(WCHAR);
                if (HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, buffSize))
                {
                    auto buffer = static_cast<WCHAR*>(GlobalLock(hBuffer));
                    memcpy_s(buffer, buffSize, wstr.data(), wstr.size() * sizeof(wchar_t));
                    buffer[wstr.size()] = WCHAR(0);
                    GlobalUnlock(hBuffer);
                    SetClipboardData(CF_UNICODETEXT, hBuffer); // Clipboard now owns the hBuffer
                    success = true;
                }
            }
        }
        CloseClipboard();
    }
#else
    // Do nothing. For now we only support Windows, and don't want errors/warnings during Linux dedicated server compilation
#endif
    return success;
}

AZStd::string UiClipboard::GetText()
{
    AZStd::string outText;
#if defined(WIN32)
    if (OpenClipboard(nullptr))
    {
        if (HANDLE hText = GetClipboardData(CF_UNICODETEXT))
        {
            const WCHAR* text = static_cast<const WCHAR*>(GlobalLock(hText));
            outText = CryStringUtils::WStrToUTF8(text);
            GlobalUnlock(hText);
        }
        CloseClipboard();
}
#else
    // Do nothing. For now we only support Windows, and don't want errors/warnings during Linux dedicated server compilation
#endif
    return outText;
}