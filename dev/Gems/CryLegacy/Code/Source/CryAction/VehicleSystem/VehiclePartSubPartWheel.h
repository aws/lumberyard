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

// Description : Implements a wheel vehicle part

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUBPARTWHEEL_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUBPARTWHEEL_H
#pragma once

#include "VehiclePartSubPart.h"

class CVehicle;

class CVehiclePartSubPartWheel
    : public CVehiclePartSubPart
    , public IVehicleWheel
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehiclePartSubPartWheel();
    virtual ~CVehiclePartSubPartWheel() {}


    // IVehiclePart
    virtual void Reset();
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);

    virtual bool ChangeState(EVehiclePartState state, int flags = 0);
    virtual void Physicalize();
    virtual void OnEvent(const SVehiclePartEvent& event);

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void PostSerialize();

    virtual IVehicleWheel* GetIWheel() { return this; }
    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
        CVehiclePartSubPart::GetMemoryUsageInternal(s);
    }
    // ~IVehiclePart

    // IVehicleWheel
    virtual int GetSlot() const { return m_slot; }
    virtual int GetWheelIndex() const { return m_pSharedParameters->m_wheelIndex; }
    virtual float GetTorqueScale() const { return m_pSharedParameters->m_torqueScale; }
    virtual float GetSlipFrictionMod(float slip) const;
    virtual const pe_cargeomparams* GetCarGeomParams() const { return &m_physGeomParams; }
    // ~IVehicleWheel

    float GetRimRadius() const { return m_pSharedParameters->m_rimRadius; }

    virtual void GetGeometryName(EVehiclePartState state, string& name);

protected:

    virtual void InitGeometry();
    bool IsFront() const { return m_physGeomParams.pivot.y > 0.f; }

    pe_cargeomparams m_physGeomParams;

    friend class CVehiclePartTread;
    friend class CVehicle;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUBPARTWHEEL_H
