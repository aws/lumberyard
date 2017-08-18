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

#include "stdafx.h"
#include "FFMPEGPlugin.h"
#include "Include/IEditorClassFactory.h"

HINSTANCE g_hInstance = 0;

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
    {
        pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
        return 0;
    }

    ModuleInitISystem(GetIEditor()->GetSystem(), "FFMPEGPlugin");
    GetIEditor()->GetSystem()->GetILog()->Log("FFMPEG plugin: CreatePluginInstance");
    CFFMPEGPlugin::RegisterTheCommand();
    return new CFFMPEGPlugin;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
        //DisableThreadLibraryCalls(hInstance);
    }

    return(TRUE);
}