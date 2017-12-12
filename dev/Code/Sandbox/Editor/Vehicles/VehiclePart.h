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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPART_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPART_H
#pragma once


#include <list>

#include "Objects/EntityObject.h"
#include "Util/Variable.h"

class CVehicleHelper;
class CVehicleSeat;
class CVehiclePrototype;

#include "VehicleDialogComponent.h"

#define PARTCLASS_WHEEL "SubPartWheel"
#define PARTCLASS_MASS  "MassBox"
#define PARTCLASS_LIGHT "Light"
#define PARTCLASS_TREAD "Tread"

typedef std::list<CVehicleHelper*> THelperList;

/*!
*   CVehiclePart represents an editable vehicle part.
*
*/
class CVehiclePart
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
    void RemoveChild(CBaseObject* node);

    void Serialize(CObjectArchive& ar){}
    /////////////////////////////////////////////////////////////////////////

    void AddPart(CVehiclePart* pPart);
    void SetMainPart(bool bMain){ m_isMain = bMain; }
    bool IsMainPart() const { return m_isMain; }
    QString GetPartClass();

    //! returns whether this part is a leaf part (false means it can have children)
    bool IsLeaf();

    void SetVariable(IVariable* pVar);
    IVariable* GetVariable(){ return m_pVar; }

    // Overrides from IVeedObject
    /////////////////////////////////////////////////////////////////////////
    void UpdateVarFromObject();
    void UpdateObjectFromVar();

    const char* GetElementName(){ return "Part"; }
    int GetIconIndex();
    void UpdateScale(float scale);
    void OnTreeSelection();
    /////////////////////////////////////////////////////////////////////////

    void SetVehicle(CVehiclePrototype* pProt){ m_pVehicle = pProt; }

    void OnObjectEvent(CBaseObject* node, int event);

protected:
    friend class CTemplateObjectClassDesc<CVehiclePart>;

    CVehiclePart();
    void DeleteThis() { delete this; };

    static const GUID& GetClassID()
    {
        // {C8ABFCCC-104E-473a-AB0F-E946E2D00F1C}
        static const GUID guid = {
            0xc8abfccc, 0x104e, 0x473a, { 0xab, 0xf, 0xe9, 0x46, 0xe2, 0xd0, 0xf, 0x1c }
        };
        return guid;
    }

    void UpdateFromVar();

    void OnSetClass(IVariable* pVar);
    void OnSetPos(IVariable* pVar);

    void DrawRotationLimits(DisplayContext& dc, IVariable* pSpeed, IVariable* pLimits, IVariable* pHelper, int axis);

    CVehiclePrototype* m_pVehicle;

    bool m_isMain;
    IVariablePtr m_pVar;

    // pointer for saving per-frame lookups
    IVariable* m_pYawSpeed;
    IVariable* m_pYawLimits;
    IVariable* m_pPitchSpeed;
    IVariable* m_pPitchLimits;
    IVariable* m_pHelper;
    IVariable* m_pPartClass;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEPART_H
