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

// Description : Vehicle part class that spawns an entity and attaches it to the vehicle.

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTENTITY_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTENTITY_H
#pragma once

#include "IGameObject.h"
#include "VehiclePartBase.h"

class CVehiclePartEntity
    : public CVehiclePartBase
{
public:

    IMPLEMENT_VEHICLEOBJECT

    CVehiclePartEntity();

    ~CVehiclePartEntity();

    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void PostInit();
    virtual void Reset();
    virtual void Update(const float frameTime);
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

    void TryDetachPart(const SVehicleEventParams& params);

    virtual void Serialize(TSerialize serializer, EEntityAspects aspects);
    virtual void PostSerialize();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    IEntity* GetPartEntity() const;

protected:

    bool EntityAttached() const { return m_entityAttached; }
    EntityId GetPartEntityId() const { return m_entityId; }

private:

    struct Flags
    {
        enum
        {
            Hidden                  = 1,
            EntityAttached          = 2,
            Destroyed               = 4,
        };
    };

    void UpdateEntity();
    void DestroyEntity();

    void SafeRemoveBuddyConstraint();

    int                         m_index;
    uint                        m_constraintId;

    string                  m_entityName, m_entityArchetype, m_helperName;

    EntityId                m_entityId;
    IVehicleHelper* m_pHelper;
    IEntityLink*        m_pLink;
    uint16                  m_entityNetId;

    bool                        m_hidden : 1;
    bool                        m_entityAttached : 1;
    bool                        m_destroyed : 1;
    bool                        m_CollideWithParent : 1;
    bool                        m_createBuddyConstraint : 1;
    bool                        m_shouldDetachInsteadOfDestroy : 1;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTENTITY_H
