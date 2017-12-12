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

// Description : TagPoint object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_VOLUME_H
#define CRYINCLUDE_EDITOR_OBJECTS_VOLUME_H
#pragma once


#include "BaseObject.h"

/*!
 *  CVolume is an spherical or box volume in space.
 *
 */
class CVolume
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
    bool IsScalable() { return false; }
    bool IsRotatable() { return false; }
    void GetLocalBounds(AABB& box);
    bool HitTest(HitContext& hc);
    bool HasMeasurementAxis() const {   return false;   }

    void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    //////////////////////////////////////////////////////////////////////////


    CVolume();

    static const GUID& GetClassID()
    {
        // {3141824B-6455-417b-B9A0-76CF1A672369}
        static const GUID guid = {
            0x3141824b, 0x6455, 0x417b, { 0xb9, 0xa0, 0x76, 0xcf, 0x1a, 0x67, 0x23, 0x69 }
        };
        return guid;
    }

protected:
    //! Called when one of size parameters change.
    void OnSizeChange(IVariable* var);

    void DeleteThis() { delete this; };

    //! Can be either sphere or box.
    bool m_sphere;

    CVariable<float> mv_width;
    CVariable<float> mv_length;
    CVariable<float> mv_height;
    CVariable<float> mv_viewDistance;
    CVariable<QString> mv_shader;
    CVariable<Vec3> mv_fogColor;

    //! Local volume space bounding box.
    AABB m_box;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_VOLUME_H
