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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_AIANCHOR_H
#define CRYINCLUDE_EDITOR_OBJECTS_AIANCHOR_H
#pragma once


#include "EntityObject.h"

// forward declaration.
struct IAIObject;

/*!
 *  CAIAnchor is an special tag point,that registered with AI, and tell him what to do when AI is idle.
 *
 */
class CAIAnchor
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void InitVariables() {}
    virtual void Display(DisplayContext& disp);
    virtual bool HitTest(HitContext& hc);
    virtual void GetLocalBounds(AABB& box);
    virtual void SetScale(const Vec3& scale) {} // Ignore scale
    virtual void SetHelperScale(float scale) { m_helperScale = scale; };
    virtual float GetHelperScale() { return m_helperScale; };
    //////////////////////////////////////////////////////////////////////////

protected:
    friend class CTemplateObjectClassDesc<CAIAnchor>;
    //! Ctor must be protected.
    CAIAnchor();

    static const GUID& GetClassID()
    {
        // {10E57056-78C7-489e-B230-89B673196DE4}
        static const GUID guid = {
            0x10e57056, 0x78c7, 0x489e, { 0xb2, 0x30, 0x89, 0xb6, 0x73, 0x19, 0x6d, 0xe4 }
        };
        return guid;
    };

    float GetRadius();

    void DeleteThis() { delete this; };

    static float m_helperScale;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_AIANCHOR_H
