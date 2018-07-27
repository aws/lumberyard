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

// Description : Implements an entity class which can serialize vehicle parts


#include "CryLegacy_precompiled.h"
#include "VehicleSeatSerializer.h"
#include "Vehicle.h"
#include "VehicleSeat.h"
#include "CryAction.h"
#include "Network/GameContext.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CVehicleSeatSerializer, CVehicleSeatSerializer)

//------------------------------------------------------------------------
CVehicleSeatSerializer::CVehicleSeatSerializer()
    :  m_pVehicle(0)
    , m_pSeat(0)
{
}

//------------------------------------------------------------------------
CVehicleSeatSerializer::~CVehicleSeatSerializer()
{
}

//------------------------------------------------------------------------
bool CVehicleSeatSerializer::Init(IGameObject* pGameObject)
{
    SetGameObject(pGameObject);

    if (m_pSeat) // needs to be done here since anywhere earlier, EntityId is not known
    {
        m_pSeat->SetSerializer(this);
    }
    else if (!gEnv->bServer)
    {
        return false;
    }

    CVehicleSeat* pSeat = NULL;
    EntityId parentId = 0;
    if (gEnv->bServer)
    {
        pSeat = static_cast<CVehicleSystem*>(CCryAction::GetCryAction()->GetIVehicleSystem())->GetInitializingSeat();
        CRY_ASSERT(pSeat);
        SetVehicle(static_cast<CVehicle*>(pSeat->GetVehicle()));
        parentId = m_pVehicle->GetEntityId();
    }

    if (0 == (GetEntity()->GetFlags() & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY)))
    {
        if (!GetGameObject()->BindToNetworkWithParent(eBTNM_Normal, parentId))
        {
            return false;
        }
    }

    GetEntity()->Activate(0);
    GetEntity()->Hide(true);

    if (!IsDemoPlayback())
    {
        if (gEnv->bServer)
        {
            pSeat->SetSerializer(this);
            SetSeat(pSeat);
        }
        else
        {
            if (m_pVehicle)
            {
                GetGameObject()->SetNetworkParent(m_pVehicle->GetEntityId());
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------
void CVehicleSeatSerializer::InitClient(ChannelId channelId)
{
}

//------------------------------------------------------------------------
bool CVehicleSeatSerializer::ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
    ResetGameObject();

    CRY_ASSERT_MESSAGE(false, "CVehicleSeatSerializer::ReloadExtension not implemented");

    return false;
}

//------------------------------------------------------------------------
bool CVehicleSeatSerializer::GetEntityPoolSignature(TSerialize signature)
{
    CRY_ASSERT_MESSAGE(false, "CVehicleSeatSerializer::GetEntityPoolSignature not implemented");

    return true;
}

//------------------------------------------------------------------------
void CVehicleSeatSerializer::FullSerialize(TSerialize ser)
{
}

//------------------------------------------------------------------------
bool CVehicleSeatSerializer::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
    if (m_pSeat)
    {
        m_pSeat->SerializeActions(ser, aspect);
    }
    return true;
}

//------------------------------------------------------------------------
void CVehicleSeatSerializer::Update(SEntityUpdateContext& ctx, int)
{
}

//------------------------------------------------------------------------
void CVehicleSeatSerializer::HandleEvent(const SGameObjectEvent& event)
{
}

//------------------------------------------------------------------------
void CVehicleSeatSerializer::SerializeSpawnInfo(TSerialize ser, IEntity* entity)
{
    EntityId vehicle = INVALID_ENTITYID;
    TVehicleSeatId seatId;

    ser.Value("vehicle", vehicle, 'eid');
    ser.Value("seat", seatId, 'seat');

    CRY_ASSERT(ser.IsReading());

    // warning GameObject not set at this point
    // GetGameObject calls will fail miserably

    m_pVehicle = static_cast<CVehicle*>(CCryAction::GetCryAction()->GetIVehicleSystem()->GetVehicle(vehicle));
    if (!m_pVehicle)
    {
        return;
    }

    CVehicleSeat* pSeat = static_cast<CVehicleSeat*>(m_pVehicle->GetSeatById(seatId));
    m_pSeat = pSeat;

    // the serializer is set on the seat in ::Init
}

namespace CVehicleSeatSerializerGetSpawnInfo
{
    struct SInfo
        : public ISerializableInfo
    {
        EntityId vehicle;
        TVehicleSeatId seatId;
        void SerializeWith(TSerialize ser)
        {
            ser.Value("vehicle", vehicle, 'eid');
            ser.Value("seat", seatId, 'seat');
        }
    };
}

//------------------------------------------------------------------------
ISerializableInfoPtr CVehicleSeatSerializer::GetSpawnInfo()
{
    CVehicleSeatSerializerGetSpawnInfo::SInfo* p = new CVehicleSeatSerializerGetSpawnInfo::SInfo;
    p->vehicle = m_pVehicle->GetEntityId();
    p->seatId = m_pSeat->GetSeatId();
    return p;
}

//------------------------------------------------------------------------
void CVehicleSeatSerializer::SetVehicle(CVehicle* pVehicle)
{
    m_pVehicle = pVehicle;
}

//------------------------------------------------------------------------
void CVehicleSeatSerializer::SetSeat(CVehicleSeat* pSeat)
{
    m_pSeat = pSeat;
}
