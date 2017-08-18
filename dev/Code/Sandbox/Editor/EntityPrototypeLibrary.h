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

#ifndef CRYINCLUDE_EDITOR_ENTITYPROTOTYPELIBRARY_H
#define CRYINCLUDE_EDITOR_ENTITYPROTOTYPELIBRARY_H
#pragma once

#include "BaseLibrary.h"

/** Library of prototypes.
*/
class CRYEDIT_API CEntityPrototypeLibrary
    : public CBaseLibrary
{
public:
    CEntityPrototypeLibrary(IBaseLibraryManager* pManager)
        : CBaseLibrary(pManager) {};
    virtual bool Save();
    virtual bool Load(const QString& filename);
    virtual void Serialize(XmlNodeRef& node, bool bLoading);

private:
};

#endif // CRYINCLUDE_EDITOR_ENTITYPROTOTYPELIBRARY_H
