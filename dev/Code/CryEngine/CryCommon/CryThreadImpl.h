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

#ifndef CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_H
#define CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_H
#pragma once


#include <CryThread.h>

// Include architecture specific code.
#if defined(LINUX) || defined(APPLE)
#include <CryThreadImpl_pthreads.h>
#elif defined(WIN32) || defined(WIN64)
#include <CryThreadImpl_windows.h>
#else
// Put other platform specific includes here!
#endif

#include <IThreadTask.h>

void CryThreadSetName(threadID dwThreadId, const char* sThreadName)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIThreadTaskManager())
    {
        gEnv->pSystem->GetIThreadTaskManager()->SetThreadName(dwThreadId, sThreadName);
    }
}

const char* CryThreadGetName(threadID dwThreadId)
{
    if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIThreadTaskManager())
    {
        return gEnv->pSystem->GetIThreadTaskManager()->GetThreadName(dwThreadId);
    }
    return "";
}

#endif // CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_H

// vim:ts=2

