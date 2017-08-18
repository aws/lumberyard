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

// Description : Implements a simple box-shaped part for mass distribution


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTMASSBOX_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTMASSBOX_H
#pragma once

#include "VehiclePartBase.h"

class CVehicle;


class CVehiclePartMassBox
    : public CVehiclePartBase
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehiclePartMassBox();
    ~CVehiclePartMassBox();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void Reset();

    virtual void Physicalize();

    virtual const Matrix34& GetLocalTM(bool relativeToParentPart, bool forced = false);
    virtual const Matrix34& GetWorldTM();
    virtual const AABB& GetLocalBounds();

    virtual void Update(const float frameTime);
    virtual void Serialize(TSerialize ser, EEntityAspects aspects);

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
        CVehiclePartBase::GetMemoryUsageInternal(s);
    }
    // ~IVehiclePart

protected:

    Matrix34 m_localTM;
    Matrix34 m_worldTM;
    Vec3 m_size;
    float m_drivingOffset;

private:
    enum EMassBoxDrivingType
    {
        eMASSBOXDRIVING_DEFAULT         = 1 << 0,
        eMASSBOXDRIVING_NORMAL          = 1 << 1,
        eMASSBOXDRIVING_INTHEAIR        = 1 << 2,
        eMASSBOXDRIVING_INTHEWATER  = 1 << 3,
        eMASSBOXDRIVING_DESTROYED       = 1 << 4,
    };
    void SetupDriving(EMassBoxDrivingType drive);
    EMassBoxDrivingType m_Driving; //0 default 1 driving 2 in the air
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTMASSBOX_H
