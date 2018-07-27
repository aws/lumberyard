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

#ifndef CRYINCLUDE_CRYAISYSTEM_AIFLYINGVEHICLE_H
#define CRYINCLUDE_CRYAISYSTEM_AIFLYINGVEHICLE_H
#pragma once

#include "Puppet.h"

class CAIFlyingVehicle
    : public CPuppet
{
    typedef CPuppet Base;

public:

    CAIFlyingVehicle   ();
    virtual ~CAIFlyingVehicle  ();

    void          SetObserver       (bool observer);
    static void   OnVisionChanged   (const VisionID& observerID,
        const ObserverParams& observerParams,
        const VisionID& observableID,
        const ObservableParams& observableParams,
        bool visible);

    //virtual void  Reset               (EObjectResetType type);

    virtual void Serialize(TSerialize ser);
    virtual void PostSerialize();
    virtual void SetSignal(int nSignalID, const char* szText, IEntity* pSender = 0, IAISignalExtraData* pData = NULL, uint32 crcCode = 0);

private:
    bool m_combatModeEnabled;
    bool m_firingAllowed;
};



#endif // CRYINCLUDE_CRYAISYSTEM_AIFLYINGVEHICLE_H
