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

#ifndef CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENMANAGER_H
#define CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENMANAGER_H
#pragma once

#include "BaseLibraryManager.h"

class CGameTokenItem;
class CGameTokenLibrary;

/** Manages Particle libraries and systems.
*/
class CRYEDIT_API CGameTokenManager
    : public CBaseLibraryManager
{
public:
    CGameTokenManager();
    ~CGameTokenManager();

    // Clear all prototypes and material libraries.
    void ClearAll();

    //! Save level game tokens
    void Save();

    //! Load level game tokens
    bool Load();

    //! Export particle systems to game.
    void Export(XmlNodeRef& node);

    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:
    CBaseLibraryItem* MakeNewItem() override;
    CBaseLibrary* MakeNewLibrary() override;

    void SaveAllLibs() override;

    //! Root node where this library will be saved.
    QString GetRootNodeName() override;
    //! Path to libraries in this manager.
    QString GetLibsPath() override;

    QString m_libsPath;
};

#endif // CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENMANAGER_H
