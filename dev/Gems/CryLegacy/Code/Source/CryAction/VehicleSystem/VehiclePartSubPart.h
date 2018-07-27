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

// Description : Implements a part for vehicles which is the an attachment
//               of a parent Animated part


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUBPART_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUBPART_H
#pragma once

#include "VehiclePartBase.h"

class CVehicle;

class CVehiclePartSubPart
    : public CVehiclePartBase
{
    IMPLEMENT_VEHICLEOBJECT

public:

    CVehiclePartSubPart();

    virtual ~CVehiclePartSubPart();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* pParent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void Reset();
    virtual void Release();
    virtual void OnEvent(const SVehiclePartEvent& event);
    virtual bool ChangeState(EVehiclePartState state, int flags = 0);
    virtual void Physicalize();
    virtual void Update(const float frameTime);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void GetMemoryUsageInternal(ICrySizer* pSizer) const;
    virtual const Matrix34& GetLocalInitialTM();
    // ~IVehiclePart

protected:

    virtual void InitGeometry();

    void RegisterStateGeom(EVehiclePartState state, IStatObj* pStatObj);

    Matrix34    m_savedTM;

private:

    struct StateGeom
    {
        inline StateGeom()
            : state(eVGS_Default)
            , pStatObj(NULL)
        {
        }

        inline StateGeom(EVehiclePartState _state, IStatObj* _pStatObj)
            : state(_state)
            , pStatObj(_pStatObj)
        {
            if (pStatObj)
            {
                pStatObj->AddRef();
            }
        }

        inline StateGeom(const StateGeom& other)
            : state(other.state)
            , pStatObj(other.pStatObj)
        {
            if (pStatObj)
            {
                pStatObj->AddRef();
            }
        }

        ~StateGeom()
        {
            if (pStatObj)
            {
                pStatObj->Release();
            }
        }

        inline StateGeom& operator = (const StateGeom& other)
        {
            state           = other.state;
            pStatObj    = other.pStatObj;

            if (pStatObj)
            {
                pStatObj->AddRef();
            }

            return *this;
        }

        EVehiclePartState   state;
        IStatObj* pStatObj;
    };

    typedef std::vector<StateGeom> TStateGeomVector;

    TStateGeomVector    m_stateGeoms;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTSUBPART_H
