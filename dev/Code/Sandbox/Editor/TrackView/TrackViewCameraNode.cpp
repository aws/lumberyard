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
#include "TrackViewCameraNode.h"
#include "TrackViewSequence.h"

void CTrackViewCameraNode::OnNodeAnimated(IAnimNode* pNode)
{
    CTrackViewAnimNode::OnNodeAnimated(pNode);

    const float time = GetSequence()->GetTime();

    CEntityObject* pEntityObject = GetNodeEntity(false);
    if (pEntityObject)
    {
        CCameraObject* pCameraObject = static_cast<CCameraObject*>(pEntityObject);

        // Get fov out of node at current time.
        float fov = RAD2DEG(pCameraObject->GetFOV());

        if (GetParamValue(time, eAnimParamType_FOV, fov))
        {
            pCameraObject->SetFOV(DEG2RAD(fov));
        }

        float nearZ = pCameraObject->GetNearZ();
        if (GetParamValue(time, eAnimParamType_NearZ, nearZ))
        {
            pCameraObject->SetNearZ(nearZ);
        }

        Vec4 multipliers = Vec4(pCameraObject->GetAmplitudeAMult(), pCameraObject->GetAmplitudeBMult(), pCameraObject->GetFrequencyAMult(), pCameraObject->GetFrequencyBMult());
        if (GetParamValue(time, eAnimParamType_ShakeMultiplier, multipliers))
        {
            pCameraObject->SetAmplitudeAMult(multipliers.x);
            pCameraObject->SetAmplitudeBMult(multipliers.y);
            pCameraObject->SetFrequencyAMult(multipliers.z);
            pCameraObject->SetFrequencyBMult(multipliers.w);
        }
    }
}

void CTrackViewCameraNode::BindToEditorObjects()
{
    CTrackViewAnimNode::BindToEditorObjects();

    CEntityObject* pEntityObject = GetNodeEntity(false);
    if (pEntityObject)
    {
        CCameraObject* pCameraObject = static_cast<CCameraObject*>(pEntityObject);
        pCameraObject->RegisterCameraListener(this);
    }
}

void CTrackViewCameraNode::UnBindFromEditorObjects()
{
    CEntityObject* pEntityObject = GetNodeEntity(false);
    if (pEntityObject)
    {
        CCameraObject* pCameraObject = static_cast<CCameraObject*>(pEntityObject);
        pCameraObject->UnregisterCameraListener(this);
    }

    CTrackViewAnimNode::UnBindFromEditorObjects();
}

void CTrackViewCameraNode::GetShakeRotation(const float time, Quat& rotation)
{
    GetAnimNode()->GetShakeRotation(time, rotation);
}

void CTrackViewCameraNode::OnFovChange(const float fov)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_FOV, fov);
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnNearZChange(const float nearZ)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_NearZ, nearZ);
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnShakeAmpAChange(const Vec3 amplitude)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_ShakeAmplitudeA, amplitude);
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnShakeAmpBChange(const Vec3 amplitude)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_ShakeAmplitudeB, amplitude);
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnShakeFreqAChange(const Vec3 frequency)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_ShakeFrequencyA, frequency);
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnShakeFreqBChange(const Vec3 frequency)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_ShakeFrequencyB, frequency);
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnShakeMultChange(const float amplitudeAMult, const float amplitudeBMult, const float frequencyAMult, const float frequencyBMult)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_ShakeMultiplier, Vec4(amplitudeAMult, amplitudeBMult, frequencyAMult, frequencyBMult));
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnShakeNoiseChange(const float noiseAAmpMult, const float noiseBAmpMult, const float noiseAFreqMult, const float noiseBFreqMult)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_ShakeNoise, Vec4(noiseAAmpMult, noiseBAmpMult, noiseAFreqMult, noiseBFreqMult));
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnShakeWorkingChange(const float timeOffsetA, const float timeOffsetB)
{
    const float time = GetSequence()->GetTime();
    SetParamValue(time, eAnimParamType_ShakeWorking, Vec4(timeOffsetA, timeOffsetB, 0.0f, 0.0f));
    GetSequence()->OnKeysChanged();
}

void CTrackViewCameraNode::OnCameraShakeSeedChange(const int seed)
{
    GetAnimNode()->SetCameraShakeSeed(seed);
    GetSequence()->OnKeysChanged();
}