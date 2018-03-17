
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

#include <IGameFramework.h>
#include <IGame.h>

#include "ModelViewport.h"

#include "ThumbnailGenerator.h"
#include "GameEngine.h"


#include "ICryAnimation.h"
#include "IAttachment.h"
#include "CryCharMorphParams.h"
#include "CryCharAnimationParams.h"
#include "IFacialAnimation.h"

#include "FileTypeUtils.h"
#include "Objects/DisplayContext.h"
#include "Material/MaterialManager.h"
#include "ErrorReport.h"

#include <I3DEngine.h>
#include <IPhysics.h>
#include <ITimer.h>
#include "IRenderAuxGeom.h"
#include "DisplaySettings.h"
#include "IViewSystem.h"
#include "IEntityHelper.h"
#include <Components/IComponentAudio.h>

#include <IGameFramework.h>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>

#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>

uint32 g_ypos = 0;

#define SKYBOX_NAME "InfoRedGal"

namespace
{
    int s_varsPanelId = 0;
    ReflectedPropertiesPanel* s_varsPanel = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CModelViewport

CModelViewport::CModelViewport(const char* settingsPath, QWidget* parent)
    : CRenderViewport(tr("Model View"), parent)
{
    m_settingsPath = QString::fromLatin1(settingsPath);

    m_pCharacterBase = 0;
    m_bPaused = false;

    m_Camera.SetFrustum(800, 600, 3.14f / 4.0f, 0.02f, 10000);

    m_pAnimationSystem = GetIEditor()->GetSystem()->GetIAnimationSystem();

    m_bInRotateMode = false;
    m_bInMoveMode = false;

    m_rollupIndex = 0;
    m_rollupIndex2 = 0;

    m_object = 0;


    m_weaponModel = 0;
    m_attachedCharacter = 0;

    m_camRadius = 10;

    m_moveSpeed = 0.1f;
    m_LightRotationRadian = 0.0f;

    m_weaponIK = false;

    m_pRESky = 0;
    m_pSkyboxName = 0;
    m_pSkyBoxShader = NULL;
    m_pPhysicalEntity = NULL;

    m_attachBone = QStringLiteral("weapon_bone");

    // Init variable.
    mv_objectAmbientColor = Vec3(0.25f, 0.25f, 0.25f);
    mv_backgroundColor      = Vec3(0.25f, 0.25f, 0.25f);

    mv_lightDiffuseColor = Vec3(0.70f, 0.70f, 0.70f);
    mv_lightMultiplier = 3.0f;
    mv_lightOrbit = 15.0f;
    mv_lightRadius = 400.0f;
    mv_lightSpecMultiplier = 1.0f;

    mv_showPhysics = false;

    m_GridOrigin = Vec3(ZERO);
    m_arrAnimatedCharacterPath.resize(0x200, ZERO);
    m_arrSmoothEntityPath.resize(0x200, ZERO);

    m_arrRunStrafeSmoothing.resize(0x100);
    SetPlayerPos();

    //--------------------------------------------------
    // Register variables.
    //--------------------------------------------------
    m_vars.AddVariable(mv_showPhysics, "Display Physics");
    m_vars.AddVariable(mv_useCharPhysics, "Use Character Physics", functor(*this, &CModelViewport::OnCharPhysics));
    mv_useCharPhysics = true;
    m_vars.AddVariable(mv_showGrid, "ShowGrid");
    mv_showGrid = true;
    m_vars.AddVariable(mv_showBase, "ShowBase");
    mv_showBase = false;
    m_vars.AddVariable(mv_showLocator, "ShowLocator");
    mv_showLocator = 0;
    m_vars.AddVariable(mv_InPlaceMovement, "InPlaceMovement");
    mv_InPlaceMovement = false;
    m_vars.AddVariable(mv_StrafingControl, "StrafingControl");
    mv_StrafingControl = false;

    m_vars.AddVariable(mv_lighting, "Lighting");
    mv_lighting = true;
    m_vars.AddVariable(mv_animateLights, "AnimLights");

    m_vars.AddVariable(mv_backgroundColor, "BackgroundColor", functor(*this, &CModelViewport::OnLightColor), IVariable::DT_COLOR);
    m_vars.AddVariable(mv_objectAmbientColor, "ObjectAmbient", functor(*this, &CModelViewport::OnLightColor), IVariable::DT_COLOR);

    m_vars.AddVariable(mv_lightDiffuseColor, "LightDiffuse", functor(*this, &CModelViewport::OnLightColor), IVariable::DT_COLOR);
    m_vars.AddVariable(mv_lightMultiplier, "Light Multiplier", functor(*this, &CModelViewport::OnLightMultiplier), IVariable::DT_SIMPLE);
    m_vars.AddVariable(mv_lightSpecMultiplier, "Light Specular Multiplier", functor(*this, &CModelViewport::OnLightMultiplier), IVariable::DT_SIMPLE);
    m_vars.AddVariable(mv_lightRadius, "Light Radius", functor(*this, &CModelViewport::OnLightMultiplier), IVariable::DT_SIMPLE);
    m_vars.AddVariable(mv_lightOrbit, "Light Orbit", functor(*this, &CModelViewport::OnLightMultiplier), IVariable::DT_SIMPLE);

    m_vars.AddVariable(mv_showWireframe1, "ShowWireframe1");
    m_vars.AddVariable(mv_showWireframe2, "ShowWireframe2");
    m_vars.AddVariable(mv_showTangents, "ShowTangents");
    m_vars.AddVariable(mv_showBinormals, "ShowBinormals");
    m_vars.AddVariable(mv_showNormals, "ShowNormals");

    m_vars.AddVariable(mv_showSkeleton, "ShowSkeleton");
    m_vars.AddVariable(mv_showJointNames, "ShowJointNames");
    m_vars.AddVariable(mv_showJointsValues, "ShowJointsValues");
    m_vars.AddVariable(mv_showStartLocation, "ShowInvStartLocation");
    m_vars.AddVariable(mv_showMotionParam, "ShowMotionParam");
    m_vars.AddVariable(mv_printDebugText, "PrintDebugText");

    m_vars.AddVariable(mv_UniformScaling, "UniformScaling");
    mv_UniformScaling = 1.0f;
    mv_UniformScaling.SetLimits(0.01f, 2.0f);
    m_vars.AddVariable(mv_forceLODNum, "ForceLODNum");
    mv_forceLODNum = 0;
    mv_forceLODNum.SetLimits(0, 10);
    m_vars.AddVariable(mv_showShaders, "ShowShaders", functor(*this, &CModelViewport::OnShowShaders));
    m_vars.AddVariable(mv_AttachCamera, "AttachCamera");

    m_vars.AddVariable(mv_fov, "FOV");
    mv_fov = 60;
    mv_fov.SetLimits(1, 120);

    RestoreDebugOptions();

    m_camRadius = 10;

    //YPR_Angle =   Ang3(0,-1.0f,0);
    //SetViewTM( Matrix34(CCamera::CreateOrientationYPR(YPR_Angle), Vec3(0,-m_camRadius,0))  );
    Vec3 camPos = Vec3(10, 10, 10);
    Matrix34 tm = Matrix33::CreateRotationVDir((Vec3(0, 0, 0) - camPos).GetNormalized());
    tm.SetTranslation(camPos);
    SetViewTM(tm);

    if (GetIEditor()->IsInPreviewMode())
    {
        // In preview mode create a simple physical grid, so we can register physical entities.
        int nCellSize = 4;
        IPhysicalWorld* pPhysWorld = gEnv->pPhysicalWorld;
        if (pPhysWorld)
        {
            pPhysWorld->SetupEntityGrid(2, Vec3(0, 0, 0), 10, 10, 1, 1);
        }
    }

    // Audio
    m_pAudioListener = NULL;

    m_AABB.Reset();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SaveDebugOptions() const
{
    QSettings settings;
    for (auto g : m_settingsPath.split('\\'))
        settings.beginGroup(g);

    CVarBlock* vb = GetVarObject()->GetVarBlock();
    int32 vbCount = vb->GetNumVariables();

    settings.setValue("iDebugOptionCount", vbCount);

    char keyType[64], keyValue[64];
    for (int32 i = 0; i < vbCount; ++i)
    {
        IVariable* var = vb->GetVariable(i);
        IVariable::EType vType = var->GetType();
        sprintf_s(keyType, "DebugOption_%s_type", var->GetName().toUtf8().data());
        sprintf_s(keyValue, "DebugOption_%s_value", var->GetName().toUtf8().data());
        switch (vType)
        {
        case IVariable::UNKNOWN:
        {
            break;
        }
        case IVariable::INT:
        {
            int32 value = 0;
            var->Get(value);
            settings.setValue(keyType, IVariable::INT);
            settings.setValue(keyValue, value);

            break;
        }
        case IVariable::BOOL:
        {
            bool value = 0;
            var->Get(value);
            settings.setValue(keyType, IVariable::BOOL);
            settings.setValue(keyValue, value);
            break;
        }
        case IVariable::FLOAT:
        {
            f32 value = 0;
            var->Get(value);
            settings.setValue(keyType, IVariable::FLOAT);
            settings.setValue(keyValue, value);
            break;
        }
        case IVariable::VECTOR:
        {
            Vec3 value;
            var->Get(value);
            f32 valueArray[3];
            valueArray[0] = value.x;
            valueArray[1] = value.y;
            valueArray[2] = value.z;
            settings.setValue(keyType, IVariable::VECTOR);
            settings.setValue(keyValue, QByteArray(reinterpret_cast<const char*>(&value), 3 * sizeof(f32)));

            break;
        }
        case IVariable::QUAT:
        {
            Quat value;
            var->Get(value);
            f32 valueArray[4];
            valueArray[0] = value.w;
            valueArray[1] = value.v.x;
            valueArray[2] = value.v.y;
            valueArray[3] = value.v.z;

            settings.setValue(keyType, IVariable::QUAT);
            settings.setValue(keyValue, QByteArray(reinterpret_cast<const char*>(&value), 4 * sizeof(f32)));

            break;
        }
        case IVariable::STRING:
        {
            QString value;
            var->Get(value);
            settings.setValue(keyType, IVariable::STRING);
            settings.setValue(keyValue, value);

            break;
        }
        case IVariable::ARRAY:
        {
            break;
        }
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::RestoreDebugOptions()
{
    QSettings settings;
    for (auto g : m_settingsPath.split('\\'))
        settings.beginGroup(g);

    f32 floatValue = .0f;
    UINT byteNum = sizeof(f32);
    QString strRead = "";
    int32 iRead = 0;
    BOOL bRead = FALSE;
    f32 fRead = .0f;
    QByteArray pbtData;
    UINT bytes = 0;

    CVarBlock* vb = m_vars.GetVarBlock();
    int32 vbCount = vb->GetNumVariables();

    char keyType[64], keyValue[64];
    for (int32 i = 0; i < vbCount; ++i)
    {
        IVariable* var = vb->GetVariable(i);
        IVariable::EType vType = var->GetType();
        sprintf_s(keyType, "DebugOption_%s_type", var->GetName().toUtf8().data());

        int32 iType = settings.value(keyType, 0).toInt();

        sprintf_s(keyValue, "DebugOption_%s_value", var->GetName().toUtf8().data());
        switch (iType)
        {
        case IVariable::UNKNOWN:
        {
            break;
        }
        case IVariable::INT:
        {
            iRead = settings.value(keyValue, 0).toInt();
            var->Set(iRead);
            break;
        }
        case IVariable::BOOL:
        {
            bRead = settings.value(keyValue, FALSE).toBool();
            var->Set(bRead);
            break;
        }
        case IVariable::FLOAT:
        {
            fRead = settings.value(keyValue).toDouble();
            var->Set(fRead);
            break;
        }
        case IVariable::VECTOR:
        {
            pbtData = settings.value(keyValue).toByteArray();
            assert(pbtData.count() == 3 * sizeof(f32));
            f32* pfRead = reinterpret_cast<f32*>(pbtData.data());

            Vec3 vecRead(pfRead[0], pfRead[1], pfRead[2]);
            var->Set(vecRead);
            break;
        }
        case IVariable::QUAT:
        {
            pbtData = settings.value(keyValue).toByteArray();
            assert(pbtData.count() == 4 * sizeof(f32));
            f32* pfRead = reinterpret_cast<f32*>(pbtData.data());

            Quat valueRead(pfRead[0], pfRead[1], pfRead[2], pfRead[3]);
            var->Set(valueRead);
            break;
        }
        case IVariable::STRING:
        {
            strRead = settings.value(keyValue, "").toString();
            var->Set(strRead);
            break;
        }
        case IVariable::ARRAY:
        {
            break;
        }
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CModelViewport::~CModelViewport()
{
    OnDestroy();
    ReleaseObject();

    GetIEditor()->FlushUndo();

    SaveDebugOptions();

    // Audio
    if (m_pAudioListener != NULL)
    {
        gEnv->pEntitySystem->RemoveEntity(m_pAudioListener->GetId(), true);
        assert(m_pAudioListener == NULL);
    }

    gEnv->pPhysicalWorld->GetPhysVars()->helperOffset.zero();
    GetIEditor()->SetConsoleVar("ca_UsePhysics", 1);
}

/////////////////////////////////////////////////////////////////////////////
// CModelViewport message handlers
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CModelViewport::ReleaseObject()
{
    if (m_object)
    {
        m_object->Release();
        m_object = NULL;
    }

    if (GetCharacterBase())
    {
        m_pCharacterBase = 0;
        m_pAnimationSystem->DeleteDebugInstances();
    }

    if (m_weaponModel)
    {
        m_weaponModel->Release();
        m_weaponModel = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::LoadObject(const QString& fileName, float scale)
{
    m_bPaused = false;

    // Load object.
    QString file = Path::MakeGamePath(fileName);

    bool reload = false;
    if (m_loadedFile == file)
    {
        reload = true;
    }
    m_loadedFile = file;

    SetName(tr("Model View - %1").arg(file));

    ReleaseObject();

    // Enables display of warning after model have been loaded.
    CErrorsRecorder errRecorder;

    if (IsPreviewableFileType(file.toUtf8().data()))
    {
        const QString fileExt = QFileInfo(file).completeSuffix();
        // Try Load character.
        const bool isSKEL = (0 == fileExt.compare(CRY_SKEL_FILE_EXT, Qt::CaseInsensitive));
        const bool isSKIN = (0 == fileExt.compare(CRY_SKIN_FILE_EXT, Qt::CaseInsensitive));
        const bool isCGA = (0 == fileExt.compare(CRY_ANIM_GEOMETRY_FILE_EXT, Qt::CaseInsensitive));
        const bool isCDF = (0 == fileExt.compare(CRY_CHARACTER_DEFINITION_FILE_EXT, Qt::CaseInsensitive));
        if (isSKEL || isSKIN || isCGA || isCDF)
        {
            m_pCharacterBase = m_pAnimationSystem->CreateInstance(file.toUtf8().data());
            if (GetCharacterBase())
            {
                f32  radius = GetCharacterBase()->GetAABB().GetRadius();
                Vec3 center = GetCharacterBase()->GetAABB().GetCenter();

                m_AABB.min = center - Vec3(radius, radius, radius);
                m_AABB.max = center + Vec3(radius, radius, radius);

                if (!reload)
                {
                    m_camRadius = center.z + radius;
                }
            }
        }
        else
        {
            LoadStaticObject(file);
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Preview Error"), tr("Preview of this file type not supported."));
        return;
    }

    //--------------------------------------------------------------------------------

    if (!s_varsPanel)
    {
        // Regedit variable panel.
        s_varsPanel = new ReflectedPropertiesPanel(this);
        s_varsPanel->Setup();
        s_varsPanel->AddVars(m_vars.GetVarBlock());
        s_varsPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Debug Options", s_varsPanel);
    }

    if (!reload)
    {
        Vec3 v = m_AABB.max - m_AABB.min;
        float radius = v.GetLength() / 2.0f;
        m_camRadius = radius * 2;
    }

    if (GetIEditor()->IsInPreviewMode())
    {
        Physicalize();
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::LoadStaticObject(const QString& file)
{
    if (m_object)
    {
        m_object->Release();
    }

    // Load Static object.
    m_object = m_engine->LoadStatObjUnsafeManualRef(file.toUtf8().data(), 0, 0, false);

    if (!m_object)
    {
        CLogFile::WriteLine("Loading of object failed.");
        return;
    }
    m_object->AddRef();

    // Generate thumbnail for this cgf.
    CThumbnailGenerator thumbGen;
    thumbGen.GenerateForFile(file);

    m_AABB.min = m_object->GetBoxMin();
    m_AABB.max = m_object->GetBoxMax();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnRender()
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    const QRect rc = contentsRect();
    ProcessKeys();
    if (m_renderer)
    {
        PreWidgetRendering();

        m_Camera.SetFrustum(m_Camera.GetViewSurfaceX(), m_Camera.GetViewSurfaceZ(), m_Camera.GetFov(), 0.02f, 10000, m_Camera.GetPixelAspectRatio());
        const int w = rc.width();
        const int h = rc.height();
        m_Camera.SetFrustum(w, h, DEG2RAD(mv_fov), 0.0101f, 10000.0f);

        if (GetIEditor()->IsInPreviewMode())
        {
            GetISystem()->SetViewCamera(m_Camera);
        }

        Vec3 clearColor = mv_backgroundColor;
        m_renderer->SetClearColor(clearColor);
        m_renderer->SetCamera(m_Camera);

        auto colorf = ColorF(clearColor, 1.0f);
        m_renderer->ClearTargetsImmediately(FRT_CLEAR | FRT_CLEAR_IMMEDIATE, colorf);
        m_renderer->ResetToDefault();

        SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(m_Camera, SRenderingPassInfo::DEFAULT_FLAGS, true);

        {
            CScopedWireFrameMode scopedWireFrame(m_renderer, mv_showWireframe1 ? R_WIREFRAME_MODE : R_SOLID_MODE);
            DrawModel(passInfo);
        }

        ICharacterManager* pCharMan = gEnv->pCharacterManager;
        if (pCharMan)
        {
            pCharMan->UpdateStreaming(-1, -1);
        }

        PostWidgetRendering();
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawSkyBox(const SRenderingPassInfo& passInfo)
{
    CRenderObject* pObj = m_renderer->EF_GetObject_Temp(passInfo.ThreadID());
    pObj->m_II.m_Matrix.SetTranslationMat(GetViewTM().GetTranslation());

    if (m_pSkyboxName)
    {
        SShaderItem skyBoxShaderItem(m_pSkyBoxShader);
        m_renderer->EF_AddEf(m_pRESky, skyBoxShaderItem, pObj, passInfo, EFSLIST_GENERAL, 1, SRendItemSorter::CreateRendItemSorter(passInfo));
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimBack()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimFastBack()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimFastForward()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimFront()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnAnimPlay()
{
    // TODO: Add your command handler code here
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::mouseDoubleClickEvent(QMouseEvent* event)
{
    // TODO: Add your message handler code here and/or call default

    CRenderViewport::mouseDoubleClickEvent(event);
    if (event->button() != Qt::LeftButton)
    {
        return;
    }
    Matrix34 tm;
    tm.SetIdentity();
    SetViewTM(tm);
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CModelViewport::keyPressEvent(QKeyEvent* event)
{
    if (GetCharacterBase())
    {
        const UINT nChar = event->nativeVirtualKey();
        const UINT nFlags = event->nativeModifiers();
        int index = -1;
        if (nChar >= Qt::Key_0 && nChar <= Qt::Key_9 && (nFlags& Qt::KeypadModifier) == 0)
        {
            if (nChar == Qt::Key_0)
            {
                index = 10;
            }
            else
            {
                index = nChar - Qt::Key_1;
            }
        }
        if (nChar >= Qt::Key_0 && nChar <= Qt::Key_9 && (nFlags& Qt::KeypadModifier) != 0)
        {
            index = nChar - Qt::Key_0 + 10;
            if (nChar == Qt::Key_0)
            {
                index = 20;
            }
        }
        //if (nFlags& MK_CONTROL

        IAnimationSet* pAnimations = GetCharacterBase()->GetIAnimationSet();
        if (pAnimations)
        {
            uint32 numAnims = pAnimations->GetAnimationCount();
            if (index >= 0 && index < numAnims)
            {
                const char* animName = pAnimations->GetNameByAnimID(index);
                PlayAnimation(animName);
            }
        }
    }
    CRenderViewport::keyPressEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnLightColor(IVariable* var)
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowNormals(IVariable* var)
{
    bool enable = mv_showNormals;
    GetIEditor()->SetConsoleVar("r_ShowNormals", (enable) ? 1 : 0);
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowTangents(IVariable* var)
{
    bool enable = mv_showTangents;
    GetIEditor()->SetConsoleVar("r_ShowTangents", (enable) ? 1 : 0);
}


//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnCharPhysics(IVariable* var)
{
    bool enable = mv_useCharPhysics;
    GetIEditor()->SetConsoleVar("ca_UsePhysics", enable);
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::AttachObjectToBone(const QString& model, const QString& bone)
{
    if (!GetCharacterBase())
    {
        return;
    }

    IAttachmentObject* pBindable = NULL;
    m_attachedCharacter = m_pAnimationSystem->CreateInstance(model.toUtf8().data());
    if (m_attachedCharacter)
    {
        m_attachedCharacter->AddRef();
    }

    if (m_weaponModel)
    {
        SAFE_RELEASE(m_weaponModel);
    }

    if (!m_attachedCharacter)
    {
        m_weaponModel = m_engine->LoadStatObjUnsafeManualRef(model.toUtf8().data(), 0, 0, false);
        m_weaponModel->AddRef();
    }

    m_attachBone = bone;
    if (!pBindable)
    {
        const QString str = QStringLiteral("Loading of weapon model %1 failed.").arg(model);
        QMessageBox::critical(this, QString(), str);
        CLogFile::WriteLine(str.toUtf8().data());
        return;
    }

    IAttachmentManager* pIAttachments = GetCharacterBase()->GetIAttachmentManager();
    if (!m_attachedCharacter)
    {
        IAttachment* pAttachment = pIAttachments->CreateAttachment("BoneAttachment", CA_BONE, bone.toUtf8().data());
        assert(pAttachment);
        pAttachment->AddBinding(pBindable);
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::AttachObjectToFace(const QString& model)
{
    if (!GetCharacterBase())
    {
        return;
    }

    if (m_weaponModel)
    {
        m_weaponModel->Release();
    }

    IAttachmentObject* pBindable = NULL;
    m_weaponModel = m_engine->LoadStatObjUnsafeManualRef(model.toUtf8().data(), 0, 0, false);
    m_weaponModel->AddRef();

    if (!pBindable)
    {
        const QString str = QStringLiteral("Loading of weapon model %1 failed.").arg(model);
        QMessageBox::critical(this, QString(), str);
        CLogFile::WriteLine(str.toUtf8().data());
        return;
    }
    IAttachmentManager* pAttachments = GetCharacterBase()->GetIAttachmentManager();
    IAttachment* pIAttachment = pAttachments->CreateAttachment("FaceAttachment", CA_FACE);
    pIAttachment->AddBinding(pBindable);
}


//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowShaders(IVariable* var)
{
    bool bEnable = mv_showShaders;
    GetIEditor()->SetConsoleVar("r_ProfileShaders", bEnable);
}

void CModelViewport::OnDestroy()
{
    ReleaseObject();
    if (m_pRESky)
    {
        m_pRESky->Release(false);
    }

    if (m_rollupIndex)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_rollupIndex);
        m_rollupIndex = 0;
    }
    if (m_rollupIndex2)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_rollupIndex2);
        m_rollupIndex2 = 0;
    }
    if (s_varsPanelId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, s_varsPanelId);
        s_varsPanelId = 0;
    }
    s_varsPanel = 0;
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnActivate()
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnDeactivate()
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::Update()
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    CRenderViewport::Update();

    if (m_pAudioListener != NULL)
    {
        m_pAudioListener->SetWorldTM(m_viewTM);
    }
    // Audio: listener update per viewport

    // Update particle effects.
    m_effectManager.Update(QuatT(m_PhysicalLocation));

    DrawInfo();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawInfo() const
{
    if (GetIEditor()->Get3DEngine())
    {
        ICVar* pDisplayInfo = gEnv->pConsole->GetCVar("r_DisplayInfo");
        if (pDisplayInfo && pDisplayInfo->GetIVal() != 0)
        {
            const float fps = gEnv->pTimer->GetFrameRate();
            const float x = (float)gEnv->pRenderer->GetWidth() - 5.0f;

            gEnv->p3DEngine->DrawTextRightAligned(x, 1, "FPS: %.2f", fps);

            int nPolygons, nShadowVolPolys;
            gEnv->pRenderer->GetPolyCount(nPolygons, nShadowVolPolys);
            int nDrawCalls = gEnv->pRenderer->GetCurrentNumberOfDrawCalls();
            gEnv->p3DEngine->DrawTextRightAligned(x, 20, "Tris:%2d,%03d - DP:%d", nPolygons / 1000, nPolygons % 1000, nDrawCalls);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SetCustomMaterial(CMaterial* pMaterial)
{
    m_pCurrentMaterial = pMaterial;
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CModelViewport::GetMaterial()
{
    if (m_pCurrentMaterial)
    {
        return m_pCurrentMaterial;
    }
    else
    {
        _smart_ptr<IMaterial> pMtl = 0;
        if (m_object)
        {
            pMtl = m_object->GetMaterial();
        }
        else if (GetCharacterBase())
        {
            pMtl = GetCharacterBase()->GetIMaterial();
        }

        CMaterial* pCMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(pMtl);
        return pCMaterial;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CModelViewport::CanDrop(const QPoint& point, IDataBaseItem* pItem)
{
    if (!pItem)
    {
        return false;
    }

    if (pItem->GetType() == EDB_TYPE_MATERIAL)
    {
        SetCustomMaterial((CMaterial*)pItem);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::Drop(const QPoint& point, IDataBaseItem* pItem)
{
    if (!pItem)
    {
        SetCustomMaterial(NULL);
        return;
    }

    if (pItem->GetType() == EDB_TYPE_MATERIAL)
    {
        SetCustomMaterial((CMaterial*)pItem);
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::Physicalize()
{
    IPhysicalWorld* pPhysWorld = gEnv->pPhysicalWorld;
    if (!pPhysWorld)
    {
        return;
    }

    if (!m_object && !GetCharacterBase())
    {
        return;
    }

    if (m_pPhysicalEntity)
    {
        pPhysWorld->DestroyPhysicalEntity(m_pPhysicalEntity);
        if (GetCharacterBase())
        {
            if (GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics(-1))
            {
                pPhysWorld->DestroyPhysicalEntity(GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics(-1));
            }
            GetCharacterBase()->GetISkeletonPose()->SetCharacterPhysics(NULL);
        }
    }


    // Add geometries.
    if (m_object)
    {
        m_pPhysicalEntity = pPhysWorld->CreatePhysicalEntity(PE_STATIC);
        if (!m_pPhysicalEntity)
        {
            return;
        }
        pe_geomparams params;
        params.flags = geom_colltype_ray;
        for (int i = 0; i < 4; i++)
        {
            if (m_object->GetPhysGeom(i))
            {
                m_pPhysicalEntity->AddGeometry(m_object->GetPhysGeom(i), &params);
            }
        }
        // Add all sub mesh geometries.
        for (int nobj = 0; nobj < m_object->GetSubObjectCount(); nobj++)
        {
            IStatObj* pStatObj = m_object->GetSubObject(nobj)->pStatObj;
            if (pStatObj)
            {
                params.pMtx3x4 = &m_object->GetSubObject(nobj)->tm;
                for (int i = 0; i < 4; i++)
                {
                    if (pStatObj->GetPhysGeom(i))
                    {
                        m_pPhysicalEntity->AddGeometry(pStatObj->GetPhysGeom(i), &params);
                    }
                }
            }
        }
    }
    else
    {
        if (GetCharacterBase())
        {
            GetCharacterBase()->GetISkeletonPose()->DestroyCharacterPhysics();
            m_pPhysicalEntity = pPhysWorld->CreatePhysicalEntity(PE_LIVING);
            IPhysicalEntity* pCharPhys = GetCharacterBase()->GetISkeletonPose()->CreateCharacterPhysics(m_pPhysicalEntity, 80.0f, -1, 70.0f);
            GetCharacterBase()->GetISkeletonPose()->CreateAuxilaryPhysics(pCharPhys, Matrix34(IDENTITY));
            if (pCharPhys)
            {
                // make sure the skeleton goes before the ropes in the list
                pe_params_pos pp;
                pp.iSimClass = 6;
                pCharPhys->SetParams(&pp);
                pp.iSimClass = 4;
                pCharPhys->SetParams(&pp);
            }
            pe_player_dynamics pd;
            pd.bActive = 0;
            m_pPhysicalEntity->SetParams(&pd);
        }
    }

    // Set materials.
    CMaterial* pMaterial = GetMaterial();
    if (pMaterial)
    {
        // Assign custom material to physics.
        int surfaceTypesId[MAX_SUB_MATERIALS];
        memset(surfaceTypesId, 0, sizeof(surfaceTypesId));
        int numIds = pMaterial->GetMatInfo()->FillSurfaceTypeIds(surfaceTypesId);

        pe_params_part ppart;
        ppart.nMats = numIds;
        ppart.pMatMapping = surfaceTypesId;
        (GetCharacterBase() && GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics() ?
         GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics() : m_pPhysicalEntity)->SetParams(&ppart);
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::RePhysicalize()
{
    m_pPhysicalEntity = NULL;
    Physicalize();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_DONE:
    {
        // In case something destroys our listener entity before we had the chance to remove it.
        if (pEntity->GetId() == m_pAudioListener->GetId())
        {
            gEnv->pEntitySystem->RemoveEntityEventListener(m_pAudioListener->GetId(), ENTITY_EVENT_DONE, this);
            m_pAudioListener = NULL;
        }

        break;
    }
    default:
    {
        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::CreateAudioListener()
{
    if (m_pAudioListener == NULL)
    {
        SEntitySpawnParams oEntitySpawnParams;
        oEntitySpawnParams.sName  = "AudioListener";
        oEntitySpawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioListener");
        m_pAudioListener = gEnv->pEntitySystem->SpawnEntity(oEntitySpawnParams, true);

        if (m_pAudioListener != NULL)
        {
            m_pAudioListener->SetFlags(m_pAudioListener->GetFlags() | ENTITY_FLAG_TRIGGER_AREAS);
            m_pAudioListener->SetFlagsExtended(m_pAudioListener->GetFlagsExtended() | ENTITY_FLAG_EXTENDED_AUDIO_LISTENER);
            gEnv->pEntitySystem->AddEntityEventListener(m_pAudioListener->GetId(), ENTITY_EVENT_DONE, this);
            CryFixedStringT<64> sTemp;
            sTemp.Format("AudioListener(%d)", static_cast<int>(m_pAudioListener->GetId()));
            m_pAudioListener->SetName(sTemp.c_str());

            IComponentAudioPtr pAudioComponent = m_pAudioListener->GetOrCreateComponent<IComponentAudio>();
            CRY_ASSERT(pAudioComponent.get());
        }
        else
        {
            CryFatalError("<Audio>: Audio listener creation failed in CModelViewport::CreateAudioListener!");
        }
    }
    else
    {
        m_pAudioListener->SetFlagsExtended(m_pAudioListener->GetFlagsExtended() | ENTITY_FLAG_EXTENDED_AUDIO_LISTENER);
        m_pAudioListener->InvalidateTM(ENTITY_XFORM_POS);
    }
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    void AllowAnimEventsToTriggerAgain(ICharacterInstance& characterInstance)
    {
        ISkeletonAnim& skeletonAnim = *characterInstance.GetISkeletonAnim();
        for (int layerIndex = 0; layerIndex < ISkeletonAnim::LayerCount; ++layerIndex)
        {
            const int animCount = skeletonAnim.GetNumAnimsInFIFO(layerIndex);
            for (int animIndex = 0; animIndex < animCount; ++animIndex)
            {
                CAnimation& animation = skeletonAnim.GetAnimFromFIFO(layerIndex, animIndex);
                animation.ClearAnimEventsEvaluated();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SetPaused(bool bPaused)
{
    //return;
    if (m_bPaused != bPaused)
    {
        if (bPaused)
        {
            if (GetCharacterBase())
            {
                GetCharacterBase()->SetPlaybackScale(0.0f);
                AllowAnimEventsToTriggerAgain(*GetCharacterBase());
            }
        }
        else
        {
            const float val = 1.0f;
            if (GetCharacterBase())
            {
                GetCharacterBase()->SetPlaybackScale(val);
            }
        }

        m_bPaused = bPaused;
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawModel(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    m_vCamPos = GetCamera().GetPosition();
    const QRect rc = contentsRect();
    //GetISystem()->SetViewCamera( m_Camera );
    IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
    m_renderer->BeginSpawningGeneratingRendItemJobs(passInfo.ThreadID());
    m_renderer->BeginSpawningShadowGeneratingRendItemJobs(passInfo.ThreadID());
    m_renderer->EF_ClearSkinningDataPool();
    m_renderer->EF_StartEf(passInfo);

    //////////////////////////////////////////////////////////////////////////
    // Draw lights.
    //////////////////////////////////////////////////////////////////////////
    if (mv_lighting == true)
    {
        pAuxGeom->DrawSphere(m_VPLight.m_Origin, 0.2f, ColorB(255, 255, 0, 255));
    }

    gEnv->pConsole->GetCVar("ca_DrawWireframe")->Set(mv_showWireframe2);
    gEnv->pConsole->GetCVar("ca_DrawTangents")->Set(mv_showTangents);
    gEnv->pConsole->GetCVar("ca_DrawBinormals")->Set(mv_showBinormals);
    gEnv->pConsole->GetCVar("ca_DrawNormals")->Set(mv_showNormals);

    DrawLights(passInfo);

    //-------------------------------------------------------------
    //------           Render physical Proxy                 ------
    //-------------------------------------------------------------
    TransformationMatrices backupSceneMatrices;

    m_renderer->Set2DMode(rc.right(), rc.bottom(), backupSceneMatrices);
    if (m_pPhysicalEntity)
    {
        const QPoint mousePoint = mapFromGlobal(QCursor::pos());
        Vec3 raySrc, rayDir;
        ViewToWorldRay(mousePoint, raySrc, rayDir);

        ray_hit hit;
        hit.pCollider = 0;
        int flags = rwi_any_hit | rwi_stop_at_pierceable;
        int col = gEnv->pPhysicalWorld->RayWorldIntersection(raySrc, rayDir * 1000.0f, ent_static, flags, &hit, 1);
        if (col > 0)
        {
            int nMatId = hit.idmatOrg;

            string sMaterial;
            if (GetMaterial())
            {
                sMaterial = GetMaterial()->GetMatInfo()->GetSafeSubMtl(nMatId)->GetName();
            }

            ISurfaceType* pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(hit.surface_idx, m_loadedFile.toUtf8().data());
            if (pSurfaceType)
            {
                float color[4] = {1, 1, 1, 1};
                //m_renderer->Draw2dLabel( mousePoint.x+12,mousePoint.y+8,1.2f,color,false,"%s\n%s",sMaterial.c_str(),pSurfaceType->GetName() );
            }
        }
    }

    m_renderer->Unset2DMode(backupSceneMatrices);

    //-----------------------------------------------------------------------------
    //-----            Render Static Object (handled by 3DEngine)              ----
    //-----------------------------------------------------------------------------
    // calculate LOD

    f32 fDistance = GetViewTM().GetTranslation().GetLength();
    SRendParams rp;
    rp.fDistance    = fDistance;

    Matrix34 tm;
    tm.SetIdentity();
    rp.pMatrix = &tm;
    rp.pPrevMatrix = &tm;

    Vec3 vAmbient;
    mv_objectAmbientColor.Get(vAmbient);

    rp.AmbientColor.r = vAmbient.x * mv_lightMultiplier;
    rp.AmbientColor.g = vAmbient.y * mv_lightMultiplier;
    rp.AmbientColor.b = vAmbient.z * mv_lightMultiplier;
    rp.AmbientColor.a = 1;

    rp.nDLightMask = 7;
    if (mv_lighting == false)
    {
        rp.nDLightMask = 0;
    }

    rp.dwFObjFlags  = 0;
    if (m_pCurrentMaterial)
    {
        rp.pMaterial = m_pCurrentMaterial->GetMatInfo();
    }

    // Render particle effects.
    m_effectManager.Render(rp, passInfo);

    //-----------------------------------------------------------------------------
    //-----            Render Static Object (handled by 3DEngine)              ----
    //-----------------------------------------------------------------------------
    if (m_object)
    {
        m_object->Render(rp, passInfo);
        if (mv_showGrid)
        {
            DrawFloorGrid(Quat(IDENTITY), Vec3(ZERO), Matrix33(IDENTITY));
        }

        if (mv_showBase)
        {
            DrawCoordSystem(IDENTITY, 10.0f);
        }
    }

    //-----------------------------------------------------------------------------
    //-----             Render Character (handled by CryAnimation)            ----
    //-----------------------------------------------------------------------------
    if (GetCharacterBase())
    {
        DrawCharacter(GetCharacterBase(), rp, passInfo);
    }

    m_renderer->EF_EndEf3D(SHDF_STREAM_SYNC, -1, -1, passInfo);
}


void CModelViewport::DrawLights(const SRenderingPassInfo& passInfo)
{
    if (mv_animateLights)
    {
        m_LightRotationRadian += m_AverageFrameTime;
    }

    if (m_LightRotationRadian > gf_PI)
    {
        m_LightRotationRadian = -gf_PI;
    }

    Matrix33 LightRot33 =   Matrix33::CreateRotationZ(m_LightRotationRadian);

    Vec3 LPos0 = Vec3(-mv_lightOrbit, mv_lightOrbit, mv_lightOrbit);
    m_VPLight.SetPosition(LightRot33 * LPos0 + m_PhysicalLocation.t);
    Vec3 d = mv_lightDiffuseColor;
    m_VPLight.SetLightColor(ColorF(d.x * mv_lightMultiplier, d.y * mv_lightMultiplier, d.z * mv_lightMultiplier, 0));
    m_VPLight.SetSpecularMult(mv_lightSpecMultiplier);
    m_VPLight.m_fRadius = mv_lightRadius;
    m_VPLight.m_Flags = DLF_SUN | DLF_DIRECTIONAL;

    if (mv_lighting == true)
    {
        m_renderer->EF_ADDDlight(&m_VPLight, passInfo);
    }
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::PlayAnimation(const char* szName)
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawFloorGrid(const Quat& m33, const Vec3& vPhysicalLocation, const Matrix33& rGridRot)
{
    if (!m_renderer)
    {
        return;
    }

    float XR = 45;
    float YR = 45;





    IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
    pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);

    Vec3 axis = m33.GetColumn0();

    Matrix33 SlopeMat33 = rGridRot;
    uint32 GroundAlign = 1;
    if (GroundAlign == 0)
    {
        SlopeMat33 = Matrix33::CreateRotationAA(m_absCurrentSlope, axis);
    }


    m_GridOrigin = Vec3(floorf(vPhysicalLocation.x), floorf(vPhysicalLocation.y), vPhysicalLocation.z);


    Matrix33 ScaleMat33 = IDENTITY;
    Vec3 rh = Matrix33::CreateRotationY(m_absCurrentSlope) * Vec3(1.0f, 0.0f, 0.0f);
    if (rh.x)
    {
        Vec3 xback = SlopeMat33.GetRow(0);
        Vec3 yback = SlopeMat33.GetRow(1);
        f32 ratiox = 1.0f / Vec3(xback.x, xback.y, 0.0f).GetLength();
        f32 ratioy = 1.0f / Vec3(yback.x, yback.y, 0.0f).GetLength();

        f32 ratio = 1.0f / rh.x;
        //  Vec3 h=Vec3((m_GridOrigin.x-vPhysicalLocation.x)*ratiox,(m_GridOrigin.y-vPhysicalLocation.y)*ratioy,0.0f);
        Vec3 h = Vec3(m_GridOrigin.x - vPhysicalLocation.x, m_GridOrigin.y - vPhysicalLocation.y, 0.0f);
        Vec3 nh = SlopeMat33 * h;
        m_GridOrigin.z += nh.z * ratio;

        ScaleMat33 = Matrix33::CreateScale(Vec3(ratiox, ratioy, 0.0f));

        //  float color1[4] = {0,1,0,1};
        //  m_renderer->Draw2dLabel(12,g_ypos,1.6f,color1,false,"h: %f %f %f    h.z: %f   ratio: %f  ratiox: %f ratioy: %f",h.x,h.y,h.z,  nh.z,ratio,ratiox,ratioy);
        //  g_ypos+=18;
    }

    Matrix33 _m33;
    _m33.SetIdentity();
    AABB aabb1 = AABB(Vec3(-0.03f, -YR, -0.001f), Vec3(0.03f, YR, 0.001f));
    OBB _obb1 = OBB::CreateOBBfromAABB(SlopeMat33, aabb1);
    AABB aabb2 = AABB(Vec3(-XR, -0.03f, -0.001f), Vec3(XR, 0.03f, 0.001f));
    OBB _obb2 = OBB::CreateOBBfromAABB(SlopeMat33, aabb2);

    SlopeMat33 = SlopeMat33 * ScaleMat33;
    // Draw grid.
    float step = 0.25f;
    for (float x = -XR; x < XR; x += step)
    {
        Vec3 p0 = Vec3(x, -YR, 0);
        Vec3 p1 = Vec3(x, YR, 0);
        //pAuxGeom->DrawLine( SlopeMat33*p0,RGBA8(0x7f,0x7f,0x7f,0x00), SlopeMat33*p1,RGBA8(0x7f,0x7f,0x7f,0x00) );
        int32 intx = int32(x);
        if (fabsf(intx - x) < 0.001f)
        {
            pAuxGeom->DrawOBB(_obb1, SlopeMat33 * Vec3(x, 0.0f, 0.0f) + m_GridOrigin, 1, RGBA8(0x9f, 0x9f, 0x9f, 0x00), eBBD_Faceted);
        }
        else
        {
            pAuxGeom->DrawLine(SlopeMat33 * p0 + m_GridOrigin, RGBA8(0x7f, 0x7f, 0x7f, 0x00), SlopeMat33 * p1 + m_GridOrigin, RGBA8(0x7f, 0x7f, 0x7f, 0x00));
        }
    }

    for (float y = -YR; y < YR; y += step)
    {
        Vec3 p0 = Vec3(-XR, y, 0);
        Vec3 p1 = Vec3(XR, y, 0);
        //  pAuxGeom->DrawLine( SlopeMat33*p0,RGBA8(0x7f,0x7f,0x7f,0x00), SlopeMat33*p1,RGBA8(0x7f,0x7f,0x7f,0x00) );
        int32 inty = int32(y);
        if (fabsf(inty - y) < 0.001f)
        {
            pAuxGeom->DrawOBB(_obb2, SlopeMat33 * Vec3(0.0f, y, 0.0f) + m_GridOrigin, 1, RGBA8(0x9f, 0x9f, 0x9f, 0x00), eBBD_Faceted);
        }
        else
        {
            pAuxGeom->DrawLine(SlopeMat33 * p0 + m_GridOrigin, RGBA8(0x7f, 0x7f, 0x7f, 0x00), SlopeMat33 * p1 + m_GridOrigin, RGBA8(0x7f, 0x7f, 0x7f, 0x00));
        }
    }

    // TODO - the grid should probably be an IRenderNode at some point
    // flushing grid geometry now so it will not override transparent
    // objects later in the render pipeline.
    pAuxGeom->Commit();
}


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void CModelViewport::DrawCoordSystem(const QuatT& location, f32 length)
{
    IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
    SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags);
    pAuxGeom->SetRenderFlags(renderFlags);

    Vec3 absAxisX   =   location.q.GetColumn0();
    Vec3 absAxisY   =   location.q.GetColumn1();
    Vec3 absAxisZ   = location.q.GetColumn2();

    const f32 scale = 3.0f;
    const f32 size = 0.009f;
    AABB xaabb = AABB(Vec3(-length * scale, -size * scale, -size * scale), Vec3(length * scale, size * scale,   size * scale));
    AABB yaabb = AABB(Vec3(-size * scale, -length * scale, -size * scale), Vec3(size * scale,     length * scale, size * scale));
    AABB zaabb = AABB(Vec3(-size * scale, -size * scale, -length * scale), Vec3(size * scale,     size * scale,     length * scale));

    OBB obb;
    obb =   OBB::CreateOBBfromAABB(Matrix33(location.q), xaabb);
    pAuxGeom->DrawOBB(obb, location.t, 1, RGBA8(0xff, 0x00, 0x00, 0xff), eBBD_Extremes_Color_Encoded);
    pAuxGeom->DrawCone(location.t + absAxisX * length * scale, absAxisX, 0.03f * scale, 0.15f * scale, RGBA8(0xff, 0x00, 0x00, 0xff));

    obb =   OBB::CreateOBBfromAABB(Matrix33(location.q), yaabb);
    pAuxGeom->DrawOBB(obb, location.t, 1, RGBA8(0x00, 0xff, 0x00, 0xff), eBBD_Extremes_Color_Encoded);
    pAuxGeom->DrawCone(location.t + absAxisY * length * scale, absAxisY, 0.03f * scale, 0.15f * scale, RGBA8(0x00, 0xff, 0x00, 0xff));

    obb =   OBB::CreateOBBfromAABB(Matrix33(location.q), zaabb);
    pAuxGeom->DrawOBB(obb, location.t, 1, RGBA8(0x00, 0x00, 0xff, 0xff), eBBD_Extremes_Color_Encoded);
    pAuxGeom->DrawCone(location.t + absAxisZ * length * scale, absAxisZ, 0.03f * scale, 0.15f * scale, RGBA8(0x00, 0x00, 0xff, 0xff));
}


//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnLightMultiplier(IVariable* var)
{
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SetSelected(bool const bSelect)
{
    // If a modelviewport gets activated, and listeners will be activated, disable the main viewport listener and re-enable when you lose focus.
    if (gEnv->pGame)
    {
        IViewSystem* const pIViewSystem = gEnv->pGame->GetIGameFramework()->GetIViewSystem();

        if (pIViewSystem)
        {
            pIViewSystem->SetControlAudioListeners(!bSelect);
        }
    }

    if (bSelect)
    {
        // Make sure we have a valid audio listener entity on an active view!
        CreateAudioListener();
    }
    else if (m_pAudioListener != NULL)
    {
        m_pAudioListener->SetFlagsExtended(m_pAudioListener->GetFlagsExtended() & ~ENTITY_FLAG_EXTENDED_AUDIO_LISTENER);
    }
}

//////////////////////////////////////////////////////////////////////////
int CAnimatedCharacterEffectManager::m_debugAnimationEffects = 0;

//////////////////////////////////////////////////////////////////////////
CAnimatedCharacterEffectManager::CAnimatedCharacterEffectManager()
    :   m_pISkeletonAnim(0)
    , m_pISkeletonPose(0)
    , m_pIDefaultSkeleton(0)
{
    GetIEditor()->RegisterNotifyListener(this);

    if (gEnv->pConsole && !gEnv->pConsole->GetCVar("m_debugAnimationEffects"))
    {
        REGISTER_CVAR(m_debugAnimationEffects, 0, VF_DEV_ONLY, "Non-zero to enable logging for Animated Character Effects");
    }
}

//////////////////////////////////////////////////////////////////////////
CAnimatedCharacterEffectManager::~CAnimatedCharacterEffectManager()
{
    KillAllEffects();
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CAnimatedCharacterEffectManager::SetSkeleton(ISkeletonAnim* pSkeletonAnim, ISkeletonPose* pSkeletonPose, const IDefaultSkeleton* pIDefaultSkeleton)
{
    m_pISkeletonAnim = pSkeletonAnim;
    m_pISkeletonPose = pSkeletonPose;
    m_pIDefaultSkeleton = (IDefaultSkeleton*)pIDefaultSkeleton;
}

//////////////////////////////////////////////////////////////////////////
void CAnimatedCharacterEffectManager::Update(const QuatT& rPhysEntity)
{
    for (int i = 0; i < m_effects.size(); )
    {
        EffectEntry& entry = m_effects[i];

        // If the animation has stopped, kill the effect.
        bool effectStillPlaying = (entry.pEmitter ? entry.pEmitter->IsAlive() : false);
        bool animStillPlaying = IsPlayingAnimation(entry.animID);
        if (animStillPlaying && effectStillPlaying)
        {
            // Update the effect position.
            QuatTS loc;
            GetEffectLoc(loc, entry.boneID, entry.secondBoneID, entry.offset, entry.dir);
            if (entry.pEmitter)
            {
                entry.pEmitter->SetLocation(rPhysEntity * loc);
            }
            ++i;
        }
        else
        {
            if (m_debugAnimationEffects)
            {
                CryLogAlways("CAnimatedCharacterEffectManager::Update(): Killing effect \"%s\" because %s.",
                    (m_effects[i].pEffect ? m_effects[i].pEffect->GetName() : "<EFFECT NULL>"), (effectStillPlaying ? "animation has ended" : "effect has ended"));
            }
            if (m_effects[i].pEmitter)
            {
                m_effects[i].pEmitter->Activate(false);
            }
            m_effects.erase(m_effects.begin() + i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimatedCharacterEffectManager::KillAllEffects()
{
    for (int i = 0, count = m_effects.size(); i < count; ++i)
    {
        if (m_effects[i].pEmitter)
        {
            m_effects[i].pEmitter->Activate(false);
        }
    }
    m_effects.clear();
}

//////////////////////////////////////////////////////////////////////////
void CAnimatedCharacterEffectManager::SpawnEffect(int animID, const char* animName, const char* effectName, const char* boneName, const char* secondBoneName, const Vec3& offset, const Vec3& dir)
{
    // Check whether we are already playing this effect, and if so dont restart it.
    bool alreadyPlayingEffect = false;
    if (!gEnv->pConsole->GetCVar("ca_AllowMultipleEffectsOfSameName")->GetIVal())
    {
        alreadyPlayingEffect = IsPlayingEffect(effectName);
    }

    if (alreadyPlayingEffect)
    {
        if (m_debugAnimationEffects)
        {
            CryLogAlways("CAnimatedCharacterEffectManager::SpawnEffect(): Refusing to start effect \"%s\" because effect is already running.", (effectName ? effectName : "<MISSING EFFECT NAME>"));
        }
    }
    else
    {
        IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(effectName);
        if (!pEffect)
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Anim events cannot find effect \"%s\", requested by animation \"%s\".", (effectName ? effectName : "<MISSING EFFECT NAME>"), (animName ? animName : "<MISSING ANIM NAME>"));
        }
        int boneID = (boneName && boneName[0] && m_pIDefaultSkeleton ? m_pIDefaultSkeleton->GetJointIDByName(boneName) : -1);
        boneID = (boneID == -1 ? 0 : boneID);

        int secondBoneID = (secondBoneName && secondBoneName[0] && m_pIDefaultSkeleton ? m_pIDefaultSkeleton->GetJointIDByName(secondBoneName) : -1);

        QuatTS loc;

        GetEffectLoc(loc, boneID, secondBoneID, offset, dir);
        IParticleEmitter* pEmitter = (pEffect ? pEffect->Spawn(loc, ePEF_Nowhere) : 0);

        if (pEffect && pEmitter)
        {
            if (m_debugAnimationEffects)
            {
                CryLogAlways("CAnimatedCharacterEffectManager::SpawnEffect(): starting effect \"%s\", requested by animation \"%s\".", (effectName ? effectName : "<MISSING EFFECT NAME>"), (animName ? animName : "<MISSING ANIM NAME>"));
            }
            // Make sure the emitter is not rendered in the game.
            m_effects.push_back(EffectEntry(pEffect, pEmitter, boneID, secondBoneID, offset, dir, animID));
        }
    }
}
//////////////////////////////////////////////////////////////////////////

void CAnimatedCharacterEffectManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnCloseScene:
        KillAllEffects();
        break;
    }
}
//////////////////////////////////////////////////////////////////////////

CAnimatedCharacterEffectManager::EffectEntry::EffectEntry(_smart_ptr<IParticleEffect> pEffect, _smart_ptr<IParticleEmitter> pEmitter, int boneID, int secondBoneID, const Vec3& offset, const Vec3& dir, int animID)
    :   pEffect(pEffect)
    , pEmitter(pEmitter)
    , boneID(boneID)
    , secondBoneID(secondBoneID)
    , offset(offset)
    , dir(dir)
    , animID(animID)
{
}

//////////////////////////////////////////////////////////////////////////
CAnimatedCharacterEffectManager::EffectEntry::~EffectEntry()
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimatedCharacterEffectManager::GetEffectLoc(QuatTS& loc, int boneID, int secondBoneID, const Vec3& offset, const Vec3& dir)
{
    if (dir.len2() > 0)
    {
        loc.q = Quat::CreateRotationXYZ(Ang3(dir * 3.14159f / 180.0f));
    }
    else
    {
        loc.q.SetIdentity();
    }
    loc.t = offset;
    loc.s = 1;

    if (m_pISkeletonPose && secondBoneID < 0)
    {
        loc = m_pISkeletonPose->GetAbsJointByID(boneID) * loc;
    }
    else if (m_pISkeletonPose)
    {
        loc = loc.CreateNLerp(m_pISkeletonPose->GetAbsJointByID(boneID), m_pISkeletonPose->GetAbsJointByID(secondBoneID), 0.5f) * loc;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAnimatedCharacterEffectManager::IsPlayingAnimation(int animID)
{
    enum
    {
        NUM_LAYERS = 4
    };

    // Check whether the animation has stopped.
    bool animPlaying = false;
    for (int layer = 0; layer < NUM_LAYERS; ++layer)
    {
        for (int animIndex = 0, animCount = (m_pISkeletonAnim ? m_pISkeletonAnim->GetNumAnimsInFIFO(layer) : 0); animIndex < animCount; ++animIndex)
        {
            CAnimation& anim = m_pISkeletonAnim->GetAnimFromFIFO(layer, animIndex);
            animPlaying = animPlaying || (anim.GetAnimationId() == animID);
        }
    }

    return animPlaying;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimatedCharacterEffectManager::IsPlayingEffect(const char* effectName)
{
    bool isPlaying = false;
    for (int effectIndex = 0, effectCount = m_effects.size(); effectIndex < effectCount; ++effectIndex)
    {
        IParticleEffect* pEffect = m_effects[effectIndex].pEffect;
        if (pEffect && _stricmp(pEffect->GetName(), effectName) == 0)
        {
            isPlaying = true;
        }
    }
    return isPlaying;
}

//////////////////////////////////////////////////////////////////////////
void CAnimatedCharacterEffectManager::Render(SRendParams& params, const SRenderingPassInfo& passInfo)
{
    for (int effectIndex = 0, effectCount = m_effects.size(); effectIndex < effectCount; ++effectIndex)
    {
        IParticleEmitter* pEmitter = m_effects[effectIndex].pEmitter;
        if (pEmitter)
        {
            pEmitter->Update();
            pEmitter->Render(params, passInfo);
        }
    }
}


#include <ModelViewport.moc>
