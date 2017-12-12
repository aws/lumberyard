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

// Description : Implements a base class for the vehicle views


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWBASE_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWBASE_H
#pragma once

class CVehicleSeat;


class CVehicleViewBase
    : public IVehicleView
{
public:

    CVehicleViewBase();

    // IVehicleView
    virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void Reset();
    virtual void ResetPosition() {};
    virtual void Release() { delete this; }

    virtual const char* GetName() { return NULL; }
    virtual bool IsThirdPerson() = 0;
    virtual bool IsPassengerHidden() { return m_hidePlayer; }

    virtual void OnStartUsing(EntityId passengerId);
    virtual void OnStopUsing();

    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
    virtual void UpdateView(SViewParams& viewParams, EntityId playerId) {}

    virtual void Update(const float frameTime);
    virtual void Serialize(TSerialize serialize, EEntityAspects);

    virtual void SetDebugView(bool debug) { m_isDebugView = debug; }
    virtual bool IsDebugView() { return m_isDebugView; }

    virtual bool ShootToCrosshair() { return true; }
    virtual bool IsAvailableRemotely() const { return m_isAvailableRemotely; }
    // ~IVehicleView

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

    bool Init(CVehicleSeat* pSeat);

protected:

    IVehicle* m_pVehicle;
    CVehicleSeat* m_pSeat;

    // view settings (changed only inside Init)

    bool m_isRotating;

    Vec3 m_rotationMin;
    Vec3 m_rotationMax;
    Vec3 m_rotationInit;
    float m_relaxDelayMax;
    float m_relaxTimeMax;
    float m_velLenMin;
    float m_velLenMax;

    bool m_isSendingActionOnRotation;
    float m_rotationBoundsActionMult;

    // status variables (changed during run-time)

    Ang3 m_rotation;
    Vec3 m_rotatingAction;
    Ang3 m_viewAngleOffset;

    Vec3 m_rotationCurrentSpeed;
    float m_rotationValRateX; // used for SmoothCD call on the rotation
    float m_rotationValRateZ;
    float m_rotationTimeAcc;
    float m_rotationTimeDec;

    bool m_isRelaxEnabled;
    float m_relaxDelay;
    float m_relaxTime;

    int m_yawLeftActionOnBorderAAM;
    int m_yawRightActionOnBorderAAM;
    int m_pitchUpActionOnBorderAAM;
    int m_pitchDownActionOnBorderAAM;

    float m_pitchVal;
    float m_yawVal;

    struct SViewGeneratedAction
    {
        TVehicleActionId actionId;
        int activationMode;
    };

    bool m_hidePlayer;
    bool m_isDebugView;
    bool m_isAvailableRemotely;
    bool m_playerViewThirdOnExit;

    typedef std::vector <string> TVehiclePartNameVector;
    TVehiclePartNameVector m_hideParts;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWBASE_H
