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

// Description : CCameraObject implementation.


#include "StdAfx.h"
#include <AzCore/Math/MathUtils.h>
#include "CameraObject.h"
#include "../Viewport.h"
#include "RenderViewport.h"
#include "ViewManager.h"
#include "AnimationContext.h"
#include "Components/IComponentCamera.h"

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

#define CAMERA_COLOR QColor(0, 255, 255)
#define CAMERA_CONE_LENGTH 4
#define CAMERABOX_RADIUS 0.7f
#define MIN_FOV_IN_DEG 0.1f

namespace
{
    const float kMinNearZ = 0.002f;
    const float kMaxNearZ = 10.0f;
    const float kMinFarZ = 11.0f;
    const float kMaxFarZ = 100000.0f;
    const float kMinCameraSeed = 0.0f;
    const float kMaxCameraSeed = 100000.0f;
};

//////////////////////////////////////////////////////////////////////////
CCameraObject::CCameraObject()
    : m_listeners(1)
{
    mv_fov = DEG2RAD(60.0f);        // 60 degrees (to fit with current game)
    mv_nearZ = DEFAULT_NEAR;
    mv_farZ = DEFAULT_FAR;
    mv_amplitudeA = Vec3(1.0f, 1.0f, 1.0f);
    mv_amplitudeAMult = 0.0f;
    mv_frequencyA = Vec3(1.0f, 1.0f, 1.0f);
    mv_frequencyAMult = 0.0f;
    mv_noiseAAmpMult = 0.0f;
    mv_noiseAFreqMult = 0.0f;
    mv_timeOffsetA = 0.0f;
    mv_amplitudeB = Vec3(1.0f, 1.0f, 1.0f);
    mv_amplitudeBMult = 0.0f;
    mv_frequencyB = Vec3(1.0f, 1.0f, 1.0f);
    mv_frequencyBMult = 0.0f;
    mv_noiseBAmpMult = 0.0f;
    mv_noiseBFreqMult = 0.0f;
    mv_timeOffsetB = 0.0f;
    m_creationStep = 0;
    mv_cameraShakeSeed = 0;

    SetColor(CAMERA_COLOR);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::Done()
{
    CViewManager* pManager = GetIEditor()->GetViewManager();
    if (pManager && pManager->GetCameraObjectId() == GetId())
    {
        CViewport* pViewport = GetIEditor()->GetViewManager()->GetSelectedViewport();
        if (CRenderViewport* rvp = viewport_cast<CRenderViewport*>(pViewport))
        {
            rvp->SetDefaultCamera();
        }
    }

    CBaseObjectPtr lookat = GetLookAt();

    CEntityObject::Done();

    if (lookat)
    {
        // If look at is also camera class, delete lookat target.
        if (qobject_cast<CCameraObjectTarget*>(lookat))
        {
            GetObjectManager()->DeleteObject(lookat);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CCameraObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = CEntityObject::Init(ie, prev, file);

    m_entityClass = "CameraSource";

    if (prev)
    {
        CBaseObjectPtr prevLookat = prev->GetLookAt();
        if (prevLookat)
        {
            CBaseObjectPtr lookat = GetObjectManager()->NewObject(prevLookat->GetClassDesc(), prevLookat);
            if (lookat)
            {
                lookat->SetPos(prevLookat->GetPos() + Vec3(3, 0, 0));
                GetObjectManager()->ChangeObjectName(lookat, QString(GetName()) + " Target");
                SetLookAt(lookat);
            }
        }
    }
    UpdateCameraEntity();

    return res;
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::InitVariables()
{
    AddVariable(mv_fov, "FOV", functor(*this, &CCameraObject::OnFovChange), IVariable::DT_ANGLE);
    AddVariable(mv_nearZ, "NearZ", functor(*this, &CCameraObject::OnNearZChange), IVariable::DT_SIMPLE);
    mv_nearZ.SetLimits(kMinNearZ, kMaxNearZ);
    AddVariable(mv_farZ, "FarZ", functor(*this, &CCameraObject::OnFarZChange), IVariable::DT_SIMPLE);
    mv_farZ.SetLimits(kMinFarZ, kMaxFarZ);
    mv_cameraShakeSeed.SetLimits(kMinCameraSeed, kMaxCameraSeed);

    mv_shakeParamsArray.DeleteAllVariables();

    AddVariable(mv_shakeParamsArray, "ShakeParams", "Shake Parameters");
    AddVariable(mv_shakeParamsArray, mv_amplitudeA, "AmplitudeA", "Amplitude A", functor(*this, &CCameraObject::OnShakeAmpAChange));
    AddVariable(mv_shakeParamsArray, mv_amplitudeAMult, "AmplitudeAMult", "Amplitude A Mult.", functor(*this, &CCameraObject::OnShakeMultChange));
    AddVariable(mv_shakeParamsArray, mv_frequencyA, "FrequencyA", "Frequency A", functor(*this, &CCameraObject::OnShakeFreqAChange));
    AddVariable(mv_shakeParamsArray, mv_frequencyAMult, "FrequencyAMult", "Frequency A Mult.", functor(*this, &CCameraObject::OnShakeMultChange));
    AddVariable(mv_shakeParamsArray, mv_noiseAAmpMult, "NoiseAAmpMult", "Noise A Amp. Mult.", functor(*this, &CCameraObject::OnShakeNoiseChange));
    AddVariable(mv_shakeParamsArray, mv_noiseAFreqMult, "NoiseAFreqMult", "Noise A Freq. Mult.", functor(*this, &CCameraObject::OnShakeNoiseChange));
    AddVariable(mv_shakeParamsArray, mv_timeOffsetA, "TimeOffsetA", "Time Offset A", functor(*this, &CCameraObject::OnShakeWorkingChange));
    AddVariable(mv_shakeParamsArray, mv_amplitudeB, "AmplitudeB", "Amplitude B", functor(*this, &CCameraObject::OnShakeAmpBChange));
    AddVariable(mv_shakeParamsArray, mv_amplitudeBMult, "AmplitudeBMult", "Amplitude B Mult.", functor(*this, &CCameraObject::OnShakeMultChange));
    AddVariable(mv_shakeParamsArray, mv_frequencyB, "FrequencyB", "Frequency B", functor(*this, &CCameraObject::OnShakeFreqBChange));
    AddVariable(mv_shakeParamsArray, mv_frequencyBMult, "FrequencyBMult", "Frequency B Mult.", functor(*this, &CCameraObject::OnShakeMultChange));
    AddVariable(mv_shakeParamsArray, mv_noiseBAmpMult, "NoiseBAmpMult", "Noise B Amp. Mult.", functor(*this, &CCameraObject::OnShakeNoiseChange));
    AddVariable(mv_shakeParamsArray, mv_noiseBFreqMult, "NoiseBFreqMult", "Noise B Freq. Mult.", functor(*this, &CCameraObject::OnShakeNoiseChange));
    AddVariable(mv_shakeParamsArray, mv_timeOffsetB, "TimeOffsetB", "Time Offset B", functor(*this, &CCameraObject::OnShakeWorkingChange));
    AddVariable(mv_shakeParamsArray, mv_cameraShakeSeed, "CameraShakeSeed", "Random Seed", functor(*this, &CCameraObject::OnCameraShakeSeedChange));
}

void CCameraObject::Serialize(CObjectArchive& ar)
{
    CEntityObject::Serialize(ar);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::SetName(const QString& name)
{
    if (gEnv->pMovieSystem && GetName().isEmpty() == false)
    {
        gEnv->pMovieSystem->OnCameraRenamed(GetName().toUtf8().data(), name.toUtf8().data());
        GetIEditor()->Notify(eNotify_OnReloadTrackView);
    }

    CEntityObject::SetName(name);
    if (GetLookAt())
    {
        GetLookAt()->SetName(QString(GetName()) + " Target");
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::BeginEditParams(IEditor* ie, int flags)
{
    // Skip entity begin edit params.
    CBaseObject::BeginEditParams(ie, flags);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::EndEditParams(IEditor* ie)
{
    // Skip entity end edit params.
    CBaseObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
int CCameraObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown || event == eMouseLUp)
    {
        Vec3 pos;
        if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
        {
            pos = view->MapViewToCP(point);
        }
        else
        {
            // Snap to terrain.
            bool hitTerrain;
            pos = view->ViewToWorld(point, &hitTerrain);
            if (hitTerrain)
            {
                pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y) + 1.0f;
            }
            pos = view->SnapToGrid(pos);
        }

        if (m_creationStep == 1)
        {
            if (GetLookAt())
            {
                GetLookAt()->SetPos(pos);
            }
        }
        else
        {
            SetPos(pos);
        }

        if (event == eMouseLDown && m_creationStep == 0)
        {
            m_creationStep = 1;
        }
        if (event == eMouseMove && 1 == m_creationStep && !GetLookAt())
        {
            float d = pos.GetDistance(GetPos());
            if (d * view->GetScreenScaleFactor(pos) > 1)
            {
                // Create LookAt Target.
                GetIEditor()->ResumeUndo();
                CreateTarget();
                GetIEditor()->SuspendUndo();
            }
        }
        if (eMouseLUp == event && 1 == m_creationStep)
        {
            return MOUSECREATE_OK;
        }

        return MOUSECREATE_CONTINUE;
    }
    return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::CreateTarget()
{
    // Create another camera object for target.
    CCameraObject* camTarget = (CCameraObject*)GetObjectManager()->NewObject("CameraTarget");
    if (camTarget)
    {
        camTarget->SetName(QString(GetName()) + " Target");
        camTarget->SetPos(GetWorldPos() + Vec3(3, 0, 0));
        SetLookAt(camTarget);
    }
}

//////////////////////////////////////////////////////////////////////////
Vec3 CCameraObject::GetLookAtEntityPos() const
{
    Vec3 pos = GetWorldPos();
    if (GetLookAt())
    {
        pos = GetLookAt()->GetWorldPos();

        if (qobject_cast<CEntityObject*>(GetLookAt()))
        {
            IEntity* pIEntity = ((CEntityObject*)GetLookAt())->GetIEntity();
            if (pIEntity)
            {
                pos = pIEntity->GetWorldPos();
            }
        }
    }
    return pos;
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::Display(DisplayContext& dc)
{
    if (GetLookAt())
    {
        Quat rotation = Quat::CreateRotationVDir((GetLookAt()->GetWorldPos() - GetWorldPos()).normalize());
        SetRotation(rotation);
    }
    Matrix34 wtm = GetWorldTM();

    if (m_pEntity)
    {
        // Display the actual entity matrix.
        wtm = m_pEntity->GetWorldTM();
    }

    Vec3 wp = wtm.GetTranslation();

    //float fScale = dc.view->GetScreenScaleFactor(wp) * 0.03f;
    float fScale = GetHelperScale();

    if (IsHighlighted() && !IsFrozen())
    {
        dc.SetLineWidth(3);
    }

    if (GetLookAt())
    {
        // Look at camera.
        if (IsFrozen())
        {
            dc.SetFreezeColor();
        }
        else
        {
            dc.SetColor(GetColor());
        }

        bool bSelected = IsSelected();

        if (bSelected || GetLookAt()->IsSelected())
        {
            dc.SetSelectedColor();
        }

        // Line from source to target.
        dc.DrawLine(wp, GetLookAtEntityPos());

        if (bSelected)
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

        dc.PushMatrix(wtm);

        //Vec3 sz(0.2f*fScale,0.1f*fScale,0.2f*fScale);
        //dc.DrawWireBox( -sz,sz );

        if (bSelected)
        {
            float dist = GetLookAtEntityPos().GetDistance(wtm.GetTranslation());
            DrawCone(dc, dist);
        }
        else
        {
            float dist = CAMERA_CONE_LENGTH / 2.0f;
            DrawCone(dc, dist, fScale);
        }
        dc.PopMatrix();
    }
    else
    {
        // Free camera
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

        dc.PushMatrix(wtm);

        //Vec3 sz(0.2f*fScale,0.1f*fScale,0.2f*fScale);
        //dc.DrawWireBox( -sz,sz );

        float dist = CAMERA_CONE_LENGTH;
        DrawCone(dc, dist, fScale);
        dc.PopMatrix();
    }

    if (IsHighlighted() && !IsFrozen())
    {
        dc.SetLineWidth(0);
    }

    //dc.DrawIcon( ICON_QUAD,wp,0.1f*dc.view->GetScreenScaleFactor(wp) );

    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
bool CCameraObject::HitTestRect(HitContext& hc)
{
    // transform all 8 vertices into world space
    QPoint p = hc.view->WorldToView(GetWorldPos());
    if (hc.rect.contains(p))
    {
        hc.object = this;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CCameraObject::HitHelperTest(HitContext& hc)
{
    if (hc.view && GetIEditor()->GetViewManager()->GetCameraObjectId() == GetId())
    {
        return false;
    }
    return CEntityObject::HitHelperTest(hc);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::GetBoundBox(AABB& box)
{
    Vec3 pos = GetWorldPos();
    float r;
    r = 1 * CAMERA_CONE_LENGTH * GetHelperScale();
    box.min = pos - Vec3(r, r, r);
    box.max = pos + Vec3(r, r, r);
    if (GetLookAt())
    {
        box.Add(GetLookAt()->GetWorldPos());
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::GetLocalBounds(AABB& box)
{
    GetBoundBox(box);
    Matrix34 invTM(GetWorldTM());
    invTM.Invert();
    box.SetTransformedAABB(invTM, box);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnMenuSetAsViewCamera()
{
    CViewport* pViewport = GetIEditor()->GetViewManager()->GetSelectedViewport();
    if (CRenderViewport* pRenderViewport = viewport_cast<CRenderViewport*>(pViewport))
    {
        pRenderViewport->SetCameraObject(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnContextMenu(QMenu* menu)
{
    if (!menu->isEmpty())
    {
        menu->addSeparator();
    }
    QAction* action = menu->addAction("Set As View Camera");
    QObject::connect(action, &QAction::triggered, [this] { OnMenuSetAsViewCamera();
        });

    CEntityObject::OnContextMenu(menu);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::GetConePoints(Vec3 q[4], float dist, const float fAspectRatio)
{
    if (dist > 1e8f)
    {
        dist = 1e8f;
    }
    float ta = (float)tan(0.5f * GetFOV());
    float h = dist * ta;
    float w = h * fAspectRatio;

    q[0] = Vec3(w, dist, h);
    q[1] = Vec3(-w, dist, h);
    q[2] = Vec3(-w, dist, -h);
    q[3] = Vec3(w, dist, -h);
}

void CCameraObject::DrawCone(DisplayContext& dc, float dist, float fScale)
{
    Vec3 q[4];
    GetConePoints(q, dist, dc.view->GetAspectRatio());

    q[0] *= fScale;
    q[1] *= fScale;
    q[2] *= fScale;
    q[3] *= fScale;

    Vec3 org(0, 0, 0);
    dc.DrawLine(org, q[0]);
    dc.DrawLine(org, q[1]);
    dc.DrawLine(org, q[2]);
    dc.DrawLine(org, q[3]);

    // Draw quad.
    dc.DrawPolyLine(q, 4);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnFovChange(IVariable* var)
{
    // No one will need a fov of less-than 0.1.
    if (mv_fov < DEG2RAD(MIN_FOV_IN_DEG))
    {
        mv_fov = DEG2RAD(MIN_FOV_IN_DEG);
    }

    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        const float fov = mv_fov;
        notifier->OnFovChange(RAD2DEG(fov));
    }

    UpdateCameraEntity();
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnNearZChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        float nearZ = mv_nearZ;
        notifier->OnNearZChange(nearZ);
    }

    UpdateCameraEntity();
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnFarZChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        float farZ = mv_farZ;
        notifier->OnFarZChange(farZ);
    }

    UpdateCameraEntity();
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::UpdateCameraEntity()
{
    if (m_pEntity)
    {
        IComponentCameraPtr pCameraComponent = m_pEntity->GetComponent<IComponentCamera>();
        if (pCameraComponent)
        {
            float fov = mv_fov;
            float nearZ = mv_nearZ;
            float farZ = mv_farZ;
            CCamera& cam = pCameraComponent->GetCamera();
            cam.SetFrustum(cam.GetViewSurfaceX(), cam.GetViewSurfaceZ(), fov, nearZ, farZ, cam.GetPixelAspectRatio());
            pCameraComponent->SetCamera(cam);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnShakeAmpAChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnShakeAmpAChange(mv_amplitudeA);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnShakeAmpBChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnShakeAmpBChange(mv_amplitudeB);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnShakeFreqAChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnShakeFreqAChange(mv_frequencyA);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnShakeFreqBChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnShakeFreqBChange(mv_frequencyB);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnShakeMultChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnShakeMultChange(mv_amplitudeAMult, mv_amplitudeBMult, mv_frequencyAMult, mv_frequencyBMult);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnShakeNoiseChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnShakeNoiseChange(mv_noiseAAmpMult, mv_noiseBAmpMult, mv_noiseAFreqMult, mv_noiseBFreqMult);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnShakeWorkingChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnShakeWorkingChange(mv_timeOffsetA, mv_timeOffsetB);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::OnCameraShakeSeedChange(IVariable* var)
{
    for (CListenerSet<ICameraObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnCameraShakeSeedChange(mv_cameraShakeSeed);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::RegisterCameraListener(ICameraObjectListener* pListener)
{
    m_listeners.Add(pListener);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::UnregisterCameraListener(ICameraObjectListener* pListener)
{
    m_listeners.Remove(pListener);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::SpawnEntity()
{
    CEntityObject::SpawnEntity();
    // SpawnEntity() is called when we load a level in the Editor. We need
    // to call UpdateCameraEntity() to ensure the entity's properties are
    // set correctly on load
    UpdateCameraEntity();
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::SetFOV(const float fov)
{
    mv_fov = std::max(MIN_FOV_IN_DEG, fov);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::SetNearZ(const float nearZ)
{
    mv_nearZ = clamp_tpl(nearZ, kMinNearZ, kMaxNearZ);
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::SetAmplitudeAMult(const float amplitudeAMult)
{
    mv_amplitudeAMult = amplitudeAMult;
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::SetAmplitudeBMult(const float amplitudeBMult)
{
    mv_amplitudeBMult = amplitudeBMult;
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::SetFrequencyAMult(const float frequencyAMult)
{
    mv_frequencyAMult = frequencyAMult;
}

//////////////////////////////////////////////////////////////////////////
void CCameraObject::SetFrequencyBMult(const float frequencyBMult)
{
    mv_frequencyBMult = frequencyBMult;
}

//////////////////////////////////////////////////////////////////////////
const float CCameraObject::GetFOV()
{
    // If there is a difference between our local mv_fov and the associated camera component, prefer the
    // camera component. Our stashed mv_fov is the 'View' in a Model-View-Controller architecture,
    // the component is the authorative 'Model'
    if (m_pEntity)
    {
        IComponentCameraPtr pCameraComponent = m_pEntity->GetComponent<IComponentCamera>();
        if (pCameraComponent)
        {
            float componentFoV = pCameraComponent->GetCamera().GetFov();
            if (!AZ::IsClose(componentFoV, mv_fov, FLT_EPSILON))
            {
                // this sets mv_fov, to be returned below
                SetFOV(componentFoV);
            }
        }
    }

    return mv_fov;
}

//////////////////////////////////////////////////////////////////////////
const float CCameraObject::GetNearZ()
{
    // If there is a difference between our local mv_nearZ and the associated camera component, prefer the
    // camera component. Our stashed mv_nearZ is the 'View' in a Model-View-Controller architecture,
    // the component is the authorative 'Model'

    if (m_pEntity)
    {
        IComponentCameraPtr pCameraComponent = m_pEntity->GetComponent<IComponentCamera>();
        if (pCameraComponent)
        {
            float componentNearZ = pCameraComponent->GetCamera().GetNearPlane();
            if (!AZ::IsClose(componentNearZ, mv_nearZ, FLT_EPSILON))
            {
                // this sets mv_nearZ, to be returned below
                SetNearZ(componentNearZ);
            }
        }
    }

    return mv_nearZ;
}

//////////////////////////////////////////////////////////////////////////
//
// CCameraObjectTarget implementation.
//
//////////////////////////////////////////////////////////////////////////
CCameraObjectTarget::CCameraObjectTarget()
{
    SetColor(CAMERA_COLOR);
}

bool CCameraObjectTarget::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    SetColor(CAMERA_COLOR);
    bool res = CEntityObject::Init(ie, prev, file);
    m_entityClass = "CameraTarget";
    return res;
}

//////////////////////////////////////////////////////////////////////////
void CCameraObjectTarget::InitVariables()
{
}

//////////////////////////////////////////////////////////////////////////
void CCameraObjectTarget::Display(DisplayContext& dc)
{
    Vec3 wp = GetWorldPos();

    //float fScale = dc.view->GetScreenScaleFactor(wp) * 0.03f;
    float fScale = GetHelperScale();

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

    Vec3 sz(0.2f * fScale, 0.2f * fScale, 0.2f * fScale);
    dc.DrawWireBox(wp - sz, wp + sz);

    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
bool CCameraObjectTarget::HitTest(HitContext& hc)
{
    Vec3 origin = GetWorldPos();
    float radius = CAMERABOX_RADIUS / 2.0f;

    //float fScale = hc.view->GetScreenScaleFactor(origin) * 0.03f;
    float fScale = GetHelperScale();

    Vec3 w = origin - hc.raySrc;
    w = hc.rayDir.Cross(w);
    float d = w.GetLength();

    if (d < radius * fScale + hc.distanceTolerance)
    {
        hc.dist = hc.raySrc.GetDistance(origin);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CCameraObjectTarget::GetBoundBox(AABB& box)
{
    Vec3 pos = GetWorldPos();
    float r = CAMERABOX_RADIUS;
    box.min = pos - Vec3(r, r, r);
    box.max = pos + Vec3(r, r, r);
}

void CCameraObjectTarget::Serialize(CObjectArchive& ar)
{
    CEntityObject::Serialize(ar);
}

#include <Objects/CameraObject.moc>