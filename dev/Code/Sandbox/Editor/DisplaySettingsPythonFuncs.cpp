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

#include "StdAfx.h"
#include "DisplaySettings.h"
#include "Util/BoostPythonHelpers.h"

namespace
{
    int PyGetMiscDisplaySettings()
    {
        return GetIEditor()->GetDisplaySettings()->GetSettings();
    }

    void PySetMiscDisplaySettings(int flags)
    {
        GetIEditor()->GetDisplaySettings()->SetSettings(flags);
    }
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetMiscDisplaySettings, settings, get_misc_editor_settings,
    "Get miscellaneous Editor settings (camera collides with terrain, AI/Physics enabled, etc).",
    "settings.get_misc_editor_settings()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetMiscDisplaySettings, settings, set_misc_editor_settings,
    "Set miscellaneous Editor settings (camera collides with terrain, AI/Physics enabled, etc).",
    "settings.set_misc_editor_settings()");
