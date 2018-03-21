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

#include "Maestro_precompiled.h"
#include "MathConversion.h"
#include "AnimCameraNode.h"
#include "SelectTrack.h"
#include "CompoundSplineTrack.h"
#include "PNoise3.h"
#include "AnimSplineTrack.h"
#include "IPostEffectGroup.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

#include <ISystem.h>
#include <I3DEngine.h>
#include <IConsole.h>
#include <IEntitySystem.h>
#include <ITimer.h>

CAnimCameraNode* CAnimCameraNode::m_pLastFrameActiveCameraNode;

#define s_nodeParamsInitialized s_nodeParamsInitializedCam
#define s_nodeParams s_nodeParamsCam
#define AddSupportedParam AddSupportedParamCam

//////////////////////////////////////////////////////////////////////////
namespace
{
    bool s_nodeParamsInitialized = false;
    std::vector<CAnimNode::SParamInfo> s_nodeParams;

    void AddSupportedParam(const char* sName, AnimParamType paramId, AnimValueType valueType)
    {
        CAnimNode::SParamInfo param;
        param.name = sName;
        param.paramType = paramId;
        param.valueType = valueType;
        s_nodeParams.push_back(param);
    }
};

CAnimCameraNode::CAnimCameraNode(const int id)
    : CAnimEntityNode(id, AnimNodeType::Camera)
    , m_fFOV(60.0f)
    , m_fDOF(ZERO)
    , m_fNearZ(DEFAULT_NEAR)
    , m_cv_r_PostProcessEffects(NULL)
    , m_bJustActivated(false)
    , m_cameraShakeSeedValue(0)
{
    SetSkipInterpolatedCameraNode(false);
    CAnimCameraNode::Initialize();
}

//////////////////////////////////////////////////////////////////////////
CAnimCameraNode::~CAnimCameraNode()
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::Initialize()
{
    if (!s_nodeParamsInitialized)
    {
        s_nodeParamsInitialized = true;
        s_nodeParams.reserve(8);
        AddSupportedParam("Position", AnimParamType::Position, AnimValueType::Vector);
        AddSupportedParam("Rotation", AnimParamType::Rotation, AnimValueType::Quat);
        AddSupportedParam("Fov", AnimParamType::FOV, AnimValueType::Float);
        AddSupportedParam("Event", AnimParamType::Event, AnimValueType::Unknown);
        AddSupportedParam("Shake", AnimParamType::ShakeMultiplier, AnimValueType::Vector4);
        AddSupportedParam("Depth of Field", AnimParamType::DepthOfField, AnimValueType::Vector);
        AddSupportedParam("Near Z", AnimParamType::NearZ, AnimValueType::Float);
        AddSupportedParam("Shutter Speed", AnimParamType::ShutterSpeed, AnimValueType::Float);
    }
}

//////////////////////////////////////////////////////////////////////////
unsigned int CAnimCameraNode::GetParamCount() const
{
    return s_nodeParams.size();
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CAnimCameraNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex >= 0 && nIndex < (int)s_nodeParams.size())
    {
        return s_nodeParams[nIndex].paramType;
    }

    return AnimParamType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (unsigned int i = 0; i < s_nodeParams.size(); i++)
    {
        if (s_nodeParams[i].paramType == paramId)
        {
            info = s_nodeParams[i];
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::CreateDefaultTracks()
{
    CreateTrack(AnimParamType::Position);
    CreateTrack(AnimParamType::Rotation);
    CreateTrack(AnimParamType::FOV);
};

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::Animate(SAnimContext& ec)
{
    CAnimEntityNode::Animate(ec);

    // If we are in editing mode we only apply post effects if the camera is active in the viewport
    const bool bApplyPostEffects = !gEnv->IsEditing() || ec.m_activeCameraEntity == GetEntityId();

    //------------------------------------------------------------------------------
    ///Depth of field track
    IEntity* pEntity = GetEntity();

    IAnimTrack* pDOFTrack = GetTrackForParameter(AnimParamType::DepthOfField);

    Vec3 dof = Vec3(ZERO);
    if (pDOFTrack)
    {
        pDOFTrack->GetValue(ec.time, dof);
    }

    IAnimTrack* pFOVTrack = GetTrackForParameter(AnimParamType::FOV);
    float fov = m_fFOV;

    bool bFOVTrackEnabled = false;
    if (pFOVTrack && !(pFOVTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
    {
        bFOVTrackEnabled = true;
    }

    if (bFOVTrackEnabled && !IsSkipInterpolatedCameraNodeEnabled())
    {
        pFOVTrack->GetValue(ec.time, fov);
    }

    IAnimTrack* pNearZTrack = GetTrackForParameter(AnimParamType::NearZ);
    float nearZ = m_fNearZ;

    bool bNearZTrackEnabled = false;
    if (pNearZTrack && !(pNearZTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
    {
        bNearZTrackEnabled = true;
    }

    if (bNearZTrackEnabled)
    {
        pNearZTrack->GetValue(ec.time, nearZ);
    }

    // is this camera active ?
    if (bApplyPostEffects && (GetLegacyEntityId(gEnv->pMovieSystem->GetCameraParams().cameraEntityId) == GetEntityId()))
    {
        if (pEntity)
        {
            pEntity->SetFlags(pEntity->GetFlags() | ENTITY_FLAG_TRIGGER_AREAS);
        }

        SCameraParams CamParams = gEnv->pMovieSystem->GetCameraParams();

        if (bFOVTrackEnabled)
        {
            CamParams.fFOV = DEG2RAD(fov);
        }

        if (bNearZTrackEnabled)
        {
            CamParams.fNearZ = nearZ;
        }

        if ((bNearZTrackEnabled || bFOVTrackEnabled) && !IsSkipInterpolatedCameraNodeEnabled())
        {
            gEnv->pMovieSystem->SetCameraParams(CamParams);
        }

        bool bDOFTrackEnabled = false;
        if (pDOFTrack && !(pDOFTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            bDOFTrackEnabled = true;
        }

        // If there is a "Depth of Field Track" with current camera
        if (bDOFTrackEnabled)
        {
            // Active Depth of field
            gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Dof_Active", true);
            // Set parameters
            gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Dof_FocusDistance", dof.x);
            gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Dof_FocusRange", dof.y);
            gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Dof_BlurAmount", dof.z);
        }
        else
        {
            // If a camera doesn't have a DoF track, it means it won't use the DoF processing.
            gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Dof_Active", false);
        }

        if (m_pLastFrameActiveCameraNode != this)
        {
            gEnv->pRenderer->EF_DisableTemporalEffects();
            static_cast<CMovieSystem*>(GetMovieSystem())->OnCameraCut();
        }

        IAnimTrack* pShutterSpeedTrack = GetTrackForParameter(AnimParamType::ShutterSpeed);
        if (pShutterSpeedTrack)
        {
            float shutterSpeed;
            pShutterSpeedTrack->GetValue(ec.time, shutterSpeed);
            gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("MotionBlur_ShutterSpeed", shutterSpeed);
        }

        m_pLastFrameActiveCameraNode = this;
    }
    else
    {
        if (pEntity)
        {
            pEntity->ClearFlags(ENTITY_FLAG_TRIGGER_AREAS);
        }
    }

    bool bNodeAnimated = false;

    if (m_bJustActivated)
    {
        bNodeAnimated = true;
        m_bJustActivated = false;
    }

    if (fov != m_fFOV)
    {
        m_fFOV = fov;
        bNodeAnimated = true;
    }

    if (dof != m_fDOF)
    {
        m_fDOF = dof;
        bNodeAnimated = true;
    }

    if (nearZ != m_fNearZ)
    {
        m_fNearZ = nearZ;
        bNodeAnimated = true;
    }

    if (pEntity)
    {
        Quat rotation = pEntity->GetRotation();
        if (GetShakeRotation(ec.time, rotation))
        {
            m_rotate = rotation;

            if (!IsSkipInterpolatedCameraNodeEnabled())
            {
                pEntity->SetRotation(rotation, ENTITY_XFORM_TRACKVIEW);
            }

            bNodeAnimated = true;
        }
    }

    if (bNodeAnimated && m_pOwner && !IsSkipInterpolatedCameraNodeEnabled())
    {
        m_bIgnoreSetParam = true;
        m_pOwner->OnNodeAnimated(this);
        m_bIgnoreSetParam = false;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetShakeRotation(const float& time, Quat& rotation)
{
    Vec4 shakeMult = Vec4(m_shakeParam[0].amplitudeMult, m_shakeParam[1].amplitudeMult,
            m_shakeParam[0].frequencyMult, m_shakeParam[1].frequencyMult);
    CCompoundSplineTrack* pShakeMultTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(AnimParamType::ShakeMultiplier));
    C2DSplineTrack* pFreqTrack[SHAKE_COUNT];
    memset(pFreqTrack, 0, sizeof(pFreqTrack));

    bool bShakeMultTrackEnabled = false;
    if (pShakeMultTrack && !(pShakeMultTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
    {
        bShakeMultTrackEnabled = true;
    }

    if (bShakeMultTrackEnabled)
    {
        pShakeMultTrack->GetValue(time, shakeMult);
        pFreqTrack[0] = static_cast<C2DSplineTrack*>(pShakeMultTrack->GetSubTrack(2)); // Frequency A is in z component.
        pFreqTrack[1] = static_cast<C2DSplineTrack*>(pShakeMultTrack->GetSubTrack(3)); // Frequency B is in w component.
    }

    if (bShakeMultTrackEnabled && shakeMult != Vec4(m_shakeParam[0].amplitudeMult, m_shakeParam[1].amplitudeMult,
            m_shakeParam[0].frequencyMult, m_shakeParam[1].frequencyMult))
    {
        m_shakeParam[0].amplitudeMult = shakeMult.x;
        m_shakeParam[1].amplitudeMult = shakeMult.y;
        m_shakeParam[0].frequencyMult = shakeMult.z;
        m_shakeParam[1].frequencyMult = shakeMult.w;
    }

    bool shakeOn[SHAKE_COUNT];
    shakeOn[0] = m_shakeParam[0].amplitudeMult != 0;
    shakeOn[1] = m_shakeParam[1].amplitudeMult != 0;

    IEntity* pEntity = GetEntity();

    if (pEntity != NULL && bShakeMultTrackEnabled && (shakeOn[0] || shakeOn[1]))
    {
        if (GetTrackForParameter(AnimParamType::Rotation))
        {
            IAnimTrack* rotTrack = GetTrackForParameter(AnimParamType::Rotation);
            rotTrack->GetValue(time, rotation);
        }

        Ang3 angles = Ang3::GetAnglesXYZ(Matrix33(rotation)) * 180.0f / gf_PI;

        for (int i = 0; i < SHAKE_COUNT; ++i)
        {
            if (shakeOn[i])
            {
                angles = m_shakeParam[i].ApplyCameraShake(m_pNoiseGen, time, angles, pFreqTrack[i], GetEntityId(), i, m_cameraShakeSeedValue);
            }
        }

        rotation.SetRotationXYZ(angles * gf_PI / 180.0f);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::OnReset()
{
    CAnimEntityNode::OnReset();
    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Dof_Active", 0.f);
    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("MotionBlur_ShutterSpeed", -1.0f);
}

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::OnStop()
{
    m_pLastFrameActiveCameraNode = NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::SetParamValue(float time, CAnimParamType param, float value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    if (param == AnimParamType::FOV)
    {
        // Set default value.
        m_fFOV = value;
    }
    else if (param == AnimParamType::NearZ)
    {
        m_fNearZ = value;
    }

    return CAnimEntityNode::SetParamValue(time, param, value);
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::SetParamValue(float time, CAnimParamType param, const Vec3& value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    if (param == AnimParamType::ShakeAmplitudeA)
    {
        m_shakeParam[0].amplitude = value;
    }
    else if (param == AnimParamType::ShakeAmplitudeB)
    {
        m_shakeParam[1].amplitude = value;
    }
    else if (param == AnimParamType::ShakeFrequencyA)
    {
        m_shakeParam[0].frequency = value;
    }
    else if (param == AnimParamType::ShakeFrequencyB)
    {
        m_shakeParam[1].frequency = value;
    }

    return CAnimEntityNode::SetParamValue(time, param, value);
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::SetParamValue(float time, CAnimParamType param, const Vec4& value)
{
    if (m_bIgnoreSetParam)
    {
        return true;
    }

    if (param == AnimParamType::ShakeMultiplier)
    {
        m_shakeParam[0].amplitudeMult = value.x;
        m_shakeParam[1].amplitudeMult = value.y;
        m_shakeParam[0].frequencyMult = value.z;
        m_shakeParam[1].frequencyMult = value.w;
    }
    else if (param == AnimParamType::ShakeNoise)
    {
        m_shakeParam[0].noiseAmpMult = value.x;
        m_shakeParam[1].noiseAmpMult = value.y;
        m_shakeParam[0].noiseFreqMult = value.z;
        m_shakeParam[1].noiseFreqMult = value.w;
    }
    else if (param == AnimParamType::ShakeWorking)
    {
        m_shakeParam[0].timeOffset = value.x;
        m_shakeParam[1].timeOffset = value.y;
    }

    return CAnimEntityNode::SetParamValue(time, param, value);
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamValue(float time, CAnimParamType param, float& value)
{
    if (param == AnimParamType::FOV)
    {
        value = m_fFOV;
        return true;
    }
    else if (param == AnimParamType::NearZ)
    {
        value = m_fNearZ;
        return true;
    }

    if (CAnimEntityNode::GetParamValue(time, param, value))
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamValue(float time, CAnimParamType param, Vec3& value)
{
    if (param == AnimParamType::ShakeAmplitudeA)
    {
        value = m_shakeParam[0].amplitude;
        return true;
    }
    else if (param == AnimParamType::ShakeAmplitudeB)
    {
        value = m_shakeParam[1].amplitude;
        return true;
    }
    else if (param == AnimParamType::ShakeFrequencyA)
    {
        value = m_shakeParam[0].frequency;
        return true;
    }
    else if (param == AnimParamType::ShakeFrequencyB)
    {
        value = m_shakeParam[1].frequency;
        return true;
    }

    if (CAnimEntityNode::GetParamValue(time, param, value))
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamValue(float time, CAnimParamType param, Vec4& value)
{
    if (param == AnimParamType::ShakeMultiplier)
    {
        value = Vec4(m_shakeParam[0].amplitudeMult, m_shakeParam[1].amplitudeMult,
                m_shakeParam[0].frequencyMult, m_shakeParam[1].frequencyMult);
        return true;
    }
    else if (param == AnimParamType::ShakeNoise)
    {
        value = Vec4(m_shakeParam[0].noiseAmpMult, m_shakeParam[1].noiseAmpMult,
                m_shakeParam[0].noiseFreqMult, m_shakeParam[1].noiseFreqMult);
        return true;
    }
    else if (param == AnimParamType::ShakeWorking)
    {
        value = Vec4(m_shakeParam[0].timeOffset, m_shakeParam[1].timeOffset,
                0.0f, 0.0f);
        return true;
    }

    if (CAnimEntityNode::GetParamValue(time, param, value))
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType)
{
    if (paramType.GetType() == AnimParamType::FOV)
    {
        if (pTrack)
        {
            pTrack->SetValue(0, m_fFOV, true);
        }
    }
    else if (paramType.GetType() == AnimParamType::NearZ)
    {
        if (pTrack)
        {
            pTrack->SetValue(0, m_fNearZ, true);
        }
    }
    else if (paramType.GetType() == AnimParamType::ShakeMultiplier)
    {
        if (pTrack)
        {
            static_cast<CCompoundSplineTrack*>(pTrack)->SetValue(0,
                Vec4(m_shakeParam[0].amplitudeMult, m_shakeParam[1].amplitudeMult,
                    m_shakeParam[0].frequencyMult, m_shakeParam[1].frequencyMult),
                true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CAnimCameraNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    CAnimEntityNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    if (bLoading)
    {
        xmlNode->getAttr("FOV", m_fFOV);
        xmlNode->getAttr("NearZ", m_fNearZ);
        xmlNode->getAttr("AmplitudeA", m_shakeParam[0].amplitude);
        xmlNode->getAttr("AmplitudeAMult", m_shakeParam[0].amplitudeMult);
        xmlNode->getAttr("FrequencyA", m_shakeParam[0].frequency);
        xmlNode->getAttr("FrequencyAMult", m_shakeParam[0].frequencyMult);
        xmlNode->getAttr("NoiseAAmpMult", m_shakeParam[0].noiseAmpMult);
        xmlNode->getAttr("NoiseAFreqMult", m_shakeParam[0].noiseFreqMult);
        xmlNode->getAttr("TimeOffsetA", m_shakeParam[0].timeOffset);
        xmlNode->getAttr("AmplitudeB", m_shakeParam[1].amplitude);
        xmlNode->getAttr("AmplitudeBMult", m_shakeParam[1].amplitudeMult);
        xmlNode->getAttr("FrequencyB", m_shakeParam[1].frequency);
        xmlNode->getAttr("FrequencyBMult", m_shakeParam[1].frequencyMult);
        xmlNode->getAttr("NoiseBAmpMult", m_shakeParam[1].noiseAmpMult);
        xmlNode->getAttr("NoiseBFreqMult", m_shakeParam[1].noiseFreqMult);
        xmlNode->getAttr("TimeOffsetB", m_shakeParam[1].timeOffset);
        xmlNode->getAttr("ShakeSeed", m_cameraShakeSeedValue);
    }
    else
    {
        xmlNode->setAttr("FOV", m_fFOV);
        xmlNode->setAttr("NearZ", m_fNearZ);
        xmlNode->setAttr("AmplitudeA", m_shakeParam[0].amplitude);
        xmlNode->setAttr("AmplitudeAMult", m_shakeParam[0].amplitudeMult);
        xmlNode->setAttr("FrequencyA", m_shakeParam[0].frequency);
        xmlNode->setAttr("FrequencyAMult", m_shakeParam[0].frequencyMult);
        xmlNode->setAttr("NoiseAAmpMult", m_shakeParam[0].noiseAmpMult);
        xmlNode->setAttr("NoiseAFreqMult", m_shakeParam[0].noiseFreqMult);
        xmlNode->setAttr("TimeOffsetA", m_shakeParam[0].timeOffset);
        xmlNode->setAttr("AmplitudeB", m_shakeParam[1].amplitude);
        xmlNode->setAttr("AmplitudeBMult", m_shakeParam[1].amplitudeMult);
        xmlNode->setAttr("FrequencyB", m_shakeParam[1].frequency);
        xmlNode->setAttr("FrequencyBMult", m_shakeParam[1].frequencyMult);
        xmlNode->setAttr("NoiseBAmpMult", m_shakeParam[1].noiseAmpMult);
        xmlNode->setAttr("NoiseBFreqMult", m_shakeParam[1].noiseFreqMult);
        xmlNode->setAttr("TimeOffsetB", m_shakeParam[1].timeOffset);
        xmlNode->setAttr("ShakeSeed", m_cameraShakeSeedValue);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::Activate(bool bActivate)
{
    CAnimEntityNode::Activate(bActivate);

    if (bActivate)
    {
        m_bJustActivated = true;
    }
};

Ang3 CAnimCameraNode::ShakeParam::ApplyCameraShake(CPNoise3& noiseGen, float time, Ang3 angles, C2DSplineTrack* pFreqTrack, EntityId camEntityId, int shakeIndex, const uint shakeSeed)
{
    Ang3 rotation;
    Ang3 rotationNoise;

    float noiseAmpMultiplier = this->amplitudeMult * this->noiseAmpMult;

    float t = this->timeOffset;

    this->phase = Vec3((t + 15.0f) * this->frequency.x,
            (t + 55.1f) * this->frequency.y,
            (t + 101.2f) * this->frequency.z);
    this->phaseNoise = Vec3((t + 70.0f) * this->frequency.x * this->noiseFreqMult,
            (t + 10.0f) * this->frequency.y * this->noiseFreqMult,
            (t + 30.5f) * this->frequency.z * this->noiseFreqMult);

    float phaseDelta = static_cast<spline::TrackSplineInterpolator<Vec2>*>(pFreqTrack->GetSpline())->Integrate(time);

#if !defined(_RELEASE)
    if (CMovieSystem::m_mov_debugCamShake)
    {
        ShowCameraShakeDebug(camEntityId, shakeIndex, phaseDelta);
    }
#endif

    this->phase += this->frequency * phaseDelta;
    this->phaseNoise += this->frequency * this->noiseFreqMult * phaseDelta;

    rotation.x = noiseGen.Noise1D(this->phase.x) * this->amplitude.x * this->amplitudeMult;

    rotationNoise.x = noiseGen.Noise1D(this->phaseNoise.x) * this->amplitude.x * noiseAmpMultiplier;

    rotation.y = noiseGen.Noise1D(this->phase.y) * this->amplitude.y * this->amplitudeMult;
    rotationNoise.y = noiseGen.Noise1D(this->phaseNoise.y) * this->amplitude.y * noiseAmpMultiplier;

    rotation.z = noiseGen.Noise1D(this->phase.z) * this->amplitude.z * this->amplitudeMult;
    rotationNoise.z = noiseGen.Noise1D(this->phaseNoise.z) * this->amplitude.z * noiseAmpMultiplier;

    angles += rotation + rotationNoise;

    return angles;
}
#if !defined(_RELEASE)
void CAnimCameraNode::ShakeParam::ShowCameraShakeDebug(EntityId camEntityId, int shakeIndex, float phaseDelta) const
{
    bool bSequenceCamInUse = gEnv->pMovieSystem->GetCallback() == NULL ||
        gEnv->pMovieSystem->GetCallback()->IsSequenceCamUsed();
    EntityId camId = GetLegacyEntityId(gEnv->pMovieSystem->GetCameraParams().cameraEntityId);
    if (!(camId == camEntityId
          && bSequenceCamInUse))
    {
        return;
    }

    f32 green[4] = {0, 1, 0, 1};
    float y = 10.0f;

    string camName = "";
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(camEntityId);
    if (pEntity)
    {
        camName = pEntity->GetName();
    }
    gEnv->pRenderer->Draw2dLabel(1.0f, y + 20.0f * shakeIndex, 1.3f, green, false, "#%d shaking of '%s' at %f",
        shakeIndex + 1, camName.c_str(), phaseDelta);
}
#endif //#if !defined(_RELEASE)

#undef s_nodeParamsInitialized
#undef s_nodeParams
#undef AddSupportedParam

