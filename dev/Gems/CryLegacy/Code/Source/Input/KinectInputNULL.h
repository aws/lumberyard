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

#pragma once

class CKinectInputNULL
    : public IKinectInput
{
public:
    CKinectInputNULL(){}
    ~CKinectInputNULL(){}

    virtual bool Init() override {return false; }
    virtual void Update() override {}
    virtual bool IsEnabled() { return false; }
    virtual const char* GetUserStatusMessage() override {return ""; }

    virtual void RegisterInputListener(IKinectInputListener* pInputListener, const char* name) override {}
    virtual void UnregisterInputListener(IKinectInputListener* pInputListener) override {}
    virtual bool RegisterArcRail(int gripId, int railId, const Vec2& vScreenPos, const Vec3& vDir, float fLenght, float fDeadzoneLength, float fToleranceConeAngle) override {return false; }
    virtual void UnregisterArcRail(int gripId) override {}
    virtual bool RegisterHoverTimeRail(int gripId, int railId, const Vec2& vScreenPos, float fHoverTime, float fTimeTillCommit, SKinGripShape* pGripShape = NULL) override {return false; }
    virtual void UnregisterHoverTimeRail(int gripId) override {}
    virtual void UnregisterAllRails() override {}

    virtual bool GetBodySpaceHandles(SKinBodyShapeHandles& bodyShapeHandles) override { return false; }
    virtual bool GetSkeletonRawData(uint32 iUser, SKinSkeletonRawData& skeletonRawData) const override { return false; };
    virtual bool GetSkeletonDefaultData(uint32 iUser, SKinSkeletonDefaultData& skeletonDefaultData) const override { return false; };
    virtual void DebugDraw() override {}

    //Skeleton
    virtual void EnableSeatedSkeletonTracking(bool bValue) override {}
    virtual uint32 GetClosestTrackedSkeleton() const override {return KIN_SKELETON_INVALID_TRACKING_ID; }

    //  Wave
    virtual void EnableWaveGestureTracking(bool bEnable) override {};
    virtual float GetWaveGestureProgress(DWORD* pTrackingId) override { return 0.f; }

    // Identity
    virtual bool IdentityDetectedIntentToPlay(DWORD dwTrackingId) override {return false; }
    virtual bool IdentityIdentify(DWORD dwTrackingId, KinIdentifyCallback callbackFunc, void* pData) override {return false; }

    // Speech
    virtual bool SpeechEnable() override {return false; }
    virtual void SpeechDisable() override {}
    virtual void SetEnableGrammar(const string& grammarName, bool enable) override {}
    virtual bool KinSpeechSetEventInterest(unsigned long ulEvents) override { return false; }
    virtual bool KinSpeechLoadDefaultGrammar() override { return false; }
    virtual bool KinSpeechStartRecognition() override {return false; }
    virtual void KinSpeechStopRecognition() override {};
};
