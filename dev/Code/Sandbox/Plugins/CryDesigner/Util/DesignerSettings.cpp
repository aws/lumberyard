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
#include "Settings.h"
#include "RenderViewport.h"
#include "ITimeOfDay.h"
#include "TimeOfDayDialog.h"
#include "Tools/DesignerTool.h"
#include "Objects/DesignerObject.h"
#include "Objects/AreaSolidObject.h"
#include "Mission.h"
#include "CryEditDoc.h"
#include "Objects/EnvironmentProbeObject.h"
#include "Serialization/Decorators/Range.h"
#include "QtViewPaneManager.h"

#include <QDir>

using namespace CD;

class CUndoExclusiveMode
    : public IUndoObject
{
public:
    CUndoExclusiveMode(const char* undoDescription = NULL)
    {
    }

    int GetSize() { return sizeof(*this); }
    QString GetDescription() { return "Undo for Designer Exclusive Mode"; }

    void Undo(bool bUndo = true) override
    {
    }

    void Redo() override
    {
    }
};

namespace CD
{
    SDesignerExclusiveMode gExclusiveModeSettings;
    SDesignerSettings gDesignerSettings;
}

DesignerTool* GetDesignerEditTool()
{
    return qobject_cast<DesignerTool*>(GetIEditor()->GetEditTool());
}

CRenderViewport* GetRenderViewport()
{
    CViewport* pViewport = GetIEditor()->GetActiveView();
    if (CRenderViewport* rvp = viewport_cast<CRenderViewport*>(pViewport))
    {
        return rvp;
    }
    return NULL;
}

void SDesignerExclusiveMode::CenterCameraForExclusiveMode()
{
    CSelectionGroup* pSelection = GetIEditor()->GetSelection();
    CRenderViewport* pView = GetRenderViewport();
    if (pSelection->GetCount() > 0 && pView)
    {
        AABB selBoundBox = pSelection->GetBounds();
        Vec3 vTopCenterPos = pSelection->GetCenter();
        vTopCenterPos.z = selBoundBox.max.z;
        vTopCenterPos -= pView->GetCamera().GetViewdir() * (selBoundBox.GetRadius() + 10.0f);
        Matrix34 cameraTM = pView->GetViewTM();
        cameraTM.SetTranslation(vTopCenterPos);
        pView->SetViewTM(cameraTM);
    }
}

void SDesignerExclusiveMode::EnableExclusiveMode(bool bEnable)
{
    if (m_bExclusiveMode == bEnable)
    {
        return;
    }

    if (bEnable)
    {
        CSelectionGroup* pGroup = GetIEditor()->GetSelection();
        if (pGroup->GetCount() == 0)
        {
            return;
        }
        for (int i = 0, iObjectCount(pGroup->GetCount()); i < iObjectCount; ++i)
        {
            CBaseObject* pObject = pGroup->GetObject(i);
            if (!qobject_cast<DesignerObject*>(pObject))
            {
                return;
            }
        }
    }

    GetIEditor()->SuspendUndo();

    m_bExclusiveMode = bEnable;

    CRenderViewport* pView = GetRenderViewport();
    if (pView == NULL)
    {
        GetIEditor()->ResumeUndo();
        return;
    }

    if (m_bExclusiveMode)
    {
        CBaseObject* pCameraObj = GetIEditor()->GetObjectManager()->FindObject(CD::BrushDesignerCameraName);
        if (pCameraObj)
        {
            GetIEditor()->GetObjectManager()->DeleteObject(pCameraObj);
        }

        QString designerCameraPath = QString("%1\\Editor\\Objects\\CryDesigner_Camera.grp").arg(QDir::currentPath());

        XmlNodeRef cameraNode = XmlHelpers::LoadXmlFromFile(designerCameraPath.toUtf8().constData());
        if (cameraNode)
        {
            CObjectArchive cameraArchive(GetIEditor()->GetObjectManager(), cameraNode, true);
            GetIEditor()->GetObjectManager()->LoadObjects(cameraArchive, false);
            pCameraObj = GetIEditor()->GetObjectManager()->FindObject(CD::BrushDesignerCameraName);
        }

        if (!pCameraObj)
        {
            GetIEditor()->ResumeUndo();
            return;
        }

        pCameraObj->SetFlags(OBJFLAG_HIDE_HELPERS);
        DynArray< _smart_ptr<CBaseObject> > allLightsUnderCamera;
        pCameraObj->GetAllChildren(allLightsUnderCamera);
        for (int i = 0, iCount(allLightsUnderCamera.size()); i < iCount; ++i)
        {
            allLightsUnderCamera[i]->SetFlags(OBJFLAG_HIDE_HELPERS);
        }


        m_OldCameraTM = pView->GetViewTM();
        pView->SetCameraObject(pCameraObj);
        pView->EnableCameraObjectMove(true);
        pView->LockCameraMovement(false);
        pView->SetViewTM(m_OldCameraTM);

        m_OldObjectHideMask = gSettings.objectHideMask;
        gSettings.objectHideMask = 0;

        SetCVForExclusiveMode();
        SetTimeOfDayForExclusiveMode();
        SetObjectsFlagForExclusiveMode();
        CenterCameraForExclusiveMode();
    }
    else if (pView->GetCameraObject() && pView->GetCameraObject()->GetName() == CD::BrushDesignerCameraName)
    {
        Matrix34 cameraTM = pView->GetCamera().GetMatrix();

        pView->SetCameraObject(NULL);
        pView->EnableCameraObjectMove(false);
        pView->LockCameraMovement(true);

        _smart_ptr<CBaseObject> pCameraObj = GetIEditor()->GetObjectManager()->FindObject(CD::BrushDesignerCameraName);
        if (pCameraObj)
        {
            DynArray< _smart_ptr<CBaseObject> > allLightsUnderCamera;
            pCameraObj->GetAllChildren(allLightsUnderCamera);
            for (int i = 0, iCount(allLightsUnderCamera.size()); i < iCount; ++i)
            {
                GetIEditor()->GetObjectManager()->DeleteObject(allLightsUnderCamera[i]);
            }
            GetIEditor()->GetObjectManager()->DeleteObject(pCameraObj);
            pView->SetViewTM(m_OldCameraTM);
        }

        gSettings.objectHideMask = m_OldObjectHideMask;
        m_OldObjectHideMask = 0;

        RestoreCV();
        RestoreTimeOfDay();
        RestoreObjectsFlag();
    }

    GetIEditor()->ResumeUndo();
}

void SDesignerExclusiveMode::SetCVForExclusiveMode()
{
    IConsole* pConsole = GetIEditor()->GetSystem()->GetIConsole();
    if (!pConsole)
    {
        return;
    }

    ICVar* pDisplayInfo = pConsole->GetCVar("r_DisplayInfo");
    if (pDisplayInfo)
    {
        m_OldConsoleVars.r_DisplayInfo = pDisplayInfo->GetIVal();
        pDisplayInfo->Set(0);
    }
    
    ICVar* pSSDO = pConsole->GetCVar("r_ssdo");
    if (pSSDO)
    {
        m_OldConsoleVars.r_ssdo = pSSDO->GetIVal();
        pSSDO->Set(0);
    }

    ICVar* pPostProcessEffect = pConsole->GetCVar("r_PostProcessEffects");
    if (pPostProcessEffect)
    {
        m_OldConsoleVars.r_PostProcessEffects = pPostProcessEffect->GetIVal();
        pPostProcessEffect->Set(0);
    }

    ICVar* pRenderVegetation = pConsole->GetCVar("e_Vegetation");
    if (pRenderVegetation)
    {
        m_OldConsoleVars.e_Vegetation = pRenderVegetation->GetIVal();
        pRenderVegetation->Set(0);
    }

    ICVar* pWaterOcean = pConsole->GetCVar("e_WaterOcean");
    if (pWaterOcean)
    {
        m_OldConsoleVars.e_WaterOcean = pWaterOcean->GetIVal();
        pWaterOcean->Set(0);
    }

    ICVar* pWaterVolume = pConsole->GetCVar("e_WaterVolumes");
    if (pWaterVolume)
    {
        m_OldConsoleVars.e_WaterVolumes = pWaterVolume->GetIVal();
        pWaterVolume->Set(0);
    }

    ICVar* pTerrain = pConsole->GetCVar("e_Terrain");
    if (pTerrain)
    {
        m_OldConsoleVars.e_Terrain = pTerrain->GetIVal();
        pTerrain->Set(0);
    }

    ICVar* pShadow = pConsole->GetCVar("e_Shadows");
    if (pShadow)
    {
        m_OldConsoleVars.e_Shadows = pShadow->GetIVal();
        pShadow->Set(0);
    }

    ICVar* pParticle = pConsole->GetCVar("e_Particles");
    if (pParticle)
    {
        m_OldConsoleVars.e_Particles = pParticle->GetIVal();
        pParticle->Set(0);
    }

    ICVar* pClouds = pConsole->GetCVar("e_Clouds");
    if (pClouds)
    {
        m_OldConsoleVars.e_Clouds = pClouds->GetIVal();
        pClouds->Set(0);
    }

    ICVar* pBeams = pConsole->GetCVar("r_Beams");
    if (pBeams)
    {
        m_OldConsoleVars.r_Beams = pBeams->GetIVal();
        pBeams->Set(0);
    }

    ICVar* pSkybox = pConsole->GetCVar("e_SkyBox");
    if (pSkybox)
    {
        m_OldConsoleVars.e_SkyBox = pSkybox->GetIVal();
        pSkybox->Set(1);
    }
}

void SDesignerExclusiveMode::SetObjectsFlagForExclusiveMode()
{
    DynArray<CBaseObject*> objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    m_ObjectHiddenFlagMap.clear();

    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();

    for (int i = 0, iObjectCount(objects.size()); i < iObjectCount; ++i)
    {
        m_ObjectHiddenFlagMap[objects[i]] = objects[i]->IsHidden();
        bool bSelected = false;
        for (int k = 0, iSelectionCount(pSelection->GetCount()); k < iSelectionCount; ++k)
        {
            if (pSelection->GetObject(k) == objects[i])
            {
                bSelected = true;
                break;
            }
        }
        if (bSelected)
        {
            continue;
        }
        if (objects[i]->GetName() != CD::BrushDesignerCameraName && (!objects[i]->GetParent() || objects[i]->GetParent()->GetName() != CD::BrushDesignerCameraName))
        {
            objects[i]->SetHidden(true);
        }
    }
}

void SDesignerExclusiveMode::SetTime(ITimeOfDay* pTOD, float fTime)
{
    if (pTOD == NULL)
    {
        return;
    }
    pTOD->SetTime(fTime, true);
    GetIEditor()->SetConsoleVar("e_TimeOfDay", fTime);

    CTimeOfDayDialog* timeOfDayDialog = FindViewPane<CTimeOfDayDialog>(QLatin1String("Time Of Day"));
    if (timeOfDayDialog)
    {
        timeOfDayDialog->UpdateValues();
    }
    else
    {
        pTOD->Update(false, true);
        GetIEditor()->GetDocument()->GetCurrentMission()->SetTime(fTime);
        GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
    }
}

void SDesignerExclusiveMode::SetTimeOfDayForExclusiveMode()
{
    QString designerTimeOfDay = QString("%1\\Editor\\CryDesigner_TimeOfDay.xml").arg(QDir::currentPath());

    XmlNodeRef root = GetIEditor()->GetSystem()->LoadXmlFromFile(designerTimeOfDay.toUtf8().constData());
    if (root)
    {
        ITimeOfDay* pTimeOfDay = GetIEditor()->Get3DEngine()->GetTimeOfDay();

        m_OldTimeOfDay = GetIEditor()->GetSystem()->CreateXmlNode();
        m_OldTimeOfTOD = GetIEditor()->GetConsoleVar("e_TimeOfDay");
        pTimeOfDay->Serialize(m_OldTimeOfDay, false);

        pTimeOfDay->Serialize(root, true);
        SetTime(pTimeOfDay, 0);
    }
}

void SDesignerExclusiveMode::RestoreTimeOfDay()
{
    if (m_OldTimeOfDay)
    {
        ITimeOfDay* pTimeOfDay = GetIEditor()->Get3DEngine()->GetTimeOfDay();
        pTimeOfDay->Serialize(m_OldTimeOfDay, true);
        SetTime(pTimeOfDay, m_OldTimeOfTOD);
        m_OldTimeOfDay = NULL;
    }
}

void SDesignerExclusiveMode::RestoreCV()
{
    IConsole* pConsole = GetIEditor()->GetSystem()->GetIConsole();
    if (!pConsole)
    {
        return;
    }

    ICVar* pDisplayInfo = pConsole->GetCVar("r_DisplayInfo");
    if (pDisplayInfo)
    {
        pDisplayInfo->Set(m_OldConsoleVars.r_DisplayInfo);
    }

    ICVar* pPostProcessEffect = pConsole->GetCVar("r_PostProcessEffects");
    if (pPostProcessEffect)
    {
        pPostProcessEffect->Set(m_OldConsoleVars.r_PostProcessEffects);
    }

    ICVar* pSSDO = pConsole->GetCVar("r_ssdo");
    if (pSSDO)
    {
        pSSDO->Set(m_OldConsoleVars.r_ssdo);
    }

    ICVar* pRenderVegetation = pConsole->GetCVar("e_Vegetation");
    if (pRenderVegetation)
    {
        pRenderVegetation->Set(m_OldConsoleVars.e_Vegetation);
    }

    ICVar* pWaterOcean = pConsole->GetCVar("e_WaterOcean");
    if (pWaterOcean)
    {
        pWaterOcean->Set(m_OldConsoleVars.e_WaterOcean);
    }

    ICVar* pWaterVolume = pConsole->GetCVar("e_WaterVolumes");
    if (pWaterVolume)
    {
        pWaterVolume->Set(m_OldConsoleVars.e_WaterVolumes);
    }

    ICVar* pTerrain = pConsole->GetCVar("e_Terrain");
    if (pTerrain)
    {
        pTerrain->Set(m_OldConsoleVars.e_Terrain);
    }

    ICVar* pShadow = pConsole->GetCVar("e_Shadows");
    if (pShadow)
    {
        pShadow->Set(m_OldConsoleVars.e_Shadows);
    }

    ICVar* pParticle = pConsole->GetCVar("e_Particles");
    if (pParticle)
    {
        pParticle->Set(m_OldConsoleVars.e_Particles);
    }

    ICVar* pClouds = pConsole->GetCVar("e_Clouds");
    if (pClouds)
    {
        pClouds->Set(m_OldConsoleVars.e_Clouds);
    }

    ICVar* pBeams = pConsole->GetCVar("r_Beams");
    if (pBeams)
    {
        pBeams->Set(m_OldConsoleVars.r_Beams);
    }

    ICVar* pSkybox = pConsole->GetCVar("e_SkyBox");
    if (pSkybox)
    {
        pSkybox->Set(m_OldConsoleVars.e_SkyBox);
    }
}

void SDesignerExclusiveMode::RestoreObjectsFlag()
{
    DynArray<CBaseObject*> objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    for (int i = 0, iObjectCount(objects.size()); i < iObjectCount; ++i)
    {
        if (m_ObjectHiddenFlagMap.find(objects[i]) != m_ObjectHiddenFlagMap.end())
        {
            objects[i]->SetHidden(m_ObjectHiddenFlagMap[objects[i]]);
        }
    }

    GetIEditor()->SetConsoleVar("e_Vegetation", 1.0f);
}

SDesignerSettings::SDesignerSettings()
{
    bExclusiveMode = false;
    bDisplayBackFaces = false;
    bKeepPivotCenter = false;
    bHighlightElements = true;
    fHighlightBoxSize = 24 * 0.001f;
    bSeamlessEdit = false;
    bDisplayTriangulation = false;
    bDisplayDimensionHelper = true;
    bDisplaySubdividedResult = true;
}

void SDesignerSettings::Load()
{
    CD::LoadSettings(Serialization::SStruct(*this), "DesignerSetting");
}

void SDesignerSettings::Save()
{
    CD::SaveSettings(Serialization::SStruct(*this), "DesignerSetting");
}

void SDesignerSettings::Serialize(Serialization::IArchive& ar)
{
    if (ar.IsEdit())
    {
        ar(bExclusiveMode, "ExclusiveMode", "Exclusive Mode");
        ar(bDisplayBackFaces, "DisplayBackFaces", "Display Back Faces(EditorOnly)");
    }
    ar(bSeamlessEdit, "SeamlessEdit", "Seamless Edit");
    ar(bKeepPivotCenter, "KeepPivotCenter", "Keep Pivot Center");
    ar(bHighlightElements, "HighlightElements", "Highlight Elements");
    ar(Serialization::Range(fHighlightBoxSize, 0.005f, 2.0f), "HighlightBoxSize", "Highlight Box Size");
    ar(bDisplayDimensionHelper, "DisplayDimensionHelper", "Display Dimension Helper");
    ar(bDisplayTriangulation, "DisplayTriangulation", "Display Triangulation");
    ar(bDisplaySubdividedResult, "DisplaySubdividedResult", "Display Subdivided Result");
}

void SDesignerSettings::Update(bool continuous)
{
    gExclusiveModeSettings.EnableExclusiveMode(bExclusiveMode);
    Save();
}