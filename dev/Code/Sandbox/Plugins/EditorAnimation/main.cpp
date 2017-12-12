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

#include "pch.h"

#include "platform_impl.h"
#include "IEditorClassFactory.h"
#include "ICommandManager.h"
#include "IPlugin.h"
#include "IResourceSelectorHost.h"
#include "AnimationCompressionManager.h"
#include "CharacterTool/CharacterToolForm.h"
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include "../Editor/LyViewPaneNames.h"

// just for CGFContent:
#include <VertexFormats.h>
#include <Cry_Geo.h>
#include <TypeInfo_impl.h>
#include <Common_TypeInfo.h>
#include <IIndexedMesh_info.h>
#include <CGFContent_info.h>
// ^^^

// we need this...
class CXmlArchive;
struct SRayHitInfo;
#include <Cry_Geo.h>

#include "Serialization.h"
#include "Serialization/Pointers.h"
#include "Serialization/ITextInputArchive.h"
#include "Serialization/ITextOutputArchive.h"

#include "CharacterTool/CharacterToolForm.h"
#include "CharacterTool/CharacterToolSystem.h"

using Serialization::SharedPtr;

void Log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    GetIEditor()->GetSystem()->GetILog()->LogV(ILog::eAlways, format, args);
    va_end(args);
}

CharacterTool::System* g_pCharacterToolSystem;

// ---------------------------------------------------------------------------

class CEditorAnimationPlugin
    : public IPlugin
{
public:
    CEditorAnimationPlugin(IEditor* pEditor)
        : m_paneRegistered(false)
    {
    }

    void Init()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.canHaveMultipleInstances = true;
        options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
        AzToolsFramework::RegisterViewPane<CharacterTool::CharacterToolForm>(LyViewPane::LegacyGeppetto, LyViewPane::CategoryAnimation, options);
        m_paneRegistered = true;
        RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());
        g_pCharacterToolSystem = new CharacterTool::System();
        g_pCharacterToolSystem->Initialize();

        const ICVar* pUseImgCafCVar = gEnv->pConsole->GetCVar("ca_UseIMG_CAF");
        const bool useImgCafSet = (pUseImgCafCVar && pUseImgCafCVar->GetIVal());
        if (useImgCafSet)
        {
            CryWarning(VALIDATOR_MODULE_EDITOR,  VALIDATOR_WARNING, "[EditorAnimation] Animation compilation disabled: 'ca_UseIMG_CAF' should be set to zero at load time for compilation to work.");
        }
        else
        {
            g_pCharacterToolSystem->animationCompressionManager.reset(new CAnimationCompressionManager());
        }
    }


    void Release()
    {
        if (g_pCharacterToolSystem)
        {
            delete g_pCharacterToolSystem;
            g_pCharacterToolSystem = 0;
        }

        UnregisterCommands();
        if (m_paneRegistered)
        {
            AzToolsFramework::UnregisterViewPane(LyViewPane::LegacyGeppetto);
        }
        delete this;
    }

    void RegisterCommands()
    {
        ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
    }

    void UnregisterCommands()
    {
        ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
        pCommandManager->UnregisterCommand("character_tool", "open_editor");
    }

    // implements IEditorNotifyListener
    void OnEditorNotify(EEditorNotifyEvent event)
    {
        switch (event)
        {
        case eNotify_OnInit:
            break;
        case eNotify_OnSelectionChange:
        case eNotify_OnEndNewScene:
        case eNotify_OnEndSceneOpen:
            break;
        }
    }

    // implements IPlugin
    void ShowAbout() { }
    const char* GetPluginGUID() { return "{C3639520-A053-4C19-9C51-F7D94B2386A1}"; }
    DWORD GetPluginVersion() { return 0x01; }
    const char* GetPluginName() { return "EditorAnimation"; }
    bool CanExitNow() { return true; }
    void Serialize(FILE* hFile, bool bIsStoring) { }
    void ResetContent() { }
    bool CreateUIElements() { return false; }
    bool ExportDataToGame(const char* pszGamePath) { return false; }
private:

    bool m_paneRegistered;
};

//////////////////////////////////////////////////////////////////////////-

static HINSTANCE g_hInstance = 0;

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
    {
        pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
        return nullptr;
    }

    CEditorAnimationPlugin* pPlugin = nullptr;

#ifdef ENABLE_LEGACY_ANIMATION
    ModuleInitISystem(GetIEditor()->GetSystem(), "EditorAnimation");

    pPlugin = new CEditorAnimationPlugin(GetIEditor());

    pPlugin->Init();
#endif //ENABLE_LEGACY_ANIMATION
    return pPlugin;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
    }

    return(TRUE);
}
