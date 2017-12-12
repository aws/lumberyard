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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_CLOUDOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_CLOUDOBJECT_H
#pragma once


#include "BaseObject.h"


class CCloudObject
    : public CBaseObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    void Display(DisplayContext& dc);
    void GetLocalBounds(AABB& box);
    bool HitTest(HitContext& hc);

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    //////////////////////////////////////////////////////////////////////////
    // Cloud parameters.
    //////////////////////////////////////////////////////////////////////////
    CVariable<int> mv_spriteRow;
    CVariable<float> m_width;
    CVariable<float> m_height;
    CVariable<float> m_length;
    CVariable<float> m_angleVariations;
    CVariable<float> mv_sizeSprites;
    CVariable<float> mv_randomSizeValue;

    static const GUID& GetClassID()
    {
        // {ea981e84-16e5-4e78-8058-ece7dc66b1b7}
        static const GUID guid = {
            0xea981e84, 0x16e5, 0x4e78, { 0x80, 0x58, 0xec, 0xe7, 0xdc, 0x66, 0xb1, 0xb7 }
        };
        return guid;
    }

    CCloudObject();

protected:
    void DeleteThis() { delete this; };
    void OnSizeChange(IVariable* pVar);

    bool m_bNotSharedGeom;
    bool m_bIgnoreNodeUpdate;
};


#endif // CRYINCLUDE_EDITOR_OBJECTS_CLOUDOBJECT_H
