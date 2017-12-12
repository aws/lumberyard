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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREMANAGER_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREMANAGER_H
#pragma once

#include "BaseLibraryManager.h"

class IOpticsElementBase;
class CLensFlareEditor;

class CRYEDIT_API CLensFlareManager
    : public CBaseLibraryManager
{
public:
    CLensFlareManager();
    virtual ~CLensFlareManager();

    void ClearAll();

    virtual bool LoadFlareItemByName(const QString& fullItemName, IOpticsElementBasePtr pDestOptics);
    void Modified();
    //! Path to libraries in this manager.
    QString GetLibsPath();
    IDataBaseLibrary* LoadLibrary(const QString& filename, bool bReload = false);

private:
    CBaseLibraryItem* MakeNewItem();
    CBaseLibrary* MakeNewLibrary();

    //! Root node where this library will be saved.
    QString GetRootNodeName();
    QString m_libsPath;
};

#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREMANAGER_H
