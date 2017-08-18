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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPROTOTYPE_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPROTOTYPE_H
#pragma once



#include "Objects/EntityObject.h"
#include "VehicleDialogComponent.h"


struct IVehicleData;
class CVehicleEditorDialog;
class CVehiclePart;
class CVehicleComponent;
class CVehicleHelper;

struct IVehiclePrototype
{
    virtual IVehicleData* GetVehicleData() = 0;
};

/*!
*   CVehiclePrototype represents an editable Vehicle.
*
*/
class CVehiclePrototype
    : public CBaseObject
    , public CVeedObject
    , public IVehiclePrototype
{
    Q_OBJECT
public:

    //////////////////////////////////////////////////////////////////////////
    // Overwrites from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void InitVariables() {};
    void Display(DisplayContext& disp);
    void Done();

    bool HitTest(HitContext& hc);

    void GetLocalBounds(AABB& box);
    void GetBoundBox(AABB& box);

    void AttachChild(CBaseObject* child, bool bKeepPos = true);
    void RemoveChild(CBaseObject* node);

    void Serialize(CObjectArchive& ar);
    //////////////////////////////////////////////////////////////////////////

    // IVeedObject
    //////////////////////////////////////////////////////////////////////////
    void UpdateVarFromObject(){}
    void UpdateObjectFromVar(){}

    const char* GetElementName(){ return "Vehicle"; }
    IVariable* GetVariable();

    virtual int GetIconIndex(){ return VEED_VEHICLE_ICON; }
    //////////////////////////////////////////////////////////////////////////

    void SetVehicleEntity(CEntityObject* pEnt);
    CEntityObject* GetCEntity(){ return m_vehicleEntity; }

    bool hasEntity(){ return m_vehicleEntity != 0; }
    bool ReloadEntity();

    IVehicleData* GetVehicleData();
    void ApplyClonedData();

    void AddPart(CVehiclePart* pPart);
    void AddComponent(CVehicleComponent* pComp);

    void AddHelper(CVehicleHelper* pHelper, IVariable* pHelperVar = 0);
    const CVehicleHelper* GetHelper(const QString& name) const;

    void OnObjectEvent(CBaseObject* node, int event);

protected:
    friend class CTemplateObjectClassDesc<CVehiclePrototype>;

    CVehiclePrototype();
    void DeleteThis();

    static const GUID& GetClassID()
    {
        // {559A1504-0AF2-47f2-A3A9-DFDF5720E232}
        static const GUID guid = {
            0x559a1504, 0xaf2, 0x47f2, { 0xa3, 0xa9, 0xdf, 0xdf, 0x57, 0x20, 0xe2, 0x32 }
        };
        return guid;
    }

    CEntityObject* m_vehicleEntity;

    // root of vehicle data tree
    IVehicleData* m_pVehicleData;
    IVariablePtr m_pClone;

    QString m_name;

    friend class CVehicleEditorDialog;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPROTOTYPE_H
