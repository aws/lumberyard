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

#include "CryLegacy_precompiled.h"
#include "IVehicleSystem.h"
#include "Vehicle.h"
#include "VehicleSeat.h"
#include "VehicleSeatActionOrientatePartToView.h"

CVehicleSeatActionOrientatePartToView::CVehicleSeatActionOrientatePartToView()
    : m_pSeat(NULL)
    , m_pVehicle(NULL)
{
}

//------------------------------------------------------------------------
bool CVehicleSeatActionOrientatePartToView::Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table)
{
    m_pVehicle = pVehicle;
    m_pSeat = pSeat;

    CVehicleParams partsTable = table.findChild("Parts");

    if (partsTable)
    {
        const int partCount = partsTable.getChildCount();

        m_controlledParts.reserve(partCount);

        for (int partIdx = 0; partIdx < partCount; ++partIdx)
        {
            CVehicleParams controlledPart = partsTable.getChild(partIdx);

            if (!strcmp(controlledPart.getTag(), "Part"))
            {
                const char* controlledPartName = controlledPart.getAttr("name");

                IVehiclePart* pControlledPart = m_pVehicle->GetPart(controlledPartName);
                if (pControlledPart)
                {
                    SOrientatePartInfo orientationPartInfo;
                    orientationPartInfo.partIndex = pControlledPart->GetIndex();
                    controlledPart.getAttr("orientationAxis", orientationPartInfo.orientationAxis);

                    m_controlledParts.push_back(orientationPartInfo);

                    pControlledPart->SetMoveable();
                }
            }
        }
    }

    return true;
}

void CVehicleSeatActionOrientatePartToView::StartUsing(EntityId passengerId)
{
    m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_AlwaysUpdate);
}

void CVehicleSeatActionOrientatePartToView::StopUsing()
{
    m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);
}

void CVehicleSeatActionOrientatePartToView::Update(const float deltaTime)
{
    const Vec3 viewDirection = GetViewDirection();

    for (TPartIndices::const_iterator partCit = m_controlledParts.begin(); partCit != m_controlledParts.end(); ++partCit)
    {
        IVehiclePart* pPart = m_pVehicle->GetPart((unsigned int)partCit->partIndex);
        IVehiclePart* pParentPart = pPart ? pPart->GetParent() : NULL;
        if (pPart && pParentPart)
        {
            const Matrix34& baseTM = pPart->GetLocalBaseTM();
            Ang3 currentBaseAngles(baseTM);
            const Vec3 orientationAxis = partCit->orientationAxis;
            const Vec3 localViewDirection = pParentPart->GetWorldTM().GetInverted().TransformVector(viewDirection);

            Ang3 newBaseAngles(Quat::CreateRotationVDir(localViewDirection, 0.0f));
            newBaseAngles.x = newBaseAngles.x * orientationAxis.x;
            newBaseAngles.y = newBaseAngles.y * orientationAxis.y;
            newBaseAngles.z = newBaseAngles.z * orientationAxis.z;

            Interpolate(currentBaseAngles, newBaseAngles, 5.0f, deltaTime, 0.01f);

            Matrix34 newBaseTM;
            newBaseTM.SetRotationXYZ(newBaseAngles);
            newBaseTM.SetTranslation(baseTM.GetTranslation());
            pPart->SetLocalBaseTM(newBaseTM);

            //const Vec3 partPos = pPart->GetWorldTM().GetTranslation();
            //gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(partPos, ColorB(0,255,0), partPos + (viewDirection * 3.0f), ColorB(0, 255, 0), 3.0f);
        }
    }
}

Vec3 CVehicleSeatActionOrientatePartToView::GetViewDirection() const
{
    IActor* pPassenger = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_pSeat->GetPassenger());
    IMovementController* pPassengerMC = pPassenger ? pPassenger->GetMovementController() : NULL;

    if (pPassengerMC)
    {
        SMovementState movementState;
        pPassengerMC->GetMovementState(movementState);

        return movementState.aimDirection;
    }

    return m_pVehicle->GetEntity()->GetWorldTM().GetColumn1();
}

void CVehicleSeatActionOrientatePartToView::GetMemoryUsage(ICrySizer* s) const
{
    s->AddObject(this, sizeof(*this));
    s->AddContainer(m_controlledParts);
}

DEFINE_VEHICLEOBJECT(CVehicleSeatActionOrientatePartToView);
