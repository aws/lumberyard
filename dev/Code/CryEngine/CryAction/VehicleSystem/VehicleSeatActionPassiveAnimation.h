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

// Description : This is a simplified version of CVehicleSeatActionPassiveAnimation
//               that will only run the animation as long as someone is seated in
//               the vehicle.
//
//               It will also not ForceFinish() on the animation action but justs stops
//               it normally, so that we are guaranteed to trigger blend-transitions.
//


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONPASSIVEANIMATION_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONPASSIVEANIMATION_H
#pragma once


class CVehicleSeatActionPassiveAnimation
    : public IVehicleSeatAction
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehicleSeatActionPassiveAnimation();
    ~CVehicleSeatActionPassiveAnimation();

    // IVehicleSeatAction
    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual void StartUsing(EntityId passengerId);
    virtual void ForceUsage() {};
    virtual void StopUsing();
    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value) {};

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void PostSerialize(){}
    virtual void Update(const float deltaTime) {};

    virtual void GetMemoryUsage(ICrySizer* s) const {s->Add(*this); }
    // ~IVehicleSeatAction

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}


private:

    // Animation control:
    void StartAnimation();
    void StopAnimation();


private:

    IVehicle* m_pVehicle;
    TAction<SAnimationContext>* m_pAnimationAction;
    FragmentID m_animationFragmentID;

    // The entity ID of the seated user (0 if none).
    EntityId m_userId;
};


#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONPASSIVEANIMATION_H
