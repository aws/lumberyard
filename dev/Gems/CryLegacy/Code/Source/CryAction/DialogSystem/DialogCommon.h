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

#ifndef CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGCOMMON_H
#define CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGCOMMON_H
#pragma once

#include <ISystem.h>
#include "DialogSystem.h"

namespace DiaLOG
{
    enum ELevel
    {
        eAlways = 1,
        eDebugA = 2,
        eDebugB = 3,
        eDebugC = 4
    };

    inline void Log(DiaLOG::ELevel level, const char*, ...) PRINTF_PARAMS(2, 3);

    //////////////////////////////////////////////////////////////////////////
    //! Reports a Game Warning to validator with WARNING severity.
    inline void Log(DiaLOG::ELevel level, const char* format, ...)
    {
        if (gEnv->pSystem && (int)level <= CDialogSystem::sDiaLOGLevel)
        {
            va_list args;
            va_start(args, format);
            gEnv->pLog->LogV(ILog::eMessage, format, args);
            va_end(args);
        }
    }
};


#endif // CRYINCLUDE_CRYACTION_DIALOGSYSTEM_DIALOGCOMMON_H
