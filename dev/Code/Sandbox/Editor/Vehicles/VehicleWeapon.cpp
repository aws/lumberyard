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

// Description : Vehicle Weapon Object implementation


#include "StdAfx.h"
#include "VehicleWeapon.h"

#include "VehicleData.h"
#include "VehiclePrototype.h"
#include "VehiclePart.h"
#include "VehicleSeat.h"


//////////////////////////////////////////////////////////////////////////
CVehicleWeapon::CVehicleWeapon()
{
    m_pVar = 0;
    m_pSeat = 0;
    m_pVehicle = 0;
}

//////////////////////////////////////////////////////////////////////////
void CVehicleWeapon::UpdateVarFromObject()
{
    if (!m_pVar || !m_pVehicle)
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehicleWeapon::Done()
{
    VeedLog("[CVehicleWeapon:Done] <%s>", GetName().toUtf8().constData());
    CBaseObject::Done();
}



//////////////////////////////////////////////////////////////////////////
void CVehicleWeapon::BeginEditParams(IEditor* ie, int flags)
{
}


//////////////////////////////////////////////////////////////////////////
bool CVehicleWeapon::HitTest(HitContext& hc)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CVehicleWeapon::Display(DisplayContext& dc)
{
    // todo: draw at mount helper, add rotation limits from parts

    DrawDefault(dc);
}


//////////////////////////////////////////////////////////////////////////
void CVehicleWeapon::GetBoundBox(AABB& box)
{
    // Transform local bounding box into world space.
    GetLocalBounds(box);
    box.SetTransformedAABB(GetWorldTM(), box);
}

//////////////////////////////////////////////////////////////////////////
void CVehicleWeapon::GetLocalBounds(AABB& box)
{
    // todo
    // return local bounds
}

#include <Vehicles/VehicleWeapon.moc>