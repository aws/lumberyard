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

#ifndef CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINDEBUG_H
#define CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINDEBUG_H
#pragma once

#ifndef _RELEASE
#define MANNEQUIN_DEBUGGING_ENABLED (1)
#else
#define MANNEQUIN_DEBUGGING_ENABLED (0)
#endif

namespace mannequin
{
    namespace debug
    {
        void RegisterCommands();

#if MANNEQUIN_DEBUGGING_ENABLED
        void Log(const IActionController& actionControllerI, const char* format, ...);
        void DrawDebug();
        inline void WebDebugState(const CActionController& actionController)                                    {}
#else
        inline void Log(const IActionController& actionControllerI, const char* format, ...)    {}
        inline void DrawDebug()                                                                                                                             {}
        inline void WebDebugState(const CActionController& actionController)                                    {}
#endif
    }
}

//--- Macro ensures that complex expressions within a log are still removed from the build completely
#if MANNEQUIN_DEBUGGING_ENABLED
#define MannLog(ac, format, ...) mannequin::debug::Log(ac, format, __VA_ARGS__)
#else //MANNEQUIN_DEBUGGING_ENABLED
#define MannLog(...)
#endif //MANNEQUIN_DEBUGGING_ENABLED

#endif // CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUINDEBUG_H