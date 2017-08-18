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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEWEAPON_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEWEAPON_H
#pragma once


#include "Objects/BaseObject.h"
#include "VehicleDialogComponent.h"

class CVehiclePart;
class CVehicleSeat;
class CVehiclePrototype;

/*!
 *  CVehicleWeapon represents a mounted vehicle weapon and is supposed
 *  to be used as children of a VehicleSeat.
 */
class CVehicleWeapon
    : public CBaseObject
    , public CVeedObject
{
    Q_OBJECT
public:
    ~CVehicleWeapon(){}

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    void Done();

    void Display(DisplayContext& dc);
    void BeginEditParams(IEditor* ie, int flags);

    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& box);
    bool HitTest(HitContext& hc);
    void Serialize(CObjectArchive& ar){}
    //////////////////////////////////////////////////////////////////////////

    // Ovverides from IVeedObject.
    //////////////////////////////////////////////////////////////////////////
    void UpdateVarFromObject();
    void UpdateObjectFromVar(){}

    const char* GetElementName(){ return "Weapon"; }
    virtual int GetIconIndex(){ return VEED_WEAPON_ICON; }

    void SetVariable(IVariable* pVar){ m_pVar = pVar; }
    //////////////////////////////////////////////////////////////////////////

    void SetVehicle(CVehiclePrototype* pProt){ m_pVehicle = pProt; }

protected:
    friend class CTemplateObjectClassDesc<CVehicleWeapon>;
    CVehicleWeapon();
    void DeleteThis() { delete this; };

    static const GUID& GetClassID()
    {
        // {93296AA8-14EC-4738-BD67-19FF83532C4F}
        static const GUID guid = {
            0x93296aa8, 0x14ec, 0x4738, { 0xbd, 0x67, 0x19, 0xff, 0x83, 0x53, 0x2c, 0x4f }
        };
        return guid;
    }
    CVehicleSeat* m_pSeat;
    CVehiclePrototype* m_pVehicle;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEWEAPON_H
