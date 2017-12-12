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
#include "platform_impl.h"
#include "TelemetryDialog.h"

/*
///////////////////////////////////////////////////////////////////////////////
// Missing externals.
///////////////////////////////////////////////////////////////////////////////
void* CryModuleMalloc(size_t size) {return malloc(size);}
size_t CryModuleFree(void *ptr) { size_t ret=_msize(ptr); free(ptr); return ret; }
void* CryModuleRealloc(void *memblock,size_t size) {return realloc(memblock, size);}
///////////////////////////////////////////////////////////////////////////////
*/

class CTelemetryPlugin
    : public IPlugin
{
public:

    void Release()
    {
        delete this;
    }

    void ShowAbout()
    {
        MessageBox(NULL, "Telemetry Plug-in\n by Crytek GmBh\nAll rights reserved.", "Telemetry Plug-in", MB_OK);
    }

    const char* GetPluginGUID()
    {
        return "{22E45992-72FD-4f3a-B937-6EB418AD52C2}";
    }

    DWORD GetPluginVersion()
    {
        return 1;
    }

    const char* GetPluginName()
    {
        return "Telemetry Plug-in";
    }

    bool CanExitNow()
    {
        return true;
    }
};


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    ISystem* pSystem = pInitParam->pIEditorInterface->GetSystem();
    ModuleInitISystem(pSystem, "TelemetryPlugin");

    // Register class for the View
    CTelemetryDialog::RegisterViewClass();

    return new CTelemetryPlugin;
}
