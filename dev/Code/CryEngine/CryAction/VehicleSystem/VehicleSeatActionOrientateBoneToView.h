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

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONORIENTATEBONETOVIEW_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONORIENTATEBONETOVIEW_H
#pragma once

#include "ICryAnimation.h"
#include <IComponent.h>

struct ISkeletonPose;
struct IAnimatedCharacter;

class CVehicleSeatActionOrientateBoneToView
    : public IVehicleSeatAction
{
    IMPLEMENT_VEHICLEOBJECT

private:

public:
    CVehicleSeatActionOrientateBoneToView();

    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual void StartUsing(EntityId passengerId);
    virtual void ForceUsage() {};
    virtual void StopUsing();
    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value) {};

    virtual void Serialize(TSerialize ser, EEntityAspects aspects) {};
    virtual void PostSerialize(){}
    virtual void Update(const float deltaTime) { }

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

    virtual void PrePhysUpdate(const float dt);

    virtual void GetMemoryUsage(ICrySizer* s) const;

protected:
    Ang3 GetDesiredViewAngles(const Vec3& lookPos, const Vec3& aimPos) const;
    Vec3 GetDesiredAimPosition() const;
    Vec3 GetCurrentLookPosition() const;

    IDefaultSkeleton* GetCharacterModelSkeleton() const;
    ISkeletonPose* GetSkeleton() const;

    IVehicle*       m_pVehicle;
    IVehicleSeat*   m_pSeat;

    IAnimationOperatorQueuePtr m_poseModifier;

    int m_MoveBoneId;
    int m_LookBoneId;

    float m_Sluggishness;

    Ang3 m_BoneOrientationAngles;
    Ang3 m_BoneSmoothingRate;
    Quat m_BoneBaseOrientation;

    IAnimatedCharacter* m_pAnimatedCharacter;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONORIENTATEBONETOVIEW_H
