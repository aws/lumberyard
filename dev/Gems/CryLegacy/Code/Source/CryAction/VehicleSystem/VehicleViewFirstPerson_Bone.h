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

// Description : Implements the first person pit view for vehicles


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWFIRSTPERSON_BONE_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWFIRSTPERSON_BONE_H
#pragma once

#include "VehicleViewFirstPerson.h"

class CVehicleViewFirstPerson_Bone
    : public CVehicleViewFirstPerson
{
    IMPLEMENT_VEHICLEOBJECT;

public:
    CVehicleViewFirstPerson_Bone();
    virtual ~CVehicleViewFirstPerson_Bone() {}

    virtual const char* GetName() { return m_name; }

    virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);

protected:
    virtual Quat GetWorldRotGoal();
    virtual Vec3 GetWorldPosGoal();

    int     m_MoveBoneId;
    Quat    m_additionalRotation;

    static const char* m_name;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEVIEWFIRSTPERSON_BONE_H

