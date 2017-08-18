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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_AIREINFORCEMENTSPOT_H
#define CRYINCLUDE_EDITOR_OBJECTS_AIREINFORCEMENTSPOT_H
#pragma once


#include "EntityObject.h"

// forward declaration.
struct IAIObject;

/*!
*   CAIReinforcementSpot is an special tag point, that is registered with AI, and tell him what to do when AI calling reinforcements.
*
*/
class CAIReinforcementSpot
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void InitVariables() {};
    virtual void Done();
    virtual void Display(DisplayContext& disp);
    virtual bool HitTest(HitContext& hc);
    virtual void GetLocalBounds(AABB& box);
    bool IsScalable() { return false; }
    virtual void SetHelperScale(float scale) { m_helperScale = scale; };
    virtual float GetHelperScale() { return m_helperScale; };
    //////////////////////////////////////////////////////////////////////////

protected:
    friend class CTemplateObjectClassDesc<CAIReinforcementSpot>;

    //! Dtor must be protected.
    CAIReinforcementSpot();

    static const GUID& GetClassID()
    {
        // {81C0EF77-4C60-40c7-98B2-C66A0725ACA0}
        static const GUID guid = {
            0x81c0ef77, 0x4c60, 0x40c7, { 0x98, 0xb2, 0xc6, 0x6a, 0x7, 0x25, 0xac, 0xa0 }
        };
        return guid;
    };

    float GetRadius();

    void DeleteThis() { delete this; };

    static float m_helperScale;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_AIREINFORCEMENTSPOT_H
