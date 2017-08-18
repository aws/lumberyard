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

#include "StdAfx.h"

#include <platform.h>
#include <platform_impl.h>
#include <IEditor.h>
#include <Include/IPlugin.h>
#include <Include/IEditorClassFactory.h>
#include "AudioControlsEditorPlugin.h"

//-----------------------------------------------------------------------------------------------//
PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    ISystem* pSystem = pInitParam->pIEditorInterface->GetSystem();
    ModuleInitISystem(pSystem, "AudioControlsEditor");
    return new CAudioControlsEditorPlugin(GetIEditor());
}

//-----------------------------------------------------------------------------------------------//
HINSTANCE g_hInstance = 0;
BOOL __stdcall DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
    }
    return TRUE;
}
