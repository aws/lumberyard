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
#include "stdafx.h"

#include "ExampleWindow.h"
#include "exampleqtobject.h"
#include <platform_impl.h>
#include <QtQml/QQmlEngine.h>
#include <QtQml/qqml.h>

#include "../EditorCommon/QtViewPane.h"

class CExamplePlugin
    : public IPlugin
{
public:
    CExamplePlugin(IEditor* editor)
    {
        RegisterQMLTypes();
        // register your QML types here - or call things which will do it.
        RegisterQtViewPane<CExampleWindow>(editor, "_Example QML ViewPane", "Test");
    }

    void Release() override
    {
        // reverse order of init
        UnregisterQtViewPane<CExampleWindow>();
        delete this;
    }

    void ShowAbout() override {}
    const char* GetPluginGUID() override { return "{BC2E7E5A-7F86-46C4-A1EF-31E420E75A2B}"; }
    DWORD GetPluginVersion() override { return 1; }
    const char* GetPluginName() override { return "ExampleQMLViewPane"; }
    bool CanExitNow() override { return true; }

    void OnEditorNotify(EEditorNotifyEvent aEventId) override
    {
        if (aEventId == eNotify_QMLReady)
        {
            // this happens when the QML engine is rebooted for some reason, like some other plugin unloaded but we did not.
            RegisterQMLTypes();
        }

        if (aEventId == eNotify_BeforeQMLDestroyed)
        {
            GetIEditor()->CloseView("_Example QML ViewPane"); // may as well do this since QML is going to be destroyed.
        }
    }

    void RegisterQMLTypes()
    {
        qmlRegisterType<ExampleQtObject>("ExampleQMLViewPane", 1, 0, "ExampleQtObject");
    }
};


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    ISystem* pSystem = pInitParam->pIEditorInterface->GetSystem();
    ModuleInitISystem(pSystem, "ExampleQMLViewPane");
    return new CExamplePlugin(GetIEditor());
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
