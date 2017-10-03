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

// Description : Assert dialog box for android


#ifndef CRYINCLUDE_CRYCOMMON_CRYASSERT_ANDROID_H
#define CRYINCLUDE_CRYCOMMON_CRYASSERT_ANDROID_H
#pragma once

#if defined(USE_CRY_ASSERT) && defined(ANDROID)

#include <NativeUIRequests.h>

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

bool CryAssert(const char* szCondition, const char* szFile, unsigned int line, bool* pIgnore)
{
    if (!gEnv)
    {
        return true;
    }

    gEnv->pSystem->OnAssert(szCondition, gs_szMessage, szFile, line);

    if (!gEnv->bNoAssertDialog && !gEnv->bIgnoreAllAsserts)
    {
        NativeUI::AssertAction result;
        EBUS_EVENT_RESULT(result, NativeUI::NativeUIRequestBus, DisplayAssertDialog, gs_szMessage);

        switch (result)
        {
        case NativeUI::AssertAction::IGNORE_ASSERT:
            return false;
        case NativeUI::AssertAction::IGNORE_ALL_ASSERTS:
            gEnv->bNoAssertDialog = true;
            gEnv->bIgnoreAllAsserts = true;
            return false;
        case NativeUI::AssertAction::BREAK:
            return true;
        default:
            break;
        }
        
        return true;
    }
    else
    {
        return false;
    }
}

#endif
#endif // CRYINCLUDE_CRYCOMMON_CRYASSERT_ANDROID_H
