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

// Description : Entity character attachment helper object.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_CHARATTACHHELPER_H
#define CRYINCLUDE_EDITOR_OBJECTS_CHARATTACHHELPER_H
#pragma once


#include "EntityObject.h"

//////////////////////////////////////////////////////////////////////////
class CRYEDIT_API CCharacterAttachHelperObject
    : public CEntityObject
{
    Q_OBJECT
public:
    CCharacterAttachHelperObject();

    static const GUID& GetClassID()
    {
        // {E7F69317-3179-488b-ACC0-69994E0C0CF2}
        static const GUID guid = {
            0xe7f69317, 0x3179, 0x488b, { 0xac, 0xc0, 0x69, 0x99, 0x4e, 0xc, 0xc, 0xf2 }
        };
        return guid;
    }

    //////////////////////////////////////////////////////////////////////////
    // CEntity
    //////////////////////////////////////////////////////////////////////////
    virtual void InitVariables() {};
    virtual void Display(DisplayContext& disp);

    virtual void SetHelperScale(float scale);
    virtual float GetHelperScale() { return m_charAttachHelperScale; };
    //////////////////////////////////////////////////////////////////////////
private:
    static float m_charAttachHelperScale;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_CHARATTACHHELPER_H
