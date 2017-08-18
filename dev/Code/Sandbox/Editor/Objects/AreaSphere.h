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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_AREASPHERE_H
#define CRYINCLUDE_EDITOR_OBJECTS_AREASPHERE_H
#pragma once


#include "EntityObject.h"
#include "PickEntitiesPanel.h"
#include "AreaBox.h"

/*!
 *  CAreaSphere is a sphere volume in space where entites can be attached to.
 *
 */
class CAreaSphere
    : public CAreaObjectBase
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool CreateGameObject();
    virtual void InitVariables();
    void Display(DisplayContext& dc);
    bool IsScalable() { return false; }
    bool IsRotatable() { return false; }
    void GetLocalBounds(AABB& box);
    bool HitTest(HitContext& hc);

    virtual void PostLoad(CObjectArchive& ar);

    void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    void SetAreaId(int nAreaId);
    int GetAreaId();
    void SetRadius(float fRadius);
    float GetRadius();

    void UpdateGameArea();

protected:
    friend class CTemplateObjectClassDesc<CAreaSphere>;

    //! Dtor must be protected.
    CAreaSphere();

    static const GUID& GetClassID()
    {
        // {640A1105-A421-4ed3-9E71-7783F091AA27}
        static const GUID guid = {
            0x640a1105, 0xa421, 0x4ed3, { 0x9e, 0x71, 0x77, 0x83, 0xf0, 0x91, 0xaa, 0x27 }
        };
        return guid;
    }

    void DeleteThis() { delete this; };

    void OnAreaChange(IVariable* pVar) override;
    void OnSizeChange(IVariable* pVar);

    //! AreaId
    CVariable<int> m_areaId;
    //! EdgeWidth
    CVariable<float> m_edgeWidth;
    //! Local volume space radius.
    CVariable<float> m_radius;
    CVariable<int> mv_groupId;
    CVariable<int> mv_priority;
    CVariable<bool> mv_filled;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_AREASPHERE_H
