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
#include "EnvironmentProbeObject.h"
#include "../EnvironmentProbePanel.h"
#include "Util/CubemapUtils.h"
#include "Util/Variable.h"
#include "GameEngine.h"
#include "../Cry3DEngine/StatObj.h"
#include "../Cry3DEngine/Material.h"
#include "../Cry3DEngine/MatMan.h"
#include <IShader.h> // <> required for Interfuscator
#include <ITimeOfDay.h>

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// statics
//////////////////////////////////////////////////////////////////////////
int CEnvironementProbeObject::s_panelProbeID = 0;
CEnvironmentProbePanel* CEnvironementProbeObject::s_pProbePanel = NULL;

//////////////////////////////////////////////////////////////////////////
CEnvironementProbeObject::CEnvironementProbeObject()
{
    m_entityClass = "EnvironmentLight";
}

//////////////////////////////////////////////////////////////////////////
CEnvironementProbeTODObject::CEnvironementProbeTODObject()
{
    m_entityClass = "EnvironmentLightTOD";
    m_timeSlots = 4;
}

//////////////////////////////////////////////////////////////////////////
bool CEnvironementProbeObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = CEntityObject::Init(ie, prev, file);
    return res;
}

//////////////////////////////////////////////////////////////////////////
void CEnvironementProbeObject::InitVariables()
{
    CVarEnumList<int>* enumList = new CVarEnumList<int>();
    enumList->AddItem("skip update", 0);
    //enumList->AddItem( "32",32 );
    //enumList->AddItem( "64",64 );
    //enumList->AddItem( "128",128 );
    enumList->AddItem("256", 256);
    enumList->AddItem("512", 512);
    enumList->AddItem("1024", 1024);
    m_cubemap_resolution->SetEnumList(enumList);
    m_cubemap_resolution->SetDisplayValue("256");
    m_cubemap_resolution->SetFlags(m_cubemap_resolution->GetFlags() | IVariable::UI_UNSORTED);

    AddVariable(m_cubemap_resolution, QString("cubemap_resolution"));
    AddVariable(m_preview_cubemap, QString("preview_cubemap"), functor(*this, &CEnvironementProbeObject::OnPreviewCubemap), IVariable::DT_SIMPLE);
    m_preview_cubemap->Set(false);

    CEntityObject::InitVariables();
}

//////////////////////////////////////////////////////////////////////////
void CEnvironementProbeObject::Serialize(CObjectArchive& ar)
{
    CEntityObject::Serialize(ar);
}

//////////////////////////////////////////////////////////////////////////
void CEnvironementProbeObject::BeginEditParams(IEditor* ie, int flags)
{
    CEntityObject::BeginEditParams(ie, flags);
    if (!s_panelProbeID)
    {
        s_pProbePanel = new CEnvironmentProbePanel();
        s_panelProbeID = ie->AddRollUpPage(ROLLUP_OBJECTS, "Probe Functions", s_pProbePanel, 3);
    }
    if (s_panelProbeID)
    {
        s_pProbePanel->SetEntity(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEnvironementProbeObject::EndEditParams(IEditor* ie)
{
    if (s_pProbePanel)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, s_panelProbeID);
        s_panelProbeID = 0;
        s_pProbePanel = 0;
    }
    CEntityObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
void CEnvironementProbeObject::Display(DisplayContext& dc)
{
    // Check to see if m_preview_cubemap is enabled but the m_visualObject does not exist.
    // This scenario is hit when serializing the level because m_visualObject will be destroyed 
    // in the entity base class during serialization, so we have to recreate it here.
    bool doPreview = false;
    m_preview_cubemap->Get(doPreview);
    if ( doPreview && !m_visualObject )
    {
        ToggleCubemapPreview( true );
    }

    Matrix34 wtm = GetWorldTM();
    Vec3 wp = wtm.GetTranslation();
    float fScale = GetHelperScale();

    if (IsHighlighted() && !IsFrozen())
    {
        dc.SetLineWidth(3);
    }

    if (IsSelected())
    {
        dc.SetSelectedColor();
    }
    else if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(GetColor());
    }

    if (!m_visualObject)
    {
        dc.PushMatrix(wtm);
        Vec3 sz(fScale * 0.5f, fScale * 0.5f, fScale * 0.5f);
        dc.DrawWireBox(-sz, sz);

        // Draw radiuses if present and object selected.
        if (gSettings.viewports.bAlwaysShowRadiuses || IsSelected())
        {
            if (m_bBoxProjectedCM)
            {
                // Draw helper for box projection.
                float fBoxWidth = m_fBoxWidth * 0.5f;
                float fBoxHeight = m_fBoxHeight * 0.5f;
                float fBoxLength = m_fBoxLength * 0.5f;

                dc.SetColor(0, 1, 0, 0.8f);
                dc.DrawWireBox(Vec3(-fBoxLength, -fBoxWidth, -fBoxHeight), Vec3(fBoxLength, fBoxWidth, fBoxHeight));
            }

            const Vec3& scale = GetScale();
            if (scale.x != 0.0f && scale.y != 0.0f && scale.z != 0.0f)
            {
                Vec3 size(m_boxSizeX * 0.5f / scale.x, m_boxSizeY * 0.5f / scale.y, m_boxSizeZ * 0.5f / scale.z);

                dc.SetColor(1, 1, 0, 0.8f);
                dc.DrawWireBox(Vec3(-size.x, -size.y, -size.z), Vec3(size.x, size.y, size.z));
            }
        }
        dc.PopMatrix();
    }

    if (IsHighlighted() && !IsFrozen())
    {
        dc.SetLineWidth(0);
    }

    if (!m_visualObject)
    {
        DrawDefault(dc);
    }

    if (m_visualObject)
    {
        Matrix34 tm(wtm);
        float sz = m_helperScale * gSettings.gizmo.helpersScale;
        tm.ScaleColumn(Vec3(sz, sz, sz));

        SRendParams rp;
        rp.AmbientColor = ColorF(1.0f, 1.0f, 1.0f, 1);
        rp.fAlpha = 1;
        rp.pMatrix = &tm;
        rp.pMaterial = m_visualObject->GetMaterial();

        SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(gEnv->p3DEngine->GetRenderingCamera());

        m_visualObject->Render(rp, passInfo);
    }
}

//////////////////////////////////////////////////////////////////////////
//! Get bounding box of object in world coordinate space.
void CEnvironementProbeObject::GetBoundBox(AABB& box)
{
    const float fScale = GetHelperScale();
    const Vec3& position = GetPos();
    const Vec3 sz(fScale * 0.5f, fScale * 0.5f, fScale * 0.5f);
    box.max = position + sz;
    box.min = position - sz;
}

void CEnvironementProbeObject::GetLocalBounds(AABB& aabb)
{
    const float fScale = GetHelperScale();
    const Vec3 sz(fScale * 0.5f, fScale * 0.5f, fScale * 0.5f);
    aabb.max = sz;
    aabb.min = -sz;
}

//////////////////////////////////////////////////////////////////////////
void CEnvironementProbeObject::GenerateCubemap()
{
    QString levelfolder = GetIEditor()->GetGameEngine()->GetLevelPath();
    QString levelname = Path::GetFile(levelfolder).toLower();
    QString fullGameFolder = QString(Path::GetEditingGameDataFolder().c_str());
    QString texturename = (GetName() + QString("_cm.tif")).toLower();

    QString fullFolder = Path::SubDirectoryCaseInsensitive(fullGameFolder, {"textures", "cubemaps", levelname});
    QString fullFilename = QDir(fullFolder).absoluteFilePath(texturename);
    QString relFilename = QDir(fullGameFolder).relativeFilePath(fullFilename);

    CFileUtil::CreateDirectory(fullFolder.toUtf8().data());

    int cubemapres = 256;
    m_cubemap_resolution->Get(cubemapres);
    if (cubemapres > 0 && CubemapUtils::GenCubemapWithObjectPathAndSize(fullFilename, this, (int)cubemapres, true))
    {
        IVariable* pVar = GetProperties()->FindVariable("texture_deferred_cubemap", true);
        if (!pVar)
        {
            return;
        }

        Path::ConvertBackSlashToSlash(relFilename);
        if (GetIEditor()->GetConsoleVar("ed_lowercasepaths"))
        {
            relFilename = relFilename.toLower();
        }

        // the actual asset name should be a dds.
        pVar->Set(Path::ReplaceExtension(relFilename, "dds"));

        if (m_visualObject)
        {
            m_visualObject->SetMaterial(CreateMaterial());
        }
        UpdatePropertyPanels();
        UpdateLinks();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEnvironementProbeTODObject::GenerateCubemap()
{
    QString levelfolder = QString(GetIEditor()->GetGameEngine()->GetLevelPath());
    QString levelname = Path::GetFile(levelfolder).toLower();
    QString fullGameFolder = Path::GetEditingGameDataFolder().c_str();
    QString fullFolder = Path::SubDirectoryCaseInsensitive(fullGameFolder, {"textures", "cubemaps", levelname});
    QString relFolder = QDir(fullGameFolder).relativeFilePath(fullFolder);

    CFileUtil::CreateDirectory(fullFolder.toUtf8().data());

    int cubemapres = 256;
    m_cubemap_resolution->Get(cubemapres);
    float fCurTime = gEnv->p3DEngine->GetTimeOfDay()->GetTime();

    for (int i = 1; i <= m_timeSlots; i++)
    {
        QString texturename = (GetName() + QString(string().Format("_tod%d_cm.tif", i))).toLower();
        QString fullFilenameTOD = QDir(fullFolder).absoluteFilePath(texturename);
        QString relFilenameTOD = QDir(fullGameFolder).relativeFilePath(fullFilenameTOD);

        IVariable* pTODVar = GetProperties()->FindVariable(string().Format("TimeOfDay%d", i), true)->FindVariable("fHour", true);
        if (pTODVar)
        {
            float hour;
            pTODVar->Get(hour);
            gEnv->p3DEngine->GetTimeOfDay()->SetTime(hour, true);

            if (cubemapres > 0 && CubemapUtils::GenCubemapWithObjectPathAndSize(fullFilenameTOD, this, (int)cubemapres, true))
            {
                IVariable* pTexVar = GetProperties()->FindVariable(string().Format("TimeOfDay%d", i), true)->FindVariable(string().Format("texture_deferred_cubemap_tod%d", i), true);
                if (!pTexVar)
                {
                    return;
                }

                Path::ConvertBackSlashToSlash(relFilenameTOD);
                if (GetIEditor()->GetConsoleVar("ed_lowercasepaths"))
                {
                    relFilenameTOD = relFilenameTOD.toLower();
                }
                pTexVar->Set(relFilenameTOD);

                if (m_visualObject)
                {
                    m_visualObject->SetMaterial(CreateMaterial());
                }
            }
        }
    }

    gEnv->p3DEngine->GetTimeOfDay()->SetTime(fCurTime, true);
    UpdatePropertyPanels();
    UpdateLinks();
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CEnvironementProbeObject::CreateMaterial()
{
    QString deferredCubemapPath;
    QString matName;
    if (!GetProperties())
    {
        return NULL;
    }

    IVariable* pVar = GetProperties()->FindVariable("texture_deferred_cubemap", true);
    if (!pVar)
    {
        return NULL;
    }

    pVar->Get(deferredCubemapPath);

    matName = Path::GetFileName(deferredCubemapPath);
    IMaterialManager*       pMatMan = GetIEditor()->Get3DEngine()->GetMaterialManager();
    _smart_ptr<IMaterial>   pMatSrc = pMatMan->LoadMaterial("Editor/Objects/envcube", false, true);
    if (pMatSrc)
    {
        _smart_ptr<IMaterial>   pMatDst = pMatMan->CreateMaterial(matName.toUtf8().data(), pMatSrc->GetFlags() | MTL_FLAG_NON_REMOVABLE);
        if (pMatDst)
        {
            SShaderItem&            si = pMatSrc->GetShaderItem();
            SInputShaderResources   isr = si.m_pShaderResources;
            // The following will create the environment map slot if did not exist
            isr.m_TexturesResourcesMap[ EFTT_ENV ].m_Name = deferredCubemapPath.toUtf8().data();

            SShaderItem             siDst = GetIEditor()->GetRenderer()->EF_LoadShaderItem(si.m_pShader->GetName(), true, 0, &isr, si.m_pShader->GetGenerationMask());
            pMatDst->AssignShaderItem(siDst);
            return pMatDst;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CEnvironementProbeObject::OnPreviewCubemap(IVariable*  piVariable)
{
    bool preview = false;
    piVariable->Get(preview);
    ToggleCubemapPreview(preview);
}

void CEnvironementProbeObject::ToggleCubemapPreview( bool enable )
{
    if (enable)
    {
        m_visualObject = GetIEditor()->Get3DEngine()->LoadStatObjUnsafeManualRef("Editor/Objects/envcube.cgf", nullptr, nullptr, false);
        if (m_visualObject)
        {
            m_visualObject = m_visualObject->Clone(false, false, false);
            m_visualObject->SetMaterial(CreateMaterial());
            m_visualObject->AddRef();
        }
    }
    else
    {
        if (m_visualObject)
        {
            m_visualObject->Release();
        }
        m_visualObject = NULL;
    }
}

int CEnvironementProbeObject::AddEntityLink(const QString& name, GUID targetEntityId)
{
    int ret = CEntityObject::AddEntityLink(name, targetEntityId);
    UpdateLinks();
    return ret;
}

void CEnvironementProbeObject::UpdateLinks()
{
    int count = GetEntityLinkCount();
    for (int idx = 0; idx < count; idx++)
    {
        CEntityLink link = GetEntityLink(idx);
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(link.targetId);
        if (!pObject)
        {
            continue;
        }

        CEntityObject* pTarget = NULL;
        if (qobject_cast<CEntityObject*>(pObject))
        {
            pTarget = (CEntityObject*)pObject;
        }

        if (!pTarget)
        {
            continue;
        }

        CVarBlock* pVarBlock = NULL;
        QString type = pObject->GetTypeDescription();
        if (QString::compare(type, "Light", Qt::CaseInsensitive) == 0)
        {
            pVarBlock = pTarget->GetProperties();
            if (!pVarBlock)
            {
                continue;
            }
        }
        else if (QString::compare(type, "DestroyableLight", Qt::CaseInsensitive) == 0 || QString::compare(type, "RigidBodyLight", Qt::CaseInsensitive) == 0)
        {
            pVarBlock = pTarget->GetProperties2();
            if (!pVarBlock)
            {
                continue;
            }
        }
        else
        {
            continue;
        }

        CVarBlock* pProperties = GetProperties();
        if (!pProperties || !pVarBlock)
        {
            continue;
        }

        IVariable* pDstDeferredCubemap = pVarBlock->FindVariable("texture_deferred_cubemap", true);
        IVariable* pDeferredCubemap = pProperties->FindVariable("texture_deferred_cubemap", true);

        if (!pDstDeferredCubemap || !pDeferredCubemap)
        {
            continue;
        }

        bool bDeferred = false;
        QString strCubemap;

        pDeferredCubemap->Get(strCubemap);
        pDstDeferredCubemap->Set(strCubemap);
    }
}

void CEnvironementProbeTODObject::UpdateLinks()
{
    int count = GetEntityLinkCount();
    for (int idx = 0; idx < count; idx++)
    {
        CEntityLink link = GetEntityLink(idx);
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(link.targetId);
        if (!pObject)
        {
            continue;
        }

        CEntityObject* pTarget = NULL;
        if (qobject_cast<CEntityObject*>(pObject))
        {
            pTarget = (CEntityObject*)pObject;
        }

        if (!pTarget)
        {
            continue;
        }

        CVarBlock* pVarBlock = NULL;
        QString type = pObject->GetTypeDescription();
        if (QString::compare(type, "Light", Qt::CaseInsensitive) == 0)
        {
            pVarBlock = pTarget->GetProperties();
            if (!pVarBlock)
            {
                continue;
            }
        }
        else if (QString::compare(type, "DestroyableLight", Qt::CaseInsensitive) == 0 || QString::compare(type, "RigidBodyLight", Qt::CaseInsensitive) == 0)
        {
            pVarBlock = pTarget->GetProperties2();
            if (!pVarBlock)
            {
                continue;
            }
        }
        else
        {
            continue;
        }
    }
}

void CEnvironementProbeObject::OnPropertyChanged(IVariable* pVar)
{
    UpdateLinks();

    if (QString::compare(pVar->GetName(), "fHour", Qt::CaseInsensitive) == 0)
    {
        float hour;
        pVar->Get(hour);
        if (hour < float(0.0))
        {
            pVar->Set(float(0.0));
        }
        else if (hour > float(24.0))
        {
            pVar->Set(float(24.0));
        }
    }

    CBaseObject::OnPropertyChanged(pVar);
}

void CEnvironementProbeObject::OnMultiSelPropertyChanged(IVariable* pVar)
{
    UpdateLinks();
    CBaseObject::OnMultiSelPropertyChanged(pVar);
}

#include <Objects/EnvironmentProbeObject.moc>
