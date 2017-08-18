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

// Description : Implements a attachment socket for vehicle entity attachments

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTATTACHMENT_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTATTACHMENT_H
#pragma once

#include "VehiclePartBase.h"

class CVehicle;


class CVehiclePartEntityAttachment
    : public CVehiclePartBase
{
    IMPLEMENT_VEHICLEOBJECT
public:

    // IVehiclePartAttachment
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    //virtual void InitGeometry();
    virtual void PostInit();

    virtual void Update(const float frameTime);

    virtual const Matrix34& GetLocalTM(bool relativeToParentPart, bool forced = false);
    virtual const Matrix34& GetWorldTM();

    virtual void SetLocalTM(const Matrix34& localTM);
    virtual const AABB& GetLocalBounds();

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
        CVehiclePartBase::GetMemoryUsageInternal(s);
    }
    // ~IVehiclePartAttachment

    void SetAttachmentEntity(EntityId entityId);
    IEntity* GetAttachmentEntity();

protected:
    void UpdateAttachment();
    void DetachAttachment();

    EntityId m_attachmentId;

    Matrix34 m_localTM;
    Matrix34 m_worldTM;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTATTACHMENT_H
