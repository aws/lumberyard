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

//////////////////////////////////////////////////////////////////////////
//
// Ebus support for triggering necessary update of Entity/Object when
// associated static geometry is hot loaded in the level on the fly as
// Asset Pipeline finishes the job.
//
//////////////////////////////////////////////////////////////////////////

#ifndef CRYINCLUDE_CRYCOMMON_STATOBJBUS_H
#define CRYINCLUDE_CRYCOMMON_STATOBJBUS_H

#include <AzCore/EBus/EBus.h>

struct IStatObj;

class StatObjEvents
    : public AZ::EBusTraits
{
public:
    virtual ~StatObjEvents() = default;

    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    typedef IStatObj* BusIdType;

    virtual void OnStatObjReloaded()
    {
    }
};

using StatObjEventBus = AZ::EBus<StatObjEvents>;

#endif // CRYINCLUDE_CRYCOMMON_STATOBJBUS_H
#pragma once