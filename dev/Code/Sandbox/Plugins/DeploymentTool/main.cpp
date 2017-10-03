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

// you must include platform_impl in exactly one of your source files to provide implementations
// of platform functionality such as CryAssert.

#include <platform_impl.h>

#include "DeploymentToolWindow.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include "../Editor/LyViewPaneNames.h"

class MyPlugin
    : public IPlugin
{
public:
    MyPlugin(IEditor* editor)
    {
        AzToolsFramework::ViewPaneOptions options;
        options.showInMenu = false;
        AzToolsFramework::RegisterViewPane<DeploymentToolWindow>(LyViewPane::DeploymentTool, LyViewPane::CategoryOther, options);
    }

    void Release() override
    {
        // called directly from the editor to release your plugin.
        // after this returns, the plugin will be unloaded.
        AzToolsFramework::UnregisterViewPane(LyViewPane::DeploymentTool);
        delete this;
    }

    void ShowAbout() override {}

    // Make sure you alter this GUID for each plugin, and provide a different name in here.
    const char* GetPluginGUID() override { return "{A1E0D97B-FCED-430B-BB01-8B0E8CBFD498}"; }
    DWORD GetPluginVersion() override { return 1; }
    const char* GetPluginName() override { return "DeploymentTool"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify(EEditorNotifyEvent aEventId) override {}
};

// called from the editor to initialize your plugin:
PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    ISystem* pSystem = pInitParam->pIEditorInterface->GetSystem();
    ModuleInitISystem(pSystem, "DeploymentTool");
    // the above line initializes the gEnv global variable if necessary, and also makes GetIEditor() and other similar functions work correctly.

    return new MyPlugin(GetIEditor());
}

HINSTANCE g_hInstance = 0;
BOOL __stdcall DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
    }

    return TRUE;
}
