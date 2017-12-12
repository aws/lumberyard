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

// Description : Vehicle Helper Object

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEHELPEROBJECT_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEHELPEROBJECT_H
#pragma once


#include "Objects/BaseObject.h"
#include "VehicleDialogComponent.h"

class CVehiclePart;
class CVehiclePrototype;

/*!
 *  CVehicleHelper is a simple helper object for specifying a position and orientation
 *  in a vehicle part coordinate frame
 *
 */
class CVehicleHelper
    : public CBaseObject
    , public CVeedObject
{
    Q_OBJECT
public:
    ~CVehicleHelper();
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Done();

    void Display(DisplayContext& dc);
    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    void GetBoundSphere(Vec3& pos, float& radius);
    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& box);
    bool HitTest(HitContext& hc);

    void Serialize(CObjectArchive& ar){}
    //////////////////////////////////////////////////////////////////////////

    // Ovverides from IVeedObject.
    //////////////////////////////////////////////////////////////////////////
    void UpdateVarFromObject();
    void UpdateObjectFromVar();

    const char* GetElementName(){ return "Helper"; }
    virtual int GetIconIndex(){ return (IsFromGeometry() ? VEED_ASSET_HELPER_ICON : VEED_HELPER_ICON); }

    virtual IVariable* GetVariable(){ return m_pVar; }
    virtual void SetVariable(IVariable* pVar);
    void UpdateScale(float scale);
    //////////////////////////////////////////////////////////////////////////

    void SetIsFromGeometry(bool b);
    bool IsFromGeometry(){ return m_fromGeometry; }

    void SetVehicle(CVehiclePrototype* pProt){ m_pVehicle = pProt; }



protected:
    friend class CTemplateObjectClassDesc<CVehicleHelper>;

    //! Dtor must be protected.
    CVehicleHelper();
    void DeleteThis() { delete this; };

    static const GUID& GetClassID()
    {
        // {13A9F32C-57B9-49f8-A877-AFDB4A3271A9}
        static const GUID guid = {
            0x13a9f32c, 0x57b9, 0x49f8, { 0xa8, 0x77, 0xaf, 0xdb, 0x4a, 0x32, 0x71, 0xa9 }
        };
        return guid;
    }

    void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);

    virtual void OnTransform();

    float m_innerRadius;
    float m_outerRadius;

    IVariable* m_pVar;

    IEditor* m_ie;

    bool m_fromGeometry;

    CVehiclePrototype* m_pVehicle;
};

#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEHELPEROBJECT_H
