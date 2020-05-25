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
#include "EditorLoadDependencies.h"

namespace Sandbox
{
    void LoadPluginDependencies()
    {
        const char* engineRoot = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(engineRoot, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetEngineRootPath);
        AZ_Assert(engineRoot, "Engine root not available.");
        if (!engineRoot)
        {
            return;
        }
        // Use QDir and filePath to combine the absolute path to the engine and the relative path to the python directory.
        QString resolvedPythonPath(QDir(engineRoot).filePath(DEFAULT_LY_PYTHONHOME_RELATIVE));
        AZ_TracePrintf("LoadPluginDependencies", "Adding Python to DLL directories: %s\n", resolvedPythonPath.toUtf8().data());
        SetDllDirectoryA(resolvedPythonPath.toUtf8().data());
    }
}
