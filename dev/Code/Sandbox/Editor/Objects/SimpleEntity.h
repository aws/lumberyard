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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_SIMPLEENTITY_H
#define CRYINCLUDE_EDITOR_OBJECTS_SIMPLEENTITY_H
#pragma once


#include "EntityObject.h"

class CSimpleEntity
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    bool ConvertFromObject(CBaseObject* object);

    void BeginEditParams(IEditor* ie, int flags);

    void Validate(CErrorReport* report);
    bool IsSimilarObject(CBaseObject* pObject);

    QString GetGeometryFile() const;
    void SetGeometryFile(const QString& filename);

    static const GUID& GetClassID()
    {
        // {F7820713-E4EE-44b9-867C-C0F9543B4871}
        static const GUID guid = {
            0xf7820713, 0xe4ee, 0x44b9, { 0x86, 0x7c, 0xc0, 0xf9, 0x54, 0x3b, 0x48, 0x71 }
        };
        return guid;
    }

private:
    void OnFileChange(QString filename);
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_SIMPLEENTITY_H
