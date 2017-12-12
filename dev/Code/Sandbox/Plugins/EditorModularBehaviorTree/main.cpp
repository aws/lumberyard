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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include "../Editor/LyViewPaneNames.h"

#include "MainWindow.h"

static const char* ViewPaneName = "Modular Behavior Tree Editor";

class Plugin
    : public IPlugin
{
    enum
    {
        Version = 1,
    };

public:
    bool Init(IEditor* pEditor)
    {
        AzToolsFramework::ViewPaneOptions options;
        options.canHaveMultipleInstances = true;
        options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
        options.isLegacy = true;
        AzToolsFramework::RegisterViewPane<MainWindow>(ViewPaneName, LyViewPane::CategoryOther, options);
        return true;
    }

    void Release() override
    {
        AzToolsFramework::UnregisterViewPane(ViewPaneName);
        delete this;
    }

    void ShowAbout() override {}
    const char* GetPluginGUID() override { return "{0AE29C33-36BB-4DB0-8596-DE662AFE0E98}"; }
    DWORD GetPluginVersion() override { return DWORD(Version); }
    const char* GetPluginName() override { return "Modular Behavior Tree Editor"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify(EEditorNotifyEvent aEventId) override {}
};

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
    {
        pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
        return nullptr;
    }

    ModuleInitISystem(GetIEditor()->GetSystem(), "Modular Behavior Tree Editor");

    Plugin* pPlugin = new Plugin();
    if (pPlugin->Init(GetIEditor()))
    {
        return pPlugin;
    }
    else
    {
        pPlugin->Release();
        return NULL;
    }
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
