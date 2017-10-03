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
#include "platform_impl.h"
#include "Include/IPlugin.h"
#include "Objects/DesignerObject.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/ClipVolumeObject.h"
#include "Tools/AreaSolidTool.h"
#include "Tools/DesignerTool.h"
#include "Tools/ClipVolumeTool.h"
#include "Util/Converter.h"
#include "Util/Exporter.h"
#include "GameExporter.h"
#include "Util/BoostPythonHelpers.h"
#include "Core/BrushHelper.h"
#include "DisplaySettings.h"
#include "UIs/UVMappingMainWnd.h"
#include "Serialization/Enum.h"
#include "QtViewPane.h"

#include <QMessageBox>

HINSTANCE g_hInst = NULL;

#ifndef AZ_TESTS_ENABLED
DECLARE_PYTHON_MODULE(designer);
#endif

class CEditorDesigner
    : public IPlugin
{
public:
    CEditorDesigner()
        : m_areaSolidClassDesc("AreaSolid", "Area", "", OBJTYPE_VOLUMESOLID, 51)
        , m_clipVolumeClassDesc("ClipVolume", "Area", "Editor/ObjectIcons/ClipVolume.bmp", OBJTYPE_VOLUMESOLID, 52)
        , m_DesignerObjectClassDesc("Designer", "Designer", "", OBJTYPE_SOLID, 150, "", "EditTool.DesignerTool")
    {
    }

    void Init()
    {
        IEditorClassFactory* cf = GetIEditor()->GetClassFactory();
        cf->RegisterClass(&m_areaSolidClassDesc);
        cf->RegisterClass(&m_clipVolumeClassDesc);
        cf->RegisterClass(&m_DesignerObjectClassDesc);
        cf->RegisterClass(&m_solidObjectClassDesc);
        CRegistrationContext rc;
        rc.pCommandManager = GetIEditor()->GetCommandManager();
        rc.pClassFactory = (CClassFactory*)GetIEditor()->GetClassFactory();
        DesignerTool::RegisterTool(rc);
        AreaSolidTool::RegisterTool(rc);
        ClipVolumeTool::RegisterTool(rc);

#ifndef DISABLE_UVMAPPING_WINDOW
        AzToolsFramework::ViewPaneOptions options;
        options.canHaveMultipleInstances = true;
        options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
        AzToolsFramework::RegisterViewPane<CD::UVMappingMainWnd>(
            Serialization::getEnumDescription<CD::EDesignerTool>().name(CD::eDesigner_UVMapping),
            "CryDesigner", options);
        m_bPanelRegistered = true;
#endif
    }
    void Release() override
    {
        CRegistrationContext rc;
        rc.pCommandManager = GetIEditor()->GetCommandManager();
        rc.pClassFactory = (CClassFactory*)GetIEditor()->GetClassFactory();
        DesignerTool::UnregisterTool(rc);
        AreaSolidTool::UnregisterTool(rc);
        ClipVolumeTool::UnregisterTool(rc);
        rc.pClassFactory->UnregisterClass(m_areaSolidClassDesc.ClassID());
        rc.pClassFactory->UnregisterClass(m_clipVolumeClassDesc.ClassID());
        rc.pClassFactory->UnregisterClass(m_DesignerObjectClassDesc.ClassID());
        rc.pClassFactory->UnregisterClass(m_solidObjectClassDesc.ClassID());
        delete this;
    }
    void ShowAbout() override {}
    const char* GetPluginGUID() override  { return "{1B77C1FA-2BE0-4DA3-8098-CBB2CEC0E61D}"; }
    DWORD GetPluginVersion() override { return 1; }
    const char* GetPluginName() override { return "CryDesigner"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify(EEditorNotifyEvent aEventId) override
    {
        switch (aEventId)
        {
        case eNotify_OnConvertToDesignerObjects:
        {
            CD::Converter converter;
            if (!converter.ConvertToDesignerObject())
            {
                CD::MessageBox("Warning", "The selected object(s) can't be converted.");
            }
        }
        break;

        case eNotify_OnBeginExportToGame:
        case eNotify_OnBeginSceneSave:
        {
            CD::RemoveAllEmptyDesignerObjects();
        }
        break;

        case eNotify_OnExportBrushes:
        {
            CGameExporter* pGameExporter = CGameExporter::GetCurrentExporter();
            if (pGameExporter)
            {
                CD::Exporter designerExporter;
                QString path = Path::RemoveBackslash(Path::GetPath(pGameExporter->GetLevelPack().m_sPath));
                designerExporter.ExportBrushes(path, pGameExporter->GetLevelPack().m_pakFile);
            }
        }
        break;
        }
        if (CD::GetDesigner())
        {
            CD::GetDesigner()->OnEditorNotifyEvent(aEventId);
        }
    }

private:
    bool m_bPanelRegistered;

    CTemplateObjectClassDesc<AreaSolidObject> m_areaSolidClassDesc;
    CTemplateObjectClassDesc<ClipVolumeObject> m_clipVolumeClassDesc;
    CTemplateObjectClassDesc<DesignerObject> m_DesignerObjectClassDesc;
    SolidObjectClassDesc m_solidObjectClassDesc;
};


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
    {
        pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
        return 0;
    }
    ModuleInitISystem(GetIEditor()->GetSystem(), "CryDesigner");
    CEditorDesigner* pDesignerPlugin = new CEditorDesigner;
    pDesignerPlugin->Init();
    return pDesignerPlugin;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hinstDLL;
    }

    return TRUE;
}