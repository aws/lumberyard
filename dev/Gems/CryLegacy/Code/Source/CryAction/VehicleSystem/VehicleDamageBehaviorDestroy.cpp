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

// Description : Implements a damage behavior which destroy a vehicle


#include "CryLegacy_precompiled.h"

#include "IVehicleSystem.h"

#include "Vehicle.h"
#include "VehicleDamageBehaviorDestroy.h"
#include "VehicleComponent.h"
#include "VehicleSeat.h"

//------------------------------------------------------------------------
bool CVehicleDamageBehaviorDestroy::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
    m_pVehicle = (CVehicle*) pVehicle;

    if (table.haveAttr("effect"))
    {
        m_effectName = table.getAttr("effect");
    }

    return true;
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorDestroy::Reset()
{
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorDestroy::OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams)
{
    if (event == eVDBE_Repair)
    {
        return;
    }

    if (behaviorParams.componentDamageRatio >= 1.0f)
    {
        SetDestroyed(true, behaviorParams.shooterId);
    }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorDestroy::SetDestroyed(bool isDestroyed, EntityId shooterId)
{
    // destroy the vehicle
    IEntity* pVehicleEntity = m_pVehicle->GetEntity();

    bool destroyed = m_pVehicle->IsDestroyed();
    m_pVehicle->SetDestroyedStatus(isDestroyed);

    if (!destroyed && isDestroyed)
    {
        CryLog ("%s being driven by %s has been destroyed!", m_pVehicle->GetEntity()->GetEntityTextDescription(), m_pVehicle->GetDriver() ? m_pVehicle->GetDriver()->GetEntity()->GetEntityTextDescription() : "nobody");
        INDENT_LOG_DURING_SCOPE();

        // do some obscure things over script side
        HSCRIPTFUNCTION scriptFunction(NULL);
        if (pVehicleEntity->GetScriptTable()->GetValue("OnVehicleDestroyed", scriptFunction) && scriptFunction)
        {
            IScriptSystem*  pIScriptSystem = gEnv->pScriptSystem;
            Script::Call(pIScriptSystem, scriptFunction, pVehicleEntity->GetScriptTable());
            pIScriptSystem->ReleaseFunc(scriptFunction);
        }
        // notify the other components

        TVehicleComponentVector& components = m_pVehicle->GetComponents();

        for (TVehicleComponentVector::iterator ite = components.begin(); ite != components.end(); ++ite)
        {
            CVehicleComponent* pComponent = *ite;
            pComponent->OnVehicleDestroyed();
        }

        // tell the parts to thrash themselves, if possible

        TVehiclePartVector parts = m_pVehicle->GetParts();
        IVehiclePart::SVehiclePartEvent partEvent;
        partEvent.type = IVehiclePart::eVPE_Damaged;
        partEvent.fparam = 1.0f;

        for (TVehiclePartVector::iterator ite = parts.begin(); ite != parts.end(); ++ite)
        {
            IVehiclePart* pPart = ite->second;
            pPart->OnEvent(partEvent);
        }

        if (IVehicleMovement* pVehicleMovement = m_pVehicle->GetMovement())
        {
            SVehicleMovementEventParams movementParams;
            movementParams.fValue = 1.0f;
            movementParams.bValue = true;
            pVehicleMovement->OnEvent(IVehicleMovement::eVME_Damage, movementParams);

            movementParams.fValue = 1.0f;
            movementParams.bValue = false;
            pVehicleMovement->OnEvent(IVehicleMovement::eVME_VehicleDestroyed, movementParams);
        }

        SVehicleEventParams eventParams;
        eventParams.entityId = shooterId;
        m_pVehicle->BroadcastVehicleEvent(eVE_Destroyed, eventParams);

        const char pEventName[] = "Destroy";
        const bool val = true;

        SEntityEvent entityEvent;
        entityEvent.event = ENTITY_EVENT_SCRIPT_EVENT;
        entityEvent.nParam[0] = (INT_PTR) pEventName;
        entityEvent.nParam[1] = IEntityClass::EVT_BOOL;
        entityEvent.nParam[2] = (INT_PTR)&val;
        m_pVehicle->GetEntity()->SendEvent(entityEvent);

        IAISystem* pAISystem = gEnv->pAISystem;
        if (pAISystem && pAISystem->IsEnabled() && pAISystem->GetSmartObjectManager())
        {
            gEnv->pAISystem->GetSmartObjectManager()->SetSmartObjectState(m_pVehicle->GetEntity(), "Dead");
        }

        m_pVehicle->OnDestroyed();
    }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorDestroy::Serialize(TSerialize ser, EEntityAspects aspects)
{
}

DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorDestroy);
