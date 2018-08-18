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

#ifndef _PIPEUSERMOVEMENTACTORADAPTER_H__
#define _PIPEUSERMOVEMENTACTORADAPTER_H__

#include <IMovementSystem.h>
#include <IAIActor.h>

class CPipeUser;

class PipeUserMovementActorAdapter
    : public IMovementActorAdapter
{
public:
    PipeUserMovementActorAdapter(CPipeUser& pipeUser)
        : m_lookTimeOffset(.0f)
        , m_attachedPipeUser(pipeUser)
    {
    }

    virtual ~PipeUserMovementActorAdapter() {}

    // IMovementActorCommunicationInterface
    virtual void OnMovementPlanProduced() override;

    virtual Vec3 GetPhysicsPosition() override;
    virtual Vec3 GetVelocity() override;
    virtual Vec3 GetMoveDirection() override;
    virtual Vec3 GetAnimationBodyDirection() override;
    virtual EActorTargetPhase GetActorPhase() override;
    virtual void SetMovementOutputValue(const PathFollowResult& result) override;
    virtual void SetBodyTargetDirection(const Vec3& direction) override;
    virtual void ResetMovementContext() override;
    virtual void ClearMovementState() override;
    virtual void ResetBodyTarget() override;
    virtual void ResetActorTargetRequest() override;
    virtual bool IsMoving() override;

    virtual void RequestExactPosition(const SAIActorTargetRequest* request, const bool lowerPrecision) override;

    virtual bool IsClosestToUseTheSmartObject(const OffMeshLink_SmartObject& smartObjectLink) override;
    virtual bool PrepareNavigateSmartObject(CSmartObject* pSmartObject, OffMeshLink_SmartObject* pSmartObjectLink) override;
    virtual void InvalidateSmartObjectLink(CSmartObject* pSmartObject, OffMeshLink_SmartObject* pSmartObjectLink) override;

    virtual void SetInCover(const bool inCover) override;
    virtual void UpdateCoverLocations() override;
    virtual void InstallInLowCover(const bool inCover) override;
    virtual void SetupCoverInformation() override;
    virtual bool IsInCover() override;

    virtual bool GetDesignedPath(SShape& pathShape) override;
    virtual void CancelRequestedPath() override;
    virtual void ConfigurePathfollower(const MovementStyle& style) override;

    virtual void SetActorPath(const MovementStyle& style, const INavPath& navPath) override;
    virtual void SetActorStyle(const MovementStyle& style, const INavPath& navPath) override;
    virtual void SetStance(const MovementStyle::Stance stance) override;

    virtual AZStd::shared_ptr<Vec3> CreateLookTarget() override;
    virtual void SetLookTimeOffset(float lookTimeOffset) override;
    virtual void UpdateLooking(float updateTime, AZStd::shared_ptr<Vec3> lookTarget, const bool targetReachable, const float pathDistanceToEnd, const Vec3& followTargetPosition, const MovementStyle& style) override;
    // ~IMovementActorCommunicationInterface

private:
    float m_lookTimeOffset;
    CPipeUser& m_attachedPipeUser;
};

#endif // _PIPEUSERMOVEMENTACTORADAPTER_H__