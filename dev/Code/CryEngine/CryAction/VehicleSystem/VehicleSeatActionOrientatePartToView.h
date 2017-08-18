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

// Description : Implements a seat action for orientate one or multiple vehicle parts
//               towards the view direction of the seat passanger


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONORIENTATEPARTTOVIEW_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONORIENTATEPARTTOVIEW_H
#pragma once

class CVehicleSeatActionOrientatePartToView
    : public IVehicleSeatAction
{
    IMPLEMENT_VEHICLEOBJECT

private:

    struct SOrientatePartInfo
    {
        SOrientatePartInfo()
            : partIndex(-1)
            , orientationAxis(1.0f, 1.0f, 1.0f)
        {
        }

        int     partIndex;
        Vec3    orientationAxis;
    };

public:

    CVehicleSeatActionOrientatePartToView();

    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset() {}
    virtual void Release() { delete this; }

    virtual void StartUsing(EntityId passengerId);
    virtual void ForceUsage() {};
    virtual void StopUsing();
    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value) {};

    virtual void Serialize(TSerialize ser, EEntityAspects aspects) {};
    virtual void PostSerialize(){}
    virtual void Update(const float deltaTime);

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

    virtual void GetMemoryUsage(ICrySizer* s) const;

protected:

    Vec3 GetViewDirection() const;

    typedef std::vector<SOrientatePartInfo> TPartIndices;

    IVehicle*       m_pVehicle;
    IVehicleSeat*   m_pSeat;

    TPartIndices    m_controlledParts;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONORIENTATEPARTTOVIEW_H
