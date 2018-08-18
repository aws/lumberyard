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
#include "VehicleHelper.h"
#include "CryAction.h"
#include "PersistentDebug.h"

//------------------------------------------------------------------------
void CVehicleHelper::GetVehicleTM(Matrix34& vehicleTM, bool forced) const
{
    vehicleTM = m_localTM;

    IVehiclePart* pParent = m_pParentPart;
    while (pParent)
    {
        vehicleTM = pParent->GetLocalTM(true, forced) * vehicleTM;
        pParent = pParent->GetParent();
    }
}

//------------------------------------------------------------------------
void CVehicleHelper::GetWorldTM(Matrix34& worldTM) const
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

    const Matrix34& partWorldTM = m_pParentPart->GetWorldTM();

    worldTM = Matrix34(Matrix33(partWorldTM) * Matrix33(m_localTM));
    worldTM.SetTranslation((partWorldTM * m_localTM).GetTranslation());
}

//------------------------------------------------------------------------
void CVehicleHelper::GetReflectedWorldTM(Matrix34& reflectedWorldTM) const
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

    Matrix34 tempMatrix = m_localTM;
    tempMatrix.m03 = -tempMatrix.m03;   // negate x coord of translation

    const Matrix34& partWorldTM = m_pParentPart->GetWorldTM();

    reflectedWorldTM = Matrix34(Matrix33(partWorldTM) * Matrix33(tempMatrix));
    reflectedWorldTM.SetTranslation((partWorldTM * tempMatrix).GetTranslation());
}

//------------------------------------------------------------------------
Vec3 CVehicleHelper::GetLocalSpaceTranslation() const
{
    return m_localTM.GetTranslation();
}

//------------------------------------------------------------------------
Vec3 CVehicleHelper::GetVehicleSpaceTranslation() const
{
    Matrix34 temp;
    GetVehicleTM(temp);
    return temp.GetTranslation();
}

//------------------------------------------------------------------------
Vec3 CVehicleHelper::GetWorldSpaceTranslation() const
{
    Matrix34 temp;
    GetWorldTM(temp);
    return temp.GetTranslation();
}

//------------------------------------------------------------------------
IVehiclePart* CVehicleHelper::GetParentPart() const
{
    return m_pParentPart;
}


