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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_GEOMENTITY_H
#define CRYINCLUDE_EDITOR_OBJECTS_GEOMENTITY_H
#pragma once


#include "EntityObject.h"

//////////////////////////////////////////////////////////////////////////
// Geometry entity.
//////////////////////////////////////////////////////////////////////////
class CGeomEntity
    : public CEntityObject
{
    Q_OBJECT
public:
    static const GUID& GetClassID()
    {
        // {A23DD1AD-7D93-4850-8EE8-BC37CCE0FAC7}
        static const GUID guid = {
            0xa23dd1ad, 0x7d93, 0x4850, { 0x8e, 0xe8, 0xbc, 0x37, 0xcc, 0xe0, 0xfa, 0xc7 }
        };
        return guid;
    }

    //////////////////////////////////////////////////////////////////////////
    // CEntity
    //////////////////////////////////////////////////////////////////////////
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void InitVariables();
    virtual void SpawnEntity();
    virtual bool ConvertFromObject(CBaseObject* object);
    virtual bool HasMeasurementAxis() const {   return true;    }

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    virtual void Validate(IErrorReport* report) override;
    virtual bool IsSimilarObject(CBaseObject* pObject);
    virtual void OnEvent(ObjectEvent event);
    //////////////////////////////////////////////////////////////////////////

    QString GetGeometryFile() const;
    void SetGeometryFile(const QString& filename);

    void GetVerticesInWorld(std::vector<Vec3>& vertices) const;

protected:
    void OnFileChange(QString filename);
    void OnGeometryFileChange(IVariable* pVar);

    CVariable<QString> mv_geometry;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_GEOMENTITY_H
