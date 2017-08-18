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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLESEAT_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLESEAT_H
#pragma once


#include <list>

#include "Objects/EntityObject.h"
#include "Util/Variable.h"


class CVehiclePrototype;
class CVehicleHelper;
class CVehiclePart;
class CVehicleWeapon;

#include "VehicleDialogComponent.h"

typedef enum
{
    WEAPON_PRIMARY,
    WEAPON_SECONDARY
} eWeaponType;

/*!
*   CVehicleSeat represents an editable vehicle seat.
*
*/
class CVehicleSeat
    : public CBaseObject
    , public CVeedObject
{
    Q_OBJECT
public:

    //////////////////////////////////////////////////////////////////////////
    // Overwrites from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Done();
    void InitVariables() {};
    void Display(DisplayContext& disp);

    bool HitTest(HitContext& hc);

    void GetLocalBounds(AABB& box);
    void GetBoundBox(AABB& box);

    void AttachChild(CBaseObject* child, bool bKeepPos = true);

    void Serialize(CObjectArchive& ar){}
    /////////////////////////////////////////////////////////////////////////


    // Overrides from IVeedObject
    /////////////////////////////////////////////////////////////////////////
    IVariable* GetVariable(){ return m_pVar; }

    void UpdateVarFromObject();
    void UpdateObjectFromVar();

    const char* GetElementName(){ return "Seat"; }
    virtual int GetIconIndex(){ return VEED_SEAT_ICON; }
    /////////////////////////////////////////////////////////////////////////

    void SetVehicle(CVehiclePrototype* pProt);
    void SetVariable(IVariable* pVar);

    //! Sets/gets the optional part the seat belongs to
    void SetPart(CVehiclePart* pPart){ m_pPart = pPart; }
    CVehiclePart* GetPart(){ return m_pPart; }

    //! Add Weapon to Seat
    void AddWeapon(int weaponType, CVehicleWeapon* pWeap, IVariable* pVar = 0);

    void OnObjectEvent(CBaseObject* node, int event);

    void OnSetPart(IVariable* pVar);

protected:
    friend class CTemplateObjectClassDesc<CVehicleSeat>;

    CVehicleSeat();
    void DeleteThis() { delete this; };

    static const GUID& GetClassID()
    {
        // {7EA47EA4-E49D-43ae-A862-15A5B764F8DC}
        static const GUID guid = {
            0x7ea47ea4, 0xe49d, 0x43ae, { 0xa8, 0x62, 0x15, 0xa5, 0xb7, 0x64, 0xf8, 0xdc }
        };
        return guid;
    }

    void UpdateFromVar();

    CVehiclePrototype* m_pVehicle;
    CVehiclePart* m_pPart;

    IVariable* m_pVar;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLESEAT_H
