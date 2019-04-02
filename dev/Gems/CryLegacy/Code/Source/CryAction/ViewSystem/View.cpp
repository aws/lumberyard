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

#include "CryLegacy_precompiled.h"

#include <Cry_Camera.h>
#include <HMDBus.h>
#include <ITimeDemoRecorder.h>
#include "View.h"
#include "GameObjects/GameObject.h"
#include <AzCore/Math/MathUtils.h>
#include <IStereoRenderer.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <Components/IComponentAudio.h>

//FIXME:not very pretty

static ICVar* pCamShakeMult = 0;
static ICVar* pHmdReferencePoint = 0;

//------------------------------------------------------------------------
CView::CView(ISystem* pSystem)
    : m_pSystem(pSystem)
    , m_linkedTo(0)
    , m_frameAdditiveAngles(0.0f, 0.0f, 0.0f)
    , m_scale(1.0f)
    , m_zoomedScale(1.0f)
    , m_pAudioListener(nullptr)
{
    if (!pCamShakeMult)
    {
        pCamShakeMult = gEnv->pConsole->GetCVar("c_shakeMult");
    }
    if (!pHmdReferencePoint)
    {
        pHmdReferencePoint = gEnv->pConsole->GetCVar("hmd_reference_point");
    }

    CreateAudioListener();
}

//------------------------------------------------------------------------
CView::~CView()
{
    if (m_pAudioListener)
    {
        gEnv->pEntitySystem->RemoveEntityEventListener(m_pAudioListener->GetId(), ENTITY_EVENT_DONE, this);
        gEnv->pEntitySystem->RemoveEntity(m_pAudioListener->GetId(), true);
        m_pAudioListener = nullptr;
    }
}

//-----------------------------------------------------------------------
void CView::Release()
{
    delete this;
}

//------------------------------------------------------------------------
void CView::Update(float frameTime, bool isActive)
{
    //FIXME:some cameras may need to be updated always
    if (!isActive)
    {
        return;
    }

    CGameObjectPtr pLinkedTo = GetLinkedGameObject();
    if (pLinkedTo && !pLinkedTo->CanUpdateView())
    {
        pLinkedTo = NULL;
    }
    IEntity* pEntity = pLinkedTo ? 0 : GetLinkedEntity();

    if (pLinkedTo || pEntity || m_azEntity)
    {
        m_viewParams.SaveLast();

        CCamera* pSysCam = &m_pSystem->GetViewCamera();

        //process screen shaking
        ProcessShaking(frameTime);

        //FIXME:to let the updateView implementation use the correct shakeVector
        m_viewParams.currentShakeShift = m_viewParams.rotation * m_viewParams.currentShakeShift;

        m_viewParams.frameTime = frameTime;
        //update view position/rotation
        if (pLinkedTo)
        {
            pLinkedTo->UpdateView(m_viewParams);
            if (!m_viewParams.position.IsValid())
            {
                m_viewParams.position = m_viewParams.GetPositionLast();
                CRY_ASSERT_MESSAGE(0, "Camera position is invalid, reverting to old position");
            }
            if (!m_viewParams.rotation.IsValid())
            {
                m_viewParams.rotation = m_viewParams.GetRotationLast();
                CRY_ASSERT_MESSAGE(0, "Camera rotation is invalid, reverting to old rotation");
            }
        }
        else if (pEntity)
        {
            Matrix34 mat = pEntity->GetWorldTM();
            mat.OrthonormalizeFast();
            m_viewParams.position = mat.GetTranslation();
            m_viewParams.rotation = Quat(mat);
        }
        else
        {
            if (m_azEntity != nullptr)
            {
                auto entityTransform = m_azEntity->GetTransform();
                if (entityTransform != nullptr)
                {
                    AZ::Transform transform = entityTransform->GetWorldTM();
                    m_viewParams.position = AZVec3ToLYVec3(transform.GetPosition());
                    m_viewParams.rotation = AZQuaternionToLYQuaternion(AZ::Quaternion::CreateFromTransform(transform));
                }
            }
        }

        ApplyFrameAdditiveAngles(m_viewParams.rotation);

        if (pLinkedTo)
        {
            pLinkedTo->PostUpdateView(m_viewParams);
        }

        const float fNearZ = gEnv->pGame->GetIGameFramework()->GetIViewSystem()->GetDefaultZNear();

        //see if the view have to use a custom near clipping plane
        const float nearPlane = (m_viewParams.nearplane >= CAMERA_MIN_NEAR) ? (m_viewParams.nearplane) : fNearZ;
        const float farPlane = (m_viewParams.farplane > 0.f) ? m_viewParams.farplane : gEnv->p3DEngine->GetMaxViewDistance();
        float fov = (m_viewParams.fov < 0.001f) ? DEFAULT_FOV : m_viewParams.fov;

        // [VR] specific
        // Modify FOV based on the HMD device configuration
        bool hmdActive = false;

        bool isRenderingToHMD = gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD();
        if (isRenderingToHMD)
        {
            const AZ::VR::HMDDeviceInfo* deviceInfo = nullptr;
            EBUS_EVENT_RESULT(deviceInfo, AZ::VR::HMDDeviceRequestBus, GetDeviceInfo);
            if (deviceInfo)
            {
                //Add 12 degrees to the FOV here used for culling.
                //It won't be used for rendering, just to make sure we don't cull
                //anything out incorrectly.
                //This value was decided based on experimentation with the HTC Vive
                //and works perfectly fine for the Oculus Rift
                const float fovCorrection = 12.0f;
                fov = deviceInfo->fovV + DEG2RAD(fovCorrection);
            }   
        }

        m_camera.SetFrustum(pSysCam->GetViewSurfaceX(), pSysCam->GetViewSurfaceZ(), fov, nearPlane, farPlane, pSysCam->GetPixelAspectRatio());

        //TODO: (14, 06, 2010, "the player view should always get updated, this due to the hud being visable, without shocking, in cutscenes - todo is to see if we can optimise this code");
        IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
        if (pActor)
        {
            CGameObjectPtr linkToObj = pActor->GetEntity()->GetComponent<CGameObject>();
            if (linkToObj && linkToObj != pLinkedTo)
            {
                linkToObj->PostUpdateView(m_viewParams);
            }
        }

        //apply shake & set the view matrix
        m_viewParams.rotation *= m_viewParams.currentShakeQuat;
        m_viewParams.rotation.NormalizeSafe();
        m_viewParams.position += m_viewParams.currentShakeShift;

        // Camera space Rendering calculations on Entity
        if (pLinkedTo)
        {
            IEntity* pLinkedToEntity = pLinkedTo->GetEntity();
            if (pLinkedToEntity)
            {
                const int slotIndex = 0;
                uint32 entityFlags = pLinkedToEntity->GetSlotFlags(slotIndex);
                if ((entityFlags & ENTITY_SLOT_RENDER_NEAREST) && !(entityFlags & ENTITY_SLOT_RENDER_NEAREST_NO_ATTACH_CAMERA))
                {
                    // Get camera pos relative to entity
                    const Vec3 cameraLocalPos = m_viewParams.position;

                    // Set entity's camera space position
                    const Vec3 cameraSpacePos(-cameraLocalPos * m_viewParams.rotation);
                    pLinkedToEntity->SetSlotCameraSpacePos(slotIndex, cameraSpacePos);

                    // Add world pos onto camera local pos
                    m_viewParams.position = pLinkedToEntity->GetWorldPos() + cameraLocalPos;
                }
            }
        }

        // Blending between cameras needs to happen after Camera space rendering calculations have been applied
        // so that the m_viewParams.position is in World Space again
        m_viewParams.UpdateBlending(frameTime);

        // [VR] specific
        // Add HMD's pose tracking on top of current camera pose
        // Each game-title can decide whether to keep this functionality here or (most likely)
        // move it somewhere else.

        Quat q = m_viewParams.rotation;
        Vec3 pos = m_viewParams.position;
        Vec3 p = Vec3(ZERO);

        // Uses the recorded tracking if time demo is on playback
        // Otherwise uses real tracking from device
        bool isRecorderPlaying = false;
        TimeDemoRecorderBus::BroadcastResult(isRecorderPlaying, &TimeDemoRecorderBus::Events::IsPlaying);
        if (isRecorderPlaying)
        {
            STimeDemoFrameRecord record;
            TimeDemoRecorderBus::Broadcast(&TimeDemoRecorderBus::Events::GetCurrentFrameRecord, record);

            p = q * record.hmdPositionOffset;
            q = q * record.hmdViewRotation;
        }
        else if (isRenderingToHMD)
        {
            if (pHmdReferencePoint && pHmdReferencePoint->GetIVal() == 1) // actor-centered HMD offset
            {
                const IEntity* pEnt = GetLinkedEntity();
                if (const IActor* clientActor = gEnv->pGame->GetIGameFramework()->GetClientActor())
                {
                    if (pEnt && clientActor->GetEntity() == pEnt)
                    {
                        q = pEnt->GetWorldRotation();
                        pos = pEnt->GetWorldPos();
                    }
                }
            }

            //This HMD tracking state is used JUST for use in the visibility system.
            //RT_SetStereoCamera in D3DRendPipeline will override this info before rendering with the absolute
            //latest tracking info.
            const AZ::VR::TrackingState* trackingState = nullptr;
            EBUS_EVENT_RESULT(trackingState, AZ::VR::HMDDeviceRequestBus, GetTrackingState);
            if (trackingState && trackingState->CheckStatusFlags(AZ::VR::HMDStatus_IsUsable))
            {
                p = q * AZVec3ToLYVec3(trackingState->pose.position);
                q = q * AZQuaternionToLYQuaternion(trackingState->pose.orientation);
            }
        }

        Matrix34 viewMtx(q);
        viewMtx.SetTranslation(pos + p);
        m_camera.SetMatrix(viewMtx);

        m_camera.SetEntityRotation(m_viewParams.rotation);
        m_camera.SetEntityPos(pos);
    }
    else
    {
        m_linkedTo = AZ::EntityId(0);
    }

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // Increment the frame update id for this camera. We do this here because 
    // the CView owns the camera and CSystem will use the active camera
    // for the call to RenderWorld.
    m_camera.IncrementFrameUpdateId();
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
}

//-----------------------------------------------------------------------
void CView::ApplyFrameAdditiveAngles(Quat& cameraOrientation)
{
    if ((m_frameAdditiveAngles.x != 0.f) || (m_frameAdditiveAngles.y != 0.f) || (m_frameAdditiveAngles.z != 0.f))
    {
        Ang3 cameraAngles(cameraOrientation);
        cameraAngles += m_frameAdditiveAngles;

        cameraOrientation.SetRotationXYZ(cameraAngles);

        m_frameAdditiveAngles.Set(0.0f, 0.0f, 0.0f);
    }
}

//------------------------------------------------------------------------
void CView::SetViewShake(Ang3 shakeAngle, Vec3 shakeShift, float duration, float frequency, float randomness, int shakeID, bool bFlipVec, bool bUpdateOnly, bool bGroundOnly)
{
    SShakeParams params;
    params.shakeAngle = shakeAngle;
    params.shakeShift = shakeShift;
    params.frequency = frequency;
    params.randomness = randomness;
    params.shakeID = shakeID;
    params.bFlipVec = bFlipVec;
    params.bUpdateOnly = bUpdateOnly;
    params.bGroundOnly = bGroundOnly;
    params.fadeInDuration = 0;                  //
    params.fadeOutDuration = duration;  // originally it was faded out from start. that is why the values are set this way here, to preserve compatibility.
    params.sustainDuration = 0;             //

    SetViewShakeEx(params);
}


//------------------------------------------------------------------------
void CView::SetViewShakeEx(const SShakeParams& params)
{
    float shakeMult = GetScale();
    if (shakeMult < 0.001f)
    {
        return;
    }

    int shakes(m_shakes.size());
    SShake* pSetShake(NULL);

    for (int i = 0; i < shakes; ++i)
    {
        SShake* pShake = &m_shakes[i];
        if (pShake->ID == params.shakeID)
        {
            pSetShake = pShake;
            break;
        }
    }

    if (!pSetShake)
    {
        m_shakes.push_back(SShake(params.shakeID));
        pSetShake = &m_shakes.back();
    }

    if (pSetShake)
    {
        // this can be set dynamically
        pSetShake->frequency = max(0.00001f, params.frequency);

        // the following are set on a 'new' shake as well
        if (params.bUpdateOnly == false)
        {
            pSetShake->amount = params.shakeAngle * shakeMult;
            pSetShake->amountVector = params.shakeShift * shakeMult;
            pSetShake->randomness = params.randomness;
            pSetShake->doFlip = params.bFlipVec;
            pSetShake->groundOnly = params.bGroundOnly;
            pSetShake->isSmooth = params.isSmooth;
            pSetShake->permanent = params.bPermanent;
            pSetShake->fadeInDuration = params.fadeInDuration;
            pSetShake->sustainDuration = params.sustainDuration;
            pSetShake->fadeOutDuration = params.fadeOutDuration;
            pSetShake->timeDone = 0;
            pSetShake->updating = true;
            pSetShake->interrupted = false;
            pSetShake->goalShake = Quat(ZERO);
            pSetShake->goalShakeSpeed = Quat(ZERO);
            pSetShake->goalShakeVector = Vec3(ZERO);
            pSetShake->goalShakeVectorSpeed = Vec3(ZERO);
            pSetShake->nextShake = 0.0f;
        }
    }
}

//------------------------------------------------------------------------
void CView::SetScale(const float scale)
{
    CRY_ASSERT_MESSAGE(scale == 1.0f || m_scale == 1.0f, "Attempting to CView::SetScale but has already been set!");
    m_scale = scale;
}

void CView::SetZoomedScale(const float scale)
{
    CRY_ASSERT_MESSAGE(scale == 1.0f || m_zoomedScale == 1.0f, "Attempting to CView::SetZoomedScale but has already been set!");
    m_zoomedScale = scale;
}

//------------------------------------------------------------------------
const float CView::GetScale()
{
    float shakeMult(pCamShakeMult->GetFVal());
    return m_scale * shakeMult * m_zoomedScale;
}

//------------------------------------------------------------------------
void CView::ProcessShaking(float frameTime)
{
    m_viewParams.currentShakeQuat.SetIdentity();
    m_viewParams.currentShakeShift.zero();
    m_viewParams.shakingRatio = 0;
    m_viewParams.groundOnly = false;

    int shakes(m_shakes.size());
    for (int i = 0; i < shakes; ++i)
    {
        ProcessShake(&m_shakes[i], frameTime);
    }
}

//------------------------------------------------------------------------
void CView::ProcessShake(SShake* pShake, float frameTime)
{
    if (!pShake->updating)
    {
        return;
    }

    pShake->timeDone += frameTime;

    if (pShake->isSmooth)
    {
        ProcessShakeSmooth(pShake, frameTime);
    }
    else
    {
        ProcessShakeNormal(pShake, frameTime);
    }
}

//------------------------------------------------------------------------
void CView::ProcessShakeNormal(SShake* pShake, float frameTime)
{
    float endSustain = pShake->fadeInDuration + pShake->sustainDuration;
    float totalDuration = endSustain + pShake->fadeOutDuration;

    bool finalDamping = (!pShake->permanent && pShake->timeDone > totalDuration) || (pShake->interrupted && pShake->ratio < 0.05f);

    if (finalDamping)
    {
        ProcessShakeNormal_FinalDamping(pShake, frameTime);
    }
    else
    {
        ProcessShakeNormal_CalcRatio(pShake, frameTime, endSustain);
        ProcessShakeNormal_DoShaking(pShake, frameTime);

        //for the global shaking ratio keep the biggest
        if (pShake->groundOnly)
        {
            m_viewParams.groundOnly = true;
        }
        m_viewParams.shakingRatio = max(m_viewParams.shakingRatio, pShake->ratio);
        m_viewParams.currentShakeQuat *= pShake->shakeQuat;
        m_viewParams.currentShakeShift += pShake->shakeVector;
    }
}

//////////////////////////////////////////////////////////////////////////
void CView::ProcessShakeSmooth(SShake* pShake, float frameTime)
{
    assert(pShake->timeDone >= 0);

    float endTimeFadeIn = pShake->fadeInDuration;
    float endTimeSustain = pShake->sustainDuration + endTimeFadeIn;
    float totalTime = endTimeSustain + pShake->fadeOutDuration;

    if (pShake->interrupted && endTimeFadeIn <= pShake->timeDone && pShake->timeDone < endTimeSustain)
    {
        pShake->timeDone = endTimeSustain;
    }

    float damping = 1.f;
    if (pShake->timeDone < endTimeFadeIn)
    {
        damping = pShake->timeDone / endTimeFadeIn;
    }
    else if (endTimeSustain < pShake->timeDone && pShake->timeDone < totalTime)
    {
        damping = (totalTime - pShake->timeDone) / (totalTime - endTimeSustain);
    }
    else if (totalTime <= pShake->timeDone)
    {
        pShake->shakeQuat.SetIdentity();
        pShake->shakeVector.zero();
        pShake->ratio = 0.0f;
        pShake->nextShake = 0.0f;
        pShake->flip = false;
        pShake->updating = false;
        return;
    }

    ProcessShakeSmooth_DoShaking(pShake, frameTime);

    if (pShake->groundOnly)
    {
        m_viewParams.groundOnly = true;
    }
    pShake->ratio = (3.f - 2.f * damping) * damping * damping; // smooth ration change
    m_viewParams.shakingRatio = max(m_viewParams.shakingRatio, pShake->ratio);
    m_viewParams.currentShakeQuat *= Quat::CreateSlerp(IDENTITY, pShake->shakeQuat, pShake->ratio);
    m_viewParams.currentShakeShift += Vec3::CreateLerp(ZERO, pShake->shakeVector, pShake->ratio);
}

//////////////////////////////////////////////////////////////////////////
void CView::GetRandomQuat(Quat& quat, SShake* pShake)
{
    quat.SetRotationXYZ(pShake->amount);
    float randomAmt(pShake->randomness);
    float len(fabs(pShake->amount.x) + fabs(pShake->amount.y) + fabs(pShake->amount.z));
    len /= 3.f;
    float r = len * randomAmt;
    quat *= Quat::CreateRotationXYZ(Ang3(cry_random(-r, r), cry_random(-r, r), cry_random(-r, r)));
}

//////////////////////////////////////////////////////////////////////////
void CView::GetRandomVector(Vec3& vec, SShake* pShake)
{
    vec = pShake->amountVector;
    float randomAmt(pShake->randomness);
    float len = fabs(pShake->amountVector.x) + fabs(pShake->amountVector.y) + fabs(pShake->amountVector.z);
    len /= 3.f;
    float r = len * randomAmt;
    vec += Vec3(cry_random(-r, r), cry_random(-r, r), cry_random(-r, r));
}

//////////////////////////////////////////////////////////////////////////
void CView::CubeInterpolateQuat(float t, SShake* pShake)
{
    Quat p0 = pShake->startShake;
    Quat p1 = pShake->goalShake;
    Quat v0 = pShake->startShakeSpeed * 0.5f;
    Quat v1 = pShake->goalShakeSpeed * 0.5f;

    pShake->shakeQuat = (((p0 *  2.f + p1 * -2.f + v0 + v1) * t
        + (p0 * -3.f + p1 *  3.f + v0 * -2.f - v1)) * t
        + (v0)) * t
        + p0;

    pShake->shakeQuat.Normalize();
}

//////////////////////////////////////////////////////////////////////////
void CView::CubeInterpolateVector(float t, SShake* pShake)
{
    Vec3 p0 = pShake->startShakeVector;
    Vec3 p1 = pShake->goalShakeVector;
    Vec3 v0 = pShake->startShakeVectorSpeed * 0.8f;
    Vec3 v1 = pShake->goalShakeVectorSpeed * 0.8f;

    pShake->shakeVector = (((p0 *  2.f + p1 * -2.f + v0 + v1) * t
        + (p0 * -3.f + p1 *  3.f + v0 * -2.f - v1)) * t
        + (v0)) * t
        + p0;
}

//////////////////////////////////////////////////////////////////////////
void CView::ProcessShakeSmooth_DoShaking(SShake* pShake, float frameTime)
{
    if (pShake->nextShake <= 0.0f)
    {
        pShake->nextShake = pShake->frequency;

        pShake->startShake              = pShake->goalShake;
        pShake->startShakeSpeed         = pShake->goalShakeSpeed;
        pShake->startShakeVector        = pShake->goalShakeVector;
        pShake->startShakeVectorSpeed   = pShake->goalShakeVectorSpeed;

        GetRandomQuat(pShake->goalShake, pShake);
        GetRandomQuat(pShake->goalShakeSpeed, pShake);
        GetRandomVector(pShake->goalShakeVector, pShake);
        GetRandomVector(pShake->goalShakeVectorSpeed, pShake);

        if (pShake->flip)
        {
            pShake->goalShake.Invert();
            pShake->goalShakeSpeed.Invert();
            pShake->goalShakeVector = -pShake->goalShakeVector;
            pShake->goalShakeVectorSpeed = -pShake->goalShakeVectorSpeed;
        }

        if (pShake->doFlip)
        {
            pShake->flip = !pShake->flip;
        }
    }

    pShake->nextShake -= frameTime;

    float t = (pShake->frequency - pShake->nextShake) / pShake->frequency;
    CubeInterpolateQuat(t, pShake);
    CubeInterpolateVector(t, pShake);
}

//////////////////////////////////////////////////////////////////////////
void CView::ProcessShakeNormal_FinalDamping(SShake* pShake, float frameTime)
{
    pShake->shakeQuat = Quat::CreateSlerp(pShake->shakeQuat, IDENTITY, frameTime * 5.0f);
    m_viewParams.currentShakeQuat *= pShake->shakeQuat;

    pShake->shakeVector = Vec3::CreateLerp(pShake->shakeVector, ZERO, frameTime * 5.0f);
    m_viewParams.currentShakeShift += pShake->shakeVector;

    float svlen2(pShake->shakeVector.len2());
    bool quatIsIdentity(Quat::IsEquivalent(IDENTITY, pShake->shakeQuat, 0.0001f));

    if (quatIsIdentity && svlen2 < 0.01f)
    {
        pShake->shakeQuat.SetIdentity();
        pShake->shakeVector.zero();

        pShake->ratio = 0.0f;
        pShake->nextShake = 0.0f;
        pShake->flip = false;

        pShake->updating = false;
    }
}


// "ratio" is the amplitude of the shaking
void CView::ProcessShakeNormal_CalcRatio(SShake* pShake, float frameTime, float endSustain)
{
    const float FADEOUT_TIME_WHEN_INTERRUPTED = 0.5f;

    if (pShake->interrupted)
    {
        pShake->ratio = max(0.f, pShake->ratio - (frameTime / FADEOUT_TIME_WHEN_INTERRUPTED));    // fadeout after interrupted
    }
    else
        if (pShake->timeDone >= endSustain && pShake->fadeOutDuration > 0)
        {
            float timeFading = pShake->timeDone - endSustain;
            pShake->ratio = clamp_tpl(1.f - timeFading / pShake->fadeOutDuration, 0.f, 1.f);   // fadeOut
        }
        else
            if (pShake->timeDone >= pShake->fadeInDuration)
            {
                pShake->ratio = 1.f; // sustain
            }
            else
            {
                pShake->ratio = min(1.f, pShake->timeDone / pShake->fadeInDuration);   // fadeIn
            }

    if (pShake->permanent && pShake->timeDone >= pShake->fadeInDuration && !pShake->interrupted)
    {
        pShake->ratio = 1.f; // permanent standing
    }
}

//////////////////////////////////////////////////////////////////////////
void CView::ProcessShakeNormal_DoShaking(SShake* pShake, float frameTime)
{
    float t;
    if (pShake->nextShake <= 0.0f)
    {
        //angular
        pShake->goalShake.SetRotationXYZ(pShake->amount);
        if (pShake->flip)
        {
            pShake->goalShake.Invert();
        }

        //translational
        pShake->goalShakeVector = pShake->amountVector;
        if (pShake->flip)
        {
            pShake->goalShakeVector = -pShake->goalShakeVector;
        }

        if (pShake->doFlip)
        {
            pShake->flip = !pShake->flip;
        }

        //randomize it a little
        float randomAmt(pShake->randomness);
        float len(fabs(pShake->amount.x) + fabs(pShake->amount.y) + fabs(pShake->amount.z));
        len /= 3.0f;
        float r = len * randomAmt;
        pShake->goalShake *= Quat::CreateRotationXYZ(Ang3(cry_random(-r, r), cry_random(-r, r), cry_random(-r, r)));

        //translational randomization
        len = fabs(pShake->amountVector.x) + fabs(pShake->amountVector.y) + fabs(pShake->amountVector.z);
        len /= 3.0f;
        r = len * randomAmt;
        pShake->goalShakeVector += Vec3(cry_random(-r, r), cry_random(-r, r), cry_random(-r, r));

        //damp & bounce it in a non linear fashion
        t = 1.0f - (pShake->ratio * pShake->ratio);
        pShake->goalShake = Quat::CreateSlerp(pShake->goalShake, IDENTITY, t);
        pShake->goalShakeVector = Vec3::CreateLerp(pShake->goalShakeVector, ZERO, t);

        pShake->nextShake = pShake->frequency;
    }

    pShake->nextShake = max(0.0f, pShake->nextShake - frameTime);

    t = min(1.0f, frameTime * (1.0f / pShake->frequency));
    pShake->shakeQuat = Quat::CreateSlerp(pShake->shakeQuat, pShake->goalShake, t);
    pShake->shakeQuat.Normalize();
    pShake->shakeVector = Vec3::CreateLerp(pShake->shakeVector, pShake->goalShakeVector, t);
}


//------------------------------------------------------------------------
void CView::StopShake(int shakeID)
{
    uint32 num = m_shakes.size();
    for (uint32 i = 0; i < num; ++i)
    {
        if (m_shakes[i].ID == shakeID && m_shakes[i].updating)
        {
            m_shakes[i].interrupted = true;
        }
    }
}


//------------------------------------------------------------------------
void CView::ResetShaking()
{
    // disable shakes
    std::vector<SShake>::iterator iter = m_shakes.begin();
    std::vector<SShake>::iterator iterEnd = m_shakes.end();
    while (iter != iterEnd)
    {
        SShake& shake = *iter;
        shake.updating = false;
        shake.timeDone = 0;
        ++iter;
    }
}

//------------------------------------------------------------------------
void CView::LinkTo(AZ::Entity* follow)
{
    CRY_ASSERT(follow);
    m_azEntity = follow;
    m_linkedTo = follow->GetId();
    m_viewParams.targetPos = Vec3();// This should be quickly overwritten by the camera's acutal position from its matrix
}

//------------------------------------------------------------------------
void CView::LinkTo(IGameObject* follow)
{
    CRY_ASSERT(follow);

    m_linkedTo = AZ::EntityId(follow->GetEntityId());
    m_viewParams.targetPos = follow->GetEntity()->GetWorldPos();
}

//------------------------------------------------------------------------
void CView::LinkTo(IEntity* follow)
{
    CRY_ASSERT(follow);
    m_linkedTo = AZ::EntityId(follow->GetId());
    m_viewParams.targetPos = follow->GetWorldPos();
}

//------------------------------------------------------------------------
void CView::Unlink()
{
    m_azEntity = nullptr;
    m_linkedTo.SetInvalid();
    m_viewParams.targetPos = Vec3();
}

//------------------------------------------------------------------------
void CView::SetFrameAdditiveCameraAngles(const Ang3& addFrameAngles)
{
    m_frameAdditiveAngles = addFrameAngles;
}

//------------------------------------------------------------------------
CGameObjectPtr CView::GetLinkedGameObject()
{
    //We should not be returning a legacy CGameObjectPtr if this object isn't linked to a legacy entity
    if (!IsLegacyEntityId(m_linkedTo))
    {
        return NULL;
    }

    EntityId legacyId = GetLegacyEntityId(m_linkedTo);
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(legacyId);

    if (!pEntity)
    {
        return NULL;
    }

    return pEntity->GetComponent<CGameObject>();
}

//------------------------------------------------------------------------
IEntity* CView::GetLinkedEntity()
{
    //We should not be returning a legacy IEntity* if this object isn't linked to a legacy entity
    if (!IsLegacyEntityId(m_linkedTo))
    {
        return NULL;
    }

    EntityId legacyId = GetLegacyEntityId(m_linkedTo);

    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(legacyId);
    return pEntity;
}

void CView::GetMemoryUsage(ICrySizer* s) const
{
    s->AddObject(this, sizeof(*this));
    s->AddObject(m_shakes);
}

void CView::Serialize(TSerialize ser)
{
    if (ser.IsReading())
    {
        ResetShaking();
    }
}

void CView::PostSerialize()
{
    CreateAudioListener();
}

//////////////////////////////////////////////////////////////////////////
void CView::OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_DONE:
    {
        // In case something destroys our listener entity before we had the chance to remove it.
        if ((m_pAudioListener != nullptr) && (pEntity->GetId() == m_pAudioListener->GetId()))
        {
            gEnv->pEntitySystem->RemoveEntityEventListener(m_pAudioListener->GetId(), ENTITY_EVENT_DONE, this);
            m_pAudioListener = nullptr;
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
void CView::CreateAudioListener()
{
    if (!m_pAudioListener)
    {
        SEntitySpawnParams oEntitySpawnParams;
        oEntitySpawnParams.sName = "AudioListener";
        oEntitySpawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AudioListener");
        oEntitySpawnParams.nFlags = ENTITY_FLAG_CLIENT_ONLY;
        m_pAudioListener = gEnv->pEntitySystem->SpawnEntity(oEntitySpawnParams, true);

        if (m_pAudioListener)
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
            CryFatalError("<Audio>: Audio listener creation failed in CView::CreateAudioListener!");
        }
    }
    else
    {
        m_pAudioListener->SetFlagsExtended(m_pAudioListener->GetFlagsExtended() | ENTITY_FLAG_EXTENDED_AUDIO_LISTENER);
        m_pAudioListener->InvalidateTM(ENTITY_XFORM_POS);
    }
}

//////////////////////////////////////////////////////////////////////////
void CView::UpdateAudioListener(Matrix34 const& rMatrix)
{
    if (m_pAudioListener)
    {
        Matrix34 newMatrix(rMatrix);

        // Audio Listener repositioning BEGIN - PR1
        IEntity* linkedEntity = GetLinkedEntity();
        bool shouldRepositionListener = !gEnv->IsEditor() || gEnv->IsEditorGameMode();
        if (linkedEntity && shouldRepositionListener)
        {
            // This code repositions the audio listener by first offsetting the view target's position in Z (up).
            // Then take the vector from view (camera) to the offset view target's position and put the listener somewhere
            // along that vector (given by a percentage, where 0.0 would remain at the view position and 1.0 would
            // be at the offset view target position).
            float audioListenerTargetZOffset = 0.f;
            float audioListenerTargetPercentage = 0.f;
            if (gEnv->pConsole)
            {
                // should cache these and update only when there are changes...
                ICVar* audioListenerTranslationZOffset = gEnv->pConsole->GetCVar("s_AudioListenerTranslationZOffset");
                ICVar* audioListenerTranslationPercentage = gEnv->pConsole->GetCVar("s_AudioListenerTranslationPercentage");

                if (audioListenerTargetZOffset)
                {
                    audioListenerTargetZOffset = audioListenerTranslationZOffset->GetFVal();
                }
                if (audioListenerTranslationPercentage)
                {
                    audioListenerTargetPercentage = audioListenerTranslationPercentage->GetFVal();
                }

                // clamp for now, but there may be scenarios where you'd want the listener to go
                // "behind" the camera or "in front" of the target.
                audioListenerTargetPercentage = AZ::GetClamp(audioListenerTargetPercentage, 0.f, 1.f);
            }

            Vec3 zOffset(0.f, 0.f, audioListenerTargetZOffset);
            Vec3 targetDirection = linkedEntity->GetWorldPos() - rMatrix.GetColumn3() + zOffset;
            targetDirection *= audioListenerTargetPercentage;
            newMatrix.SetColumn(3, rMatrix.GetColumn3() + targetDirection);
        }
        // Audio Listener repositioning END

        m_pAudioListener->SetWorldTM(newMatrix);
    }
}

//////////////////////////////////////////////////////////////////////////
void CView::SetActive(bool const bActive)
{
    if (bActive)
    {
        // Make sure we have a valid audio listener entity on an active view!
        CreateAudioListener();
    }
    else if (m_pAudioListener && (m_pAudioListener->GetFlags() & ENTITY_FLAG_TRIGGER_AREAS) != 0)
    {
        gEnv->pEntitySystem->GetAreaManager()->ExitAllAreas(m_pAudioListener);
        m_pAudioListener->SetFlagsExtended(m_pAudioListener->GetFlagsExtended() & ~ENTITY_FLAG_EXTENDED_AUDIO_LISTENER);
    }
}


