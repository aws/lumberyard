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

// Description : Implements a seat action to handle IK on the passenger body

#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "IActorSystem.h"
#include "IVehicleSystem.h"
#include "Vehicle.h"
#include "VehicleSeatActionPassengerIK.h"

#include "IRenderAuxGeom.h"

//------------------------------------------------------------------------
bool CVehicleSeatActionPassengerIK::Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table)
{
    m_pVehicle = pVehicle;
    m_passengerId = 0;

    CVehicleParams ikTable = table.findChild("PassengerIK");
    if (!ikTable)
    {
        return false;
    }

    CVehicleParams limbsTable = ikTable.findChild("Limbs");
    if (!limbsTable)
    {
        return false;
    }

    if (!ikTable.getAttr("waitShortlyBeforeStarting", m_waitShortlyBeforeStarting))
    {
        m_waitShortlyBeforeStarting = false;
    }

    int i = 0;
    int c = limbsTable.getChildCount();
    m_ikLimbs.reserve(c);

    for (; i < c; i++)
    {
        if (CVehicleParams limbTable = limbsTable.getChild(i))
        {
            SIKLimb limb;

            if (limbTable.haveAttr("limb"))
            {
                limb.limbName = limbTable.getAttr("limb");
                if (limbTable.haveAttr("helper"))
                {
                    if (limb.pHelper = m_pVehicle->GetHelper(limbTable.getAttr("helper")))
                    {
                        m_ikLimbs.push_back(limb);
                    }
                }
            }
        }
    }

    return !m_ikLimbs.empty();
}

//------------------------------------------------------------------------
void CVehicleSeatActionPassengerIK::Reset()
{
}

//------------------------------------------------------------------------
void CVehicleSeatActionPassengerIK::StartUsing(EntityId passengerId)
{
    m_passengerId = passengerId;

    m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_Visible);
}

//------------------------------------------------------------------------
void CVehicleSeatActionPassengerIK::StopUsing()
{
    m_passengerId = 0;

    m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);
}

//------------------------------------------------------------------------
void CVehicleSeatActionPassengerIK::Update(float frameTime)
{
    if (!m_passengerId)
    {
        return;
    }

    IActor* pActor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(m_passengerId);
    if (!pActor)
    {
        CRY_ASSERT(!"Invalid entity id for the actor id, Actor System didn't know it");
        return;
    }

    if (ICharacterInstance* pCharInstance = pActor->GetEntity()->GetCharacter(0))
    {
        ISkeletonAnim* pSkeletonAnim = pCharInstance->GetISkeletonAnim();
        CRY_ASSERT(pSkeletonAnim);

        if (pSkeletonAnim->GetNumAnimsInFIFO(0) >= 1)
        {
            const CAnimation& anim = pSkeletonAnim->GetAnimFromFIFO(0, 0);
            const uint16 loopCount = anim.GetLoop();
            const f32 animationTime = pSkeletonAnim->GetAnimationNormalizedTime(&anim);
            if (!m_waitShortlyBeforeStarting || (loopCount > 0 || animationTime > 0.9f))
            {
                for (TIKLimbVector::iterator ite = m_ikLimbs.begin(); ite != m_ikLimbs.end(); ++ite)
                {
                    const SIKLimb& limb = *ite;

                    Vec3 helperPos = limb.pHelper->GetWorldSpaceTranslation();
                    pActor->SetIKPos(limb.limbName.c_str(), helperPos, 1);
                }
            }
        }
    }
}

void CVehicleSeatActionPassengerIK::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
    s->AddObject(m_ikLimbs);
}

DEFINE_VEHICLEOBJECT(CVehicleSeatActionPassengerIK);
