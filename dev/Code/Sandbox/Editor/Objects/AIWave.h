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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_AIWAVE_H
#define CRYINCLUDE_EDITOR_OBJECTS_AIWAVE_H
#pragma once


#include "EntityObject.h"


class CAIWaveObject
    : public CEntityObject
{
    Q_OBJECT
public:
    virtual void InitVariables() {}

    void BeginEditParams(IEditor* ie, int flags);
    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();

    void SetName(const QString& newName);

    static const GUID& GetClassID()
    {
        // {10E57056-78C7-489e-B230-89B673196DE5}
        static const GUID guid = {
            0x10e57056, 0x78c7, 0x489e, { 0xb2, 0x30, 0x89, 0xb6, 0x73, 0x19, 0x6d, 0xe5 }
        };
        return guid;
    };

    CAIWaveObject();
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_AIWAVE_H
