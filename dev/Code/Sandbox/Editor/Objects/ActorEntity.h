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
#ifndef CRYINCLUDE_EDITOR_OBJECTS_ACTORENTITY_H
#define CRYINCLUDE_EDITOR_OBJECTS_ACTORENTITY_H
#pragma once

#include "EntityObject.h"

//////////////////////////////////////////////////////////////////////////
// Prototype entity.
//////////////////////////////////////////////////////////////////////////
class CActorEntity
    : public CEntityObject
{
    Q_OBJECT
public:
    static const GUID& GetClassID()
    {
        // {5529607A-C374-4588-AD41-D68FB26A1E8C}
        static const GUID guid =
        {
            0x5529607a, 0xc374, 0x4588, { 0xad, 0x41, 0xd6, 0x8f, 0xb2, 0x6a, 0x1e, 0x8c }
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
    virtual bool HasMeasurementAxis() const { return true; }
    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams(IEditor* ie);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    virtual void Validate(CErrorReport* report);
    virtual bool IsSimilarObject(CBaseObject* pObject);
    virtual void OnEvent(ObjectEvent event);
    //////////////////////////////////////////////////////////////////////////

    QString GetPrototypeFile() const;
    void SetPrototypeFile(const QString& filename);

    void GetVerticesInWorld(std::vector<Vec3>& vertices) const;

protected:
    void OnFileChange(QString filename);
    void OnPrototypeFileChange(IVariable* pVar);

    CVariable<QString> mv_prototype;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_ACTORENTITY_H