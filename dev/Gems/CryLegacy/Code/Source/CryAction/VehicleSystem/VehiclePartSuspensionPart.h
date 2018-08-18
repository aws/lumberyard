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

// Description : Implements a suspension part


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUSPENSIONPART_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUSPENSIONPART_H
#pragma once

#include "VehicleSystem/VehiclePartSubPart.h"

class CVehicle;
class CVehiclePartAnimated;

class CVehiclePartSuspensionPart
    : public CVehiclePartSubPart
{
    IMPLEMENT_VEHICLEOBJECT

private:
    typedef CVehiclePartSubPart inherited;

public:
    CVehiclePartSuspensionPart();
    virtual ~CVehiclePartSuspensionPart();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* pParent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void PostInit();
    virtual void Reset();
    virtual void Release();
    virtual void OnEvent(const SVehiclePartEvent& event);
    virtual bool ChangeState(EVehiclePartState state, int flags = 0);
    virtual void Physicalize();
    virtual void Update(const float frameTime);
    // ~IVehiclePart

protected:
    enum
    {
        k_modeRotate = 0,         // Rotate towards the target, but dont translate or stretch the joint
        k_modeStretch,            // Rotate towards EF and stretch so that the end-effector hits the target
        k_modeSnapToEF,           // Rotate and translate so that the end-effector hits the target
    };
    enum
    {
        k_flagTargetHelper = 0x1,                // Target was specified using a helper
        k_flagIgnoreTargetRotation = 0x2,        // Dont use the target's rotation (i.e. for wheels)
    };
    CVehiclePartAnimated* m_animatedRoot;
    CVehiclePartBase* m_targetPart;                // The part the target lies on
    Quat m_initialRot;                             // This is not always ID, so need to store it
    Vec3 m_targetOffset;                           // target position on target part
    Vec3 m_pos0;                                   // Initial position, relative to parent
    Vec3 m_direction0;                             // The initial direction vector
    float m_invLength0;                            // Inverse length of direction0
    int16 m_jointId;                               // Joint ID of cga statobj that was overridden
    int8 m_mode;                                   // IK mode
    uint8 m_ikFlags;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUSPENSIONPART_H
