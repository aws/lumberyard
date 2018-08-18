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

// Description: Implements a part for vehicles which uses static objects

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSTATIC_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSTATIC_H
#pragma once

class CVehicle;

class CVehiclePartStatic
    : public CVehiclePartBase
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehiclePartStatic() {}
    ~CVehiclePartStatic() {}

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void InitGeometry();
    virtual void Release();
    virtual void Reset();

    virtual void Physicalize();

    virtual void Update(const float frameTime);

    virtual void SetLocalTM(const Matrix34& localTM);

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
        s->AddObject(m_filename);
        s->AddObject(m_filenameDestroyed);
        s->AddObject(m_geometry);
        CVehiclePartBase::GetMemoryUsageInternal(s);
    }
    // ~IVehiclePart

protected:

    string m_filename;
    string m_filenameDestroyed;
    string m_geometry;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSTATIC_H
