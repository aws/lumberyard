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

#ifndef CRYINCLUDE_EDITOR_ENTITYPROTOTYPEMANAGER_H
#define CRYINCLUDE_EDITOR_ENTITYPROTOTYPEMANAGER_H
#pragma once

#include "BaseLibraryManager.h"

class CEntityPrototype;
class CEntityPrototypeLibrary;

/** Manages all entity prototypes and prototype libraries.
*/
class CRYEDIT_API CEntityPrototypeManager
    : public CBaseLibraryManager
{
public:
    CEntityPrototypeManager();
    ~CEntityPrototypeManager();

    void ClearAll();

    //! Find prototype by fully specified prototype name.
    //! Name is given in this form: Library.Group.ItemName (ex. AI.Cover.Merc)
    CEntityPrototype* FindPrototype(REFGUID guid) const { return (CEntityPrototype*)FindItem(guid); }

    //! Loads a new entity archetype from a xml node.
    CEntityPrototype* LoadPrototype(CEntityPrototypeLibrary* pLibrary, XmlNodeRef& node);

    void ExportPrototypes(const QString& path, const QString& levelName, CPakFile& levelPakFile);

private:
    virtual CBaseLibraryItem* MakeNewItem();
    virtual CBaseLibrary* MakeNewLibrary();
    //! Root node where this library will be saved.
    virtual QString GetRootNodeName();
    //! Path to libraries in this manager.
    virtual QString GetLibsPath();

    QString m_libsPath;
};

#endif // CRYINCLUDE_EDITOR_ENTITYPROTOTYPEMANAGER_H
