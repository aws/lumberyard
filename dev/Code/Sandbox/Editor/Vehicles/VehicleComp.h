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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLECOMP_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLECOMP_H
#pragma once


#include "Objects/BaseObject.h"
#include "Util/Variable.h"

class CVehicleHelper;
class CVehiclePart;
class CVehiclePrototype;


#include "VehicleDialogComponent.h"


/*!
*   CVehicleComponent represents an editable vehicle Component
*
*/
class CVehicleComponent
    : public CBaseObject
    , public CVeedObject
{
    Q_OBJECT
public:

    //////////////////////////////////////////////////////////////////////////
    // Overwrites from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    virtual void Done();
    virtual void InitVariables() {}
    virtual void Display(DisplayContext& dc);

    virtual bool HitTest(HitContext& hc);

    virtual void GetLocalBounds(AABB& box);
    virtual void GetBoundBox(AABB& box);

    virtual void Serialize(CObjectArchive& ar) {}
    /////////////////////////////////////////////////////////////////////////

    // Overrides from IVeedObject
    /////////////////////////////////////////////////////////////////////////
    virtual void UpdateVarFromObject();
    virtual void UpdateObjectFromVar();

    virtual const char* GetElementName() { return "Component"; }
    virtual int GetIconIndex();

    virtual void SetVariable(IVariable* pVar);
    virtual void UpdateScale(float scale);
    /////////////////////////////////////////////////////////////////////////

    void CreateVariable();
    void ResetPosition();

    void SetVehicle(CVehiclePrototype* pProt) { m_pVehicle = pProt; }

protected:
    friend class CTemplateObjectClassDesc<CVehicleComponent>;
    CVehicleComponent();
    virtual void DeleteThis() { delete this; }
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);

    static const GUID& GetClassID()
    {
        // {680D0873-CC5F-48da-B88F-E8F95D666D81}
        static const GUID guid = {
            0x680d0873, 0xcc5f, 0x48da, { 0xb8, 0x8f, 0xe8, 0xf9, 0x5d, 0x66, 0x6d, 0x81 }
        };
        return guid;
    }

    virtual void OnTransform();

private:
    void UpdateObjectNameFromVar();
    void UpdateObjectBoundsFromVar();

    void UpdateVarNameFromObject();
    void UpdateVarBoundsFromObject();

    bool GetUseBoundsFromParts() const;

    bool HasZeroSize() const;

private:
    CVehiclePrototype* m_pVehicle;

    IVariablePtr m_pPosition;
    IVariablePtr m_pSize;
    IVariablePtr m_pUseBoundsFromParts;
};
	
#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLECOMP_H
