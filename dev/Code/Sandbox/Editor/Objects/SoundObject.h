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

// Description : SoundObject object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_SOUNDOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_SOUNDOBJECT_H
#pragma once


#include "BaseObject.h"

/*!
 *  CSoundObject is an object that represent named 3d position in world.
 *
 */
class CSoundObject
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

    //////////////////////////////////////////////////////////////////////////
    virtual void SetName(const QString& name);

    bool IsScalable() const { return false; }
    bool IsRotatable() const { return false; }

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    void GetBoundSphere(Vec3& pos, float& radius);
    bool HitTest(HitContext& hc);

    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    //////////////////////////////////////////////////////////////////////////

public:
    CSoundObject();

    static const GUID& GetClassID()
    {
        // {6B3EDDE5-7BCF-4936-B891-39CD7A8DC021}
        static const GUID guid = {
            0x6b3edde5, 0x7bcf, 0x4936, { 0xb8, 0x91, 0x39, 0xcd, 0x7a, 0x8d, 0xc0, 0x21 }
        };
        return guid;
    }

protected:
    void DeleteThis() { delete this; };

    float m_innerRadius;
    float m_outerRadius;

    IEditor* m_ie;
    //ISoundObject *m_ITag;

    static int m_rollupId;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_SOUNDOBJECT_H
