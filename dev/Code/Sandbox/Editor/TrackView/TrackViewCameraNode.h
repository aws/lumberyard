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

#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWCAMERANODE_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWCAMERANODE_H
#pragma once


#include "TrackViewAnimNode.h"
#include "Objects/CameraObject.h"

////////////////////////////////////////////////////////////////////////////
//
// This class represents an IAnimNode that is a camera
//
////////////////////////////////////////////////////////////////////////////
class CTrackViewCameraNode
    : public CTrackViewAnimNode
    , public ICameraObjectListener
{
public:
    CTrackViewCameraNode(IAnimSequence* pSequence, IAnimNode* pAnimNode, CTrackViewNode* pParentNode)
        : CTrackViewAnimNode(pSequence, pAnimNode, pParentNode) {}

    virtual void OnNodeAnimated(IAnimNode* pNode) override;

    virtual void BindToEditorObjects() override;
    virtual void UnBindFromEditorObjects() override;

    // Get camera shake rotation
    void GetShakeRotation(const float time, Quat& rotation);

private:
    virtual void OnFovChange(const float fov);
    virtual void OnNearZChange(const float nearZ);
    virtual void OnFarZChange(const float farZ) {}
    virtual void OnShakeAmpAChange(const Vec3 amplitude);
    virtual void OnShakeAmpBChange(const Vec3 amplitude);
    virtual void OnShakeFreqAChange(const Vec3 frequency);
    virtual void OnShakeFreqBChange(const Vec3 frequency);
    virtual void OnShakeMultChange(const float amplitudeAMult, const float amplitudeBMult, const float frequencyAMult, const float frequencyBMult);
    virtual void OnShakeNoiseChange(const float noiseAAmpMult, const float noiseBAmpMult, const float noiseAFreqMult, const float noiseBFreqMult);
    virtual void OnShakeWorkingChange(const float timeOffsetA, const float timeOffsetB);
    virtual void OnCameraShakeSeedChange(const int seed);
};
#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWCAMERANODE_H
