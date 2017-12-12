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

// Description : Assert dialog box for Mac OS X


#ifndef CRYINCLUDE_CRYCOMMON_CRYASSERT_MAC_H
#define CRYINCLUDE_CRYCOMMON_CRYASSERT_MAC_H
#pragma once

#if defined(USE_CRY_ASSERT) && defined(MAC)

static char gs_szMessage[MAX_PATH];

void CryAssertTrace(const char* szFormat, ...)
{
    if (gEnv == 0)
    {
        return;
    }

    if (!gEnv->bIgnoreAllAsserts || gEnv->bTesting)
    {
        if (szFormat == NULL)
        {
            gs_szMessage[0] = '\0';
        }
        else
        {
            va_list args;
            va_start(args, szFormat);
            vsnprintf(gs_szMessage, sizeof(gs_szMessage), szFormat, args);
            va_end(args);
        }
    }
}
/*
bool CryAssert(const char* szCondition, const char* szFile,unsigned int line, bool *pIgnore)
{
    if (!gEnv) return false;

        gEnv->pSystem->OnAssert(szCondition, gs_szMessage, szFile, line);

        if (!gEnv->bNoAssertDialog && !gEnv->bIgnoreAllAsserts)
    {
        EDialogAction action = MacOSXHandleAssert(szCondition, szFile, line, gs_szMessage, gEnv->pRenderer != NULL);

        switch (action) {
            case eDAStop:
                raise(SIGABRT);
                exit(-1);
            case eDABreak:
                return true;
            case eDAIgnoreAll:
                gEnv->bIgnoreAllAsserts = true;
                break;
            case eDAIgnore:
                *pIgnore = true;
                break;
            case eDAReportAsBug:
                if ( gEnv && gEnv->pSystem)
                {
                    gEnv->pSystem->ReportBug("Assert: %s - %s", szCondition,gs_szMessage);
                }

            case eDAContinue:
            default:
                break;
        }
    }

    return false;
}*/

bool CryAssert(const char* szCondition, const char* szFile, unsigned int line, bool* pIgnore)
{
    if (!gEnv)
    {
        return false;
    }

    static const int max_len = 4096;
    static char gs_command_str[4096];

    static CryLockT<CRYLOCK_RECURSIVE> lock;

    gEnv->pSystem->OnAssert(szCondition, gs_szMessage, szFile, line);

    size_t file_len = strlen(szFile);

    if (!gEnv->bNoAssertDialog && !gEnv->bIgnoreAllAsserts)
    {
        // For asserts on the Mac always trigger a debug break. Annoying but at least it does not kill the thread like assert() does.
        __asm__("int $3");
#if 0 // Disable the assert terminal program for now as it does not build nor correctly install it self to be useful
        CryAutoLock< CryLockT<CRYLOCK_RECURSIVE> > lk (lock);
        snprintf(gs_command_str, max_len,
            "osascript -e 'tell application \"Terminal\"' "
            "-e 'activate' "
            "-e 'set w to do script \"%s/assert_term \\\"%s\\\" \\\"%s\\\" %d \\\"%s\\\"; echo $? > \\\"%s\\\"/.assert_return;\"' "
            "-e 'repeat' "
            "-e 'delay 0.1' "
            "-e 'if not busy of w then exit repeat' "
            "-e 'end repeat' "
            "-e 'do script \"exit 0\" in window 1' "
            "-e 'end tell' &>/dev/null",
            GetModulePath(), szCondition, (file_len > 60) ? szFile + (file_len - 61) : szFile, line, gs_szMessage, GetModulePath());
        int ret = system(gs_command_str);
        if (ret != 0)
        {
            CryLogAlways("<Assert> Terminal failed to execute");
            return false;
        }
        snprintf(gs_command_str, 4069, "%s/.assert_return", GetModulePath());
        FILE* assert_file = fopen(gs_command_str, "r");
        if (!assert_file)
        {
            CryLogAlways("<Assert> Couldn't open assert file");
            return false;
        }
        int result = -1;
        fscanf(assert_file, "%d", &result);
        fclose(assert_file);

        switch (result)
        {
        case 0:
            break;
        case 1:
            *pIgnore = true;
            break;
        case 2:
            gEnv->bIgnoreAllAsserts = true;
            break;
        case 3:
            return true;
            break;
        case 4:
            raise(SIGABRT);
            exit(-1);
            break;
        default:
            CryLogAlways("<Assert> Unknown result in assert file: %d", result);
            return false;
        }

#endif
    }


    return false;
}


#endif

#endif // CRYINCLUDE_CRYCOMMON_CRYASSERT_MAC_H
