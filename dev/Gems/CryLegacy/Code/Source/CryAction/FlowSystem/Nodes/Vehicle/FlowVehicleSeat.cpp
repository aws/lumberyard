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

// Description : Implements a flow node to vehicle seats


#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "IVehicleSystem.h"
#include "VehicleSystem/Vehicle.h"
#include "VehicleSystem/VehicleSeat.h"
#include "IFlowSystem.h"
#include "FlowSystem/Nodes/FlowBaseNode.h"
#include "FlowVehicleSeat.h"

//------------------------------------------------------------------------
CFlowVehicleSeat::CFlowVehicleSeat(SActivationInfo* pActivationInfo)
{
    Init(pActivationInfo);

    m_isDriverSeatRequested = false;
    m_seatId = InvalidVehicleSeatId;
}

//------------------------------------------------------------------------
IFlowNodePtr CFlowVehicleSeat::Clone(SActivationInfo* pActivationInfo)
{
    IFlowNode* pNode = new CFlowVehicleSeat(pActivationInfo);
    return pNode;
}

//------------------------------------------------------------------------
void CFlowVehicleSeat::GetConfiguration(SFlowNodeConfig& nodeConfig)
{
    CFlowVehicleBase::GetConfiguration(nodeConfig);

    static const SInputPortConfig pInConfig[] =
    {
        InputPortConfig<int>("seatId", _HELP("seatId (optional, use instead of seatName)"), _HELP("Seat"), _UICONFIG("enum_int:None=0,Driver=1,Gunner=2,Seat03=3,Seat04=4,Seat05=5,Seat06=6,Seat07=7,Seat08=8,Seat09=9,Seat10=10,Seat11=11")),
        InputPortConfig<string>("seatName", _HELP("the name of the seat which should be requested (optional, use instead of seatId)")),
        InputPortConfig<bool>("isDriverSeat", _HELP("used to select the driver seat (optional).")),
        InputPortConfig_Void ("Lock", _HELP("Locks the vehicle")),
        InputPortConfig_Void ("Unlock", _HELP("Unlocks the vehicle")),
        InputPortConfig<int> ("LockType", 0, _HELP("Who to lock the vehicle for"), NULL, _UICONFIG("enum_int:All=0,Players=1,AI=2")),
        {0}
    };

    static const SOutputPortConfig pOutConfig[] =
    {
        OutputPortConfig<int>("seatId", _HELP("the seat id selected by the input values")),
        OutputPortConfig<int>("passengerId", _HELP("the entity id of the current passenger on the seat, if there's any")),
        {0}
    };

    nodeConfig.sDescription = _HELP("Handle vehicle seats");
    nodeConfig.nFlags |= EFLN_TARGET_ENTITY;
    nodeConfig.pInputPorts = pInConfig;
    nodeConfig.pOutputPorts = pOutConfig;
    nodeConfig.SetCategory(EFLN_APPROVED);
}

//------------------------------------------------------------------------
void CFlowVehicleSeat::ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo)
{
    CFlowVehicleBase::ProcessEvent(flowEvent, pActivationInfo);

    if (flowEvent == eFE_Initialize || flowEvent == eFE_Activate)
    {
        CVehicle* pVehicle = static_cast<CVehicle*>(GetVehicle());

        if (!pVehicle)
        {
            return;
        }

        if (IsPortActive(pActivationInfo, IN_SEATID))
        {
            m_seatId = GetPortInt(pActivationInfo, IN_SEATID);
        }

        if (IsPortActive(pActivationInfo, IN_SEATNAME))
        {
            const string& seatName = GetPortString(pActivationInfo, IN_SEATNAME);
            if (!seatName.empty())
            {
                m_seatId = pVehicle->GetSeatId(seatName.c_str());
            }
        }

        if (IsPortActive(pActivationInfo, IN_DRIVERSEAT))
        {
            if (GetPortBool(pActivationInfo, IN_DRIVERSEAT))
            {
                m_seatId = pVehicle->GetDriverSeatId();
            }
        }

        if (IsPortActive(pActivationInfo, IN_LOCK))
        {
            if (CVehicleSeat* pSeat = (CVehicleSeat*)pVehicle->GetSeatById(m_seatId))
            {
                EVehicleSeatLockStatus lockType = eVSLS_Locked;
                int lock = GetPortInt(pActivationInfo, IN_LOCKTYPE);
                switch (lock)
                {
                case 0:
                    lockType = eVSLS_Locked;
                    break;
                case 1:
                    lockType = eVSLS_LockedForPlayers;
                    break;
                case 2:
                    lockType = eVSLS_LockedForAI;
                    break;
                default:
                    assert(false);
                    break;
                }
                pSeat->SetLocked(lockType);
            }
        }

        if (IsPortActive(pActivationInfo, IN_UNLOCK))
        {
            if (CVehicleSeat* pSeat = (CVehicleSeat*)pVehicle->GetSeatById(m_seatId))
            {
                pSeat->SetLocked(eVSLS_Unlocked);
            }
        }

        ActivateOutputPorts(pActivationInfo);
    }
}

//------------------------------------------------------------------------
void CFlowVehicleSeat::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
    CFlowVehicleBase::OnVehicleEvent(event, params);

    // prevent firing FG output if serializing
    // might need to be looked into again, in case something relies on PassengerEnter event to fire
    // a FG output here during loading time

    if (gEnv->pSystem->IsSerializingFile())
    {
        return;
    }

    if (event == eVE_PassengerEnter)
    {
        if (m_seatId != InvalidVehicleSeatId)
        {
            if (params.iParam == m_seatId)
            {
                if (IVehicle* pVehicle = GetVehicle())
                {
                    if (IVehicleSeat* pSeat = pVehicle->GetSeatById(m_seatId))
                    {
                        FlowEntityId actorId = FlowEntityId(pSeat->GetPassenger());
                        SFlowAddress addr(m_nodeID, OUT_PASSENGERID, true);
                        m_pGraph->ActivatePort(addr, actorId);
                    }
                }
            }
        }
    }
    else if (event == eVE_PassengerExit)
    {
        if (m_seatId != InvalidVehicleSeatId)
        {
            if (params.iParam == m_seatId)
            {
                if (IVehicle* pVehicle = GetVehicle())
                {
                    if (IVehicleSeat* pSeat = pVehicle->GetSeatById(m_seatId))
                    {
                        SFlowAddress addr(m_nodeID, OUT_PASSENGERID, true);
                        m_pGraph->ActivatePort(addr, 0);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------
void CFlowVehicleSeat::ActivateOutputPorts(SActivationInfo* pActivationInfo)
{
    IVehicle* pVehicle = GetVehicle();

    if (!pVehicle)
    {
        return;
    }

    if (m_seatId != InvalidVehicleSeatId)
    {
        ActivateOutput(pActivationInfo, OUT_SEATID, m_seatId);

        IVehicleSeat* pSeat = pVehicle->GetSeatById(m_seatId);
        if (pSeat)
        {
            TVehicleSeatId passengerId = pSeat->GetPassenger();
            ActivateOutput(pActivationInfo, OUT_PASSENGERID, passengerId);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
class CFlowVehicleChangeSeat
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum EInputPorts
    {
        eINP_Sync = 0,
        eINP_Seat
    };

    enum EOutputPorts
    {
        eOUT_Succeed = 0,
        eOUT_Fail
    };
public:
    CFlowVehicleChangeSeat(SActivationInfo* pActInfo) {}
    ~CFlowVehicleChangeSeat() {}

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inp_config[] = {
            InputPortConfig_Void ("Sync", _HELP("Triggers the seat change")),
            InputPortConfig<int>("SeatNumber", _HELP("New seat"), _HELP("Seat"), _UICONFIG("enum_int:None=0,Driver=1,Gunner=2,Seat03=3,Seat04=4,Seat05=5,Seat06=6,Seat07=7,Seat08=8,Seat09=9,Seat10=10,Seat11=11")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            OutputPortConfig_Void("Succeed"),
            OutputPortConfig_Void("Fail"),
            {0}
        };

        config.sDescription = _HELP("Moves an actor from a seat to another one. Only works if the actor is already inside a vehicle");
        config.pInputPorts = inp_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, eINP_Sync) && pActInfo->pEntity)
            {
                TVehicleSeatId newSeatId = GetPortInt(pActInfo, eINP_Seat);
                bool success = false;

                FlowEntityId actorEntityId = FlowEntityId(pActInfo->pEntity->GetId());
                IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorEntityId);
                if (pActor && pActor->GetLinkedVehicle())
                {
                    CVehicle* pVehicle = static_cast<CVehicle*>(pActor->GetLinkedVehicle());
                    pVehicle->ChangeSeat(actorEntityId, 0, newSeatId);
                    IVehicleSeat* pNewSeat = pVehicle->GetSeatForPassenger(actorEntityId);
                    success = pNewSeat && pNewSeat->GetSeatId() == newSeatId;
                }
                ActivateOutput(pActInfo, success ? eOUT_Succeed : eOUT_Fail, true);
            }
            break;
        }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
class CFlowVehicleLock
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum EInputPorts
    {
        eINP_Lock = 0,
        eINP_Unlock,
        eINP_LockFor,
    };

public:
    CFlowVehicleLock(SActivationInfo* pActInfo) {}
    ~CFlowVehicleLock() {}

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inp_config[] = {
            InputPortConfig_Void ("Lock", _HELP("Locks the vehicle")),
            InputPortConfig_Void ("Unlock", _HELP("Unlocks the vehicle")),
            InputPortConfig<int> ("LockType", 0, _HELP("Who to lock the vehicle for"), NULL, _UICONFIG("enum_int:All=0,Players=1,AI=2")),
            {0}
        };
        static const SOutputPortConfig out_config[] = {
            {0}
        };

        config.sDescription = _HELP("Locks/Unlocks all seats of a vehicle");
        config.pInputPorts = inp_config;
        config.pOutputPorts = out_config;
        config.nFlags |= EFLN_TARGET_ENTITY;
        config.SetCategory(EFLN_APPROVED);
    }
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            IVehicle* pVehicle = pActInfo->pEntity ? CCryAction::GetCryAction()->GetIVehicleSystem()->GetVehicle(pActInfo->pEntity->GetId()) : NULL;

            if (pVehicle)
            {
                int lockFor = GetPortInt(pActInfo, eINP_LockFor);

                if (IsPortActive(pActInfo, eINP_Lock))
                {
                    Lock(pVehicle, true, lockFor);
                }
                if (IsPortActive(pActInfo, eINP_Unlock))
                {
                    Lock(pVehicle, false, 0);
                }
            }
            break;
        }
        }
    }


    void Lock(IVehicle* pVehicle, bool lock, int lockFor)
    {
        EVehicleSeatLockStatus lockType = eVSLS_Unlocked;
        if (lock)
        {
            switch (lockFor)
            {
            case 0:
                lockType = eVSLS_Locked;
                break;
            case 1:
                lockType = eVSLS_LockedForPlayers;
                break;
            case 2:
                lockType = eVSLS_LockedForAI;
                break;

            default:
                assert(false);
                break;
            }
        }

        for (uint32 i = 0; i < pVehicle->GetSeatCount(); ++i)
        {
            CVehicleSeat* pSeat = static_cast<CVehicleSeat*>(pVehicle->GetSeatById(i + 1));             // vehicle seats start at id 1
            if (pSeat)
            {
                pSeat->SetLocked(lockType);
            }
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};



REGISTER_FLOW_NODE("Vehicle:Seat", CFlowVehicleSeat);
REGISTER_FLOW_NODE("Vehicle:ChangeSeat", CFlowVehicleChangeSeat)
REGISTER_FLOW_NODE("Vehicle:Lock", CFlowVehicleLock)
