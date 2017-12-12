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

// Description : Smart Object Helper Object


#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTHELPEROBJECT_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTHELPEROBJECT_H

#pragma once

#include "Objects/BaseObject.h"


class CEntityObject;
class CSmartObjectClass;


/*!
 *  CSmartObjectHelperObject is a simple helper object for specifying a position and orientation
 */
class CSmartObjectHelperObject
    : public CBaseObject
{
    Q_OBJECT
public:

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

    //////////////////////////////////////////////////////////////////////////
    void UpdateVarFromObject();
    void UpdateObjectFromVar()
    {
    }

    //  const char* GetElementName() { return "Helper"; }

    IVariable* GetVariable() { return m_pVar; }
    void SetVariable(IVariable* pVar) { m_pVar = pVar; }
    //////////////////////////////////////////////////////////////////////////

    //  void IsFromGeometry( bool b ) { m_fromGeometry = b; }
    //  bool IsFromGeometry() { return m_fromGeometry; }

    CSmartObjectClass* GetSmartObjectClass() const { return m_pSmartObjectClass; }
    void SetSmartObjectClass(CSmartObjectClass* pClass) { m_pSmartObjectClass = pClass; }

    CEntityObject* GetSmartObjectEntity() const { return m_pEntity; }
    void SetSmartObjectEntity(CEntityObject* pEntity) { m_pEntity = pEntity; }

protected:
    friend class CTemplateObjectClassDesc<CSmartObjectHelperObject>;
    //! Dtor must be protected.
    CSmartObjectHelperObject();
    ~CSmartObjectHelperObject();

    static const GUID& GetClassID()
    {
        // {9c9af214-be12-4213-bf12-83e734ccbd13}
        static const GUID guid = {
            0x9c9af214, 0xbe12, 0x4213, { 0xbf, 0x12, 0x83, 0xe7, 0x34, 0xcc, 0xbd, 0x13 }
        };
        return guid;
    }

    void DeleteThis() { delete this; };

    float m_innerRadius;
    float m_outerRadius;

    IVariable* m_pVar;

    IEditor* m_ie;

    //  bool m_fromGeometry;

    CSmartObjectClass* m_pSmartObjectClass;

    CEntityObject* m_pEntity;
};

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTHELPEROBJECT_H
