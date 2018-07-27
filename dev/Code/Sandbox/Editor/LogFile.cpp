/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : implementation of the CLogFile class.


#include "StdAfx.h"
#include "LogFile.h"
#include "CryEdit.h"

#include "ProcessInfo.h"
#include "Controls/ConsoleSCB.h"
#include "QtUtilWin.h"

#include <QScrollBar>
#include <QListWidget>
#include <QTime>
#include <QScreen>

#include <stdarg.h>

#if !defined(AZ_PLATFORM_WINDOWS)
#include <time.h>
#endif

#if defined(AZ_PLATFORM_APPLE_OSX)
#include <mach/clock.h>
#include <mach/mach.h>

#include <Carbon/Carbon.h>
#endif

#define EDITOR_LOG_FILE "Editor.log"

// Static member variables
SANDBOX_API QListWidget* CLogFile::m_hWndListBox = nullptr;
SANDBOX_API QTextEdit* CLogFile::m_hWndEditBox = nullptr;
bool CLogFile::m_bShowMemUsage = false;
bool CLogFile::m_bIsQuitting = false;

//////////////////////////////////////////////////////////////////////////
SANDBOX_API void Error(const char* format, ...)
{
    va_list ArgList;

    va_start(ArgList, format);
    ErrorV(format, ArgList);
    va_end(ArgList);
}


//////////////////////////////////////////////////////////////////////////
SANDBOX_API void ErrorV(const char* format, va_list argList)
{
    char        szBuffer[MAX_LOGBUFFER_SIZE];
    azvsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, format, argList);

    QString str = "####-ERROR-####: ";
    str += szBuffer;

    //CLogFile::WriteLine( str );
    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, str.toUtf8().data());

    if (!CCryEditApp::instance()->IsInTestMode() && !CCryEditApp::instance()->IsInExportMode() && !CCryEditApp::instance()->IsInLevelLoadTestMode())
    {
        if (gEnv && gEnv->pSystem)
        {
            gEnv->pSystem->ShowMessage(szBuffer, "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
SANDBOX_API void Warning(const char* format, ...)
{
    va_list ArgList;

    va_start(ArgList, format);
    WarningV(format, ArgList);
    va_end(ArgList);
}


//////////////////////////////////////////////////////////////////////////
SANDBOX_API void WarningV(const char* format, va_list argList)
{
    char        szBuffer[MAX_LOGBUFFER_SIZE];
    azvsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, format, argList);

    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, szBuffer);

    bool bNoUI = false;
    ICVar* pCVar = gEnv->pConsole->GetCVar("sys_no_crash_dialog");
    if (pCVar->GetIVal() != 0)
    {
        bNoUI = true;
    }

    if (!CCryEditApp::instance()->IsInTestMode() && !CCryEditApp::instance()->IsInExportMode() && !bNoUI)
    {
        if (gEnv && gEnv->pSystem)
        {
            gEnv->pSystem->ShowMessage(szBuffer, "Warning", MB_OK | MB_ICONWARNING | MB_APPLMODAL);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
SANDBOX_API void Log(const char* format, ...)
{
    va_list ArgList;
    va_start(ArgList, format);
    LogV(format, ArgList);
    va_end(ArgList);
}

//////////////////////////////////////////////////////////////////////////
SANDBOX_API void LogV(const char* format, va_list argList)
{
    char        szBuffer[MAX_LOGBUFFER_SIZE];
    azvsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, format, argList);
    CLogFile::WriteLine(szBuffer);
}



//////////////////////////////////////////////////////////////////////////
const char* CLogFile::GetLogFileName()
{
    if (gEnv && gEnv->pLog)
    {
        // Return the path
        return gEnv->pLog->GetFileName();
    }
    else
    {
        return "";
    }
}

void CLogFile::AttachListBox(QListWidget* hWndListBox) 
{ 
    m_hWndListBox = hWndListBox; 
}

void CLogFile::AttachEditBox(QTextEdit* hWndEditBox) 
{ 
    m_hWndEditBox = hWndEditBox; 
}

void CLogFile::FormatLine(const char * format, ...)
{
    ////////////////////////////////////////////////////////////////////////
    // Write a line with printf style formatting
    ////////////////////////////////////////////////////////////////////////

    va_list ArgList;
    va_start(ArgList, format);
    FormatLineV(format, ArgList);
    va_end(ArgList);
}

void CLogFile::FormatLineV(const char * format, va_list argList)
{
    char szBuffer[MAX_LOGBUFFER_SIZE];
    azvsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, format, argList);
    CryLog("%s", szBuffer);
}


void CLogFile::AboutSystem()
{
#if defined(AZ_PLATFORM_WINDOWS)
    //////////////////////////////////////////////////////////////////////
    // Write the system informations to the log
    //////////////////////////////////////////////////////////////////////

    char szBuffer[MAX_LOGBUFFER_SIZE];
    char szProfileBuffer[128];
    char szLanguageBuffer[64];
    //char szCPUModel[64];
    char* pChar = 0;
    MEMORYSTATUS MemoryStatus;
    DEVMODE DisplayConfig;
    OSVERSIONINFO OSVerInfo;
    OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //////////////////////////////////////////////////////////////////////
    // Display editor and Windows version
    //////////////////////////////////////////////////////////////////////

    // Get system language
    GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE,
        szLanguageBuffer, sizeof(szLanguageBuffer));

    // Format and send OS information line
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, "Current Language: %s ", szLanguageBuffer);
    CryLog("%s", szBuffer);
#else
    QLocale locale;
    CryLog("Current Language: %s (%s)", qPrintable(QLocale::languageToString(locale.language())), qPrintable(QLocale::countryToString(locale.country())));
#endif


#if defined(AZ_PLATFORM_WINDOWS)
    // Format and send OS version line
    QString str = "Windows ";

#pragma warning( push )
#pragma warning(disable: 4996)
    GetVersionEx(&OSVerInfo);
#pragma warning( pop )

    if (OSVerInfo.dwMajorVersion == 4)
    {
        if (OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        {
            if (OSVerInfo.dwMinorVersion > 0)
            {
                // Windows 98
                str += "98";
            }
            else
            {
                // Windows 95
                str += "95";
            }
        }
        else
        {
            // Windows NT
            str += "NT";
        }
    }
    else if (OSVerInfo.dwMajorVersion == 5)
    {
        if (OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        {
            // Windows Millennium
            str += "ME";
        }
        else
        {
            if (OSVerInfo.dwMinorVersion > 0)
            {
                // Windows XP
                str += "XP";
            }
            else
            {
                // Windows 2000
                str += "2000";
            }
        }
    }
    else if (OSVerInfo.dwMajorVersion == 6)
    {
        if (OSVerInfo.dwMinorVersion == 0)
        {
            // Windows Vista
            str += "Vista";
        }
        else if (OSVerInfo.dwMinorVersion == 1)
        {
            // Windows 7
            str += "7";
        }
        else if (OSVerInfo.dwMinorVersion == 2)
        {
            // Windows 8
            str += "8";
        }
        else
        {
            // Windows unknown (newer than Win8)
            str += "Version Unknown";
        }
    }
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, " %d.%d", OSVerInfo.dwMajorVersion, OSVerInfo.dwMinorVersion);
    str += szBuffer;

    //////////////////////////////////////////////////////////////////////
    // Show Windows directory
    //////////////////////////////////////////////////////////////////////

    str += " (";
    GetWindowsDirectory(szBuffer, sizeof(szBuffer));
    str += szBuffer;
    str += ")";
    CryLog("%s", str.toUtf8().data());
#elif defined(AZ_PLATFORM_APPLE)
    QString operatingSystemName;
    if (QSysInfo::MacintoshVersion >= Q_MV_OSX(10, 12))
    {
        operatingSystemName = "macOS ";
    }
    else
    {
        operatingSystemName = "OS X ";
    }

    int majorVersion = 0;
    int minorVersion = 0;
    Gestalt(gestaltSystemVersionMajor, &majorVersion);
    Gestalt(gestaltSystemVersionMinor, &minorVersion);

    CryLog("%s - %d.%d", qPrintable(operatingSystemName), majorVersion, minorVersion);
#else
    CryLog("Unknown Operating System");
#endif

    //////////////////////////////////////////////////////////////////////
    // Send system time & date
    //////////////////////////////////////////////////////////////////////

#if defined(AZ_PLATFORM_WINDOWS)
    str = "Local time is ";
    _strtime(szBuffer);
    str += szBuffer;
    str += " ";
    _strdate(szBuffer);
    str += szBuffer;
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, ", system running for %d minutes", GetTickCount() / 60000);
    str += szBuffer;
    CryLog("%s", str.toUtf8().data());
#else
    struct timespec ts;
#if defined(AZ_PLATFORM_APPLE)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    CryLog("Local time is %s, system running for %ld minutes", qPrintable(QTime::currentTime().toString("hh:mm:ss")), ts.tv_sec / 60);
#endif

    //////////////////////////////////////////////////////////////////////
    // Send system memory status
    //////////////////////////////////////////////////////////////////////

#if defined(AZ_PLATFORM_WINDOWS)
    GlobalMemoryStatus(&MemoryStatus);
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, "%zdMB phys. memory installed, %zdMB paging available",
        MemoryStatus.dwTotalPhys / 1048576 + 1,
        MemoryStatus.dwAvailPageFile / 1048576);
    CryLog("%s", szBuffer);
#else
    SInt32 mb = 0, lmb = 0;
    Gestalt(gestaltPhysicalRAMSizeInMegabytes, &mb);
    Gestalt(gestaltLogicalRAMSize, &lmb);
    CryLog("%dMB phys. memory installed, %dMB paging available", mb, lmb);
#endif

    //////////////////////////////////////////////////////////////////////
    // Send display settings
    //////////////////////////////////////////////////////////////////////

#if defined(AZ_PLATFORM_WINDOWS)
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DisplayConfig);
    GetPrivateProfileString("boot.description", "display.drv",
        "(Unknown graphics card)", szProfileBuffer, sizeof(szProfileBuffer),
        "system.ini");
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, "Current display mode is %dx%dx%d, %s",
        DisplayConfig.dmPelsWidth, DisplayConfig.dmPelsHeight,
        DisplayConfig.dmBitsPerPel, szProfileBuffer);
    CryLog("%s", szBuffer);
#else
    auto screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        CryLog("Current display mode is %dx%dx%d, %s", screen->size().width(), screen->size().height(), screen->depth(), qPrintable(screen->name()));
    }
#endif

    //////////////////////////////////////////////////////////////////////
    // Send input device configuration
    //////////////////////////////////////////////////////////////////////

#if defined(AZ_PLATFORM_WINDOWS)
    str = "";
    // Detect the keyboard type
    switch (GetKeyboardType(0))
    {
    case 1:
        str = "IBM PC/XT (83-key)";
        break;
    case 2:
        str = "ICO (102-key)";
        break;
    case 3:
        str = "IBM PC/AT (84-key)";
        break;
    case 4:
        str = "IBM enhanced (101/102-key)";
        break;
    case 5:
        str = "Nokia 1050";
        break;
    case 6:
        str = "Nokia 9140";
        break;
    case 7:
        str = "Japanese";
        break;
    default:
        str = "Unknown";
        break;
    }

    // Any mouse attached ?
    if (!GetSystemMetrics(SM_MOUSEPRESENT))
    {
        CryLog("%s", str + " keyboard and no mouse installed");
    }
    else
    {
        azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, " keyboard and %i+ button mouse installed",
            GetSystemMetrics(SM_CMOUSEBUTTONS));
        str += szBuffer;
        CryLog("%s", str.toUtf8().data());
    }

    CryLog("--------------------------------------------------------------------------------");
#endif
}

//////////////////////////////////////////////////////////////////////////
QString CLogFile::GetMemUsage()
{
    ProcessMemInfo mi;
    CProcessInfo::QueryMemInfo(mi);
    int MB = 1024 * 1024;

    QString str;
    str = QStringLiteral("Memory=%1Mb, Pagefile=%2Mb").arg(mi.WorkingSet / MB).arg(mi.PagefileUsage / MB);
    //FormatLine( "PeakWorkingSet=%dMb, PeakPagefileUsage=%dMb",pc.PeakWorkingSetSize/MB,pc.PeakPagefileUsage/MB );
    //FormatLine( "PagedPoolUsage=%d",pc.QuotaPagedPoolUsage/MB );
    //FormatLine( "NonPagedPoolUsage=%d",pc.QuotaNonPagedPoolUsage/MB );

    return str;
}

//////////////////////////////////////////////////////////////////////////
void CLogFile::WriteLine(const char* pszString)
{
    CryLog("%s", pszString);
}

//////////////////////////////////////////////////////////////////////////
void CLogFile::WriteString(const char* pszString)
{
    if (gEnv && gEnv->pLog)
    {
        gEnv->pLog->LogPlus(pszString);
    }
}

static inline QString CopyAndRemoveColorCode(const char* sText)
{
    QByteArray ret = sText;      // alloc and copy

    char* s, * d;

    s = ret.data();
    d = s;

    // remove color code in place
    while (*s != 0)
    {
        if (*s == '$' && *(s + 1) >= '0' && *(s + 1) <= '9')
        {
            s += 2;
            continue;
        }

        *d++ = *s++;
    }

    ret.resize(d - ret.data());

    return QString::fromLatin1(ret);
}

//////////////////////////////////////////////////////////////////////////
void CLogFile::OnWriteToConsole(const char* sText, bool bNewLine)
{
    if (!gEnv)
    {
        return;
    }
    //  if (GetIEditor()->IsInGameMode())
    //  return;

    // Skip any non character.
    if (*sText != 0 && ((uint8) * sText) < 32)
    {
        sText++;
    }

    // If we have a listbox attached, also output the string to this listbox
    if (m_hWndListBox)
    {
        QString str = CopyAndRemoveColorCode(sText);        // editor prinout doesn't support color coded log messages

        if (m_bShowMemUsage)
        {
            str = QString("(") + GetMemUsage() + ")" + str;
        }

        // Add the string to the listbox
        m_hWndListBox->addItem(str);

        // Make sure the recently added string is visible
        m_hWndListBox->scrollToItem(m_hWndListBox->item(m_hWndListBox->count() - 1));

        if (m_hWndEditBox)
        {
            static int nCounter = 0;
            if (nCounter++ > 500)
            {
                // Clean Edit box every 500 lines.
                nCounter = 0;
                m_hWndEditBox->clear();
            }

            // remember selection and the top row
            int len = m_hWndEditBox->document()->toPlainText().length();
            int top;
            int from = m_hWndEditBox->textCursor().selectionStart();
            int to = from + m_hWndEditBox->textCursor().selectionEnd();
            bool keepPos = false;

            if (from != len || to != len)
            {
                keepPos = m_hWndEditBox->hasFocus();
                if (keepPos)
                {
                    top = m_hWndEditBox->verticalScrollBar()->value();

                }
                QTextCursor cur = m_hWndEditBox->textCursor();
                cur.setPosition(len);
                m_hWndEditBox->setTextCursor(cur);
            }
            if (bNewLine)
            {
                //str = CString("\r\n") + str.TrimLeft();
                //str = CString("\r\n") + str;
                //str = CString("\r") + str;
                str = QString("\r\n") + str;
                str = str.trimmed();
            }
            m_hWndEditBox->textCursor().insertText(str);

            // restore selection and the top line
            if (keepPos)
            {
                QTextCursor cur = m_hWndEditBox->textCursor();
                cur.setPosition(from);
                cur.setPosition(to, QTextCursor::KeepAnchor);
                m_hWndEditBox->setTextCursor(cur);
                m_hWndEditBox->verticalScrollBar()->setValue(top);
            }
        }
    }

    if (CConsoleSCB::GetCreatedInstance())
    {
        QString sOutLine(sText);

        if (gSettings.bShowTimeInConsole)
        {
            char sTime[128];
            time_t ltime;
            time(&ltime);
            struct tm today;
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
            localtime_s(&today, &ltime);
#else
            today = *localtime(&ltime);
#endif
            strftime(sTime, sizeof(sTime), "<%H:%M:%S> ", &today);
            sOutLine = sTime;
            sOutLine += sText;
        }

        CConsoleSCB::GetCreatedInstance()->AddToConsole(sOutLine, bNewLine);
    }
    else
    {
        // add to intermediate messages until an instance of CConsoleSCB exists
        CConsoleSCB::AddToPendingLines(sText, bNewLine);
    }

    //////////////////////////////////////////////////////////////////////////
    // Look for exit messages while writing to the console
    //////////////////////////////////////////////////////////////////////////
    if (gEnv && gEnv->pSystem && !gEnv->pSystem->IsQuitting() && !m_bIsQuitting)
    {
        if (QCoreApplication::closingDown())
        {
            m_bIsQuitting = true;
            CCryEditApp::instance()->ExitInstance();
            m_bIsQuitting = false;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLogFile::OnWriteToFile(const char* sText, bool bNewLine)
{
}
