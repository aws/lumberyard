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

#include "StdAfx.h"
#include "EntityPrototypeManager.h"

#include "EntityPrototype.h"
#include "EntityPrototypeLibrary.h"
#include "Util/CryMemFile.h"
#include "Util/PakFile.h"

#define ENTITY_LIBS_PATH "Libs/EntityArchetypes/"

//////////////////////////////////////////////////////////////////////////
// CEntityPrototypeManager implementation.
//////////////////////////////////////////////////////////////////////////
CEntityPrototypeManager::CEntityPrototypeManager()
{
    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);
}

//////////////////////////////////////////////////////////////////////////
CEntityPrototypeManager::~CEntityPrototypeManager()
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityPrototypeManager::ClearAll()
{
    CBaseLibraryManager::ClearAll();

    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);
}

//////////////////////////////////////////////////////////////////////////
CEntityPrototype* CEntityPrototypeManager::LoadPrototype(CEntityPrototypeLibrary* pLibrary, XmlNodeRef& node)
{
    assert(pLibrary);
    assert(node != NULL);

    CBaseLibraryItem::SerializeContext ctx(node, true);
    ctx.bCopyPaste = true;
    ctx.bUniqName = true;

    CEntityPrototype* prototype = new CEntityPrototype;
    pLibrary->AddItem(prototype);
    prototype->Serialize(ctx);
    return prototype;
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem* CEntityPrototypeManager::MakeNewItem()
{
    return new CEntityPrototype;
}

//////////////////////////////////////////////////////////////////////////
CBaseLibrary* CEntityPrototypeManager::MakeNewLibrary()
{
    return new CEntityPrototypeLibrary(this);
}

//////////////////////////////////////////////////////////////////////////
QString CEntityPrototypeManager::GetRootNodeName()
{
    return "EntityPrototypesLibs";
}

//////////////////////////////////////////////////////////////////////////
QString CEntityPrototypeManager::GetLibsPath()
{
    if (m_libsPath.isEmpty())
    {
        m_libsPath = ENTITY_LIBS_PATH;
    }
    return m_libsPath;
}

//////////////////////////////////////////////////////////////////////////
void CEntityPrototypeManager::ExportPrototypes(const QString& path, const QString& levelName, CPakFile& levelPakFile)
{
    if (!m_pLevelLibrary)
    {
        return;
    }

    XmlNodeRef node = XmlHelpers::CreateXmlNode("EntityPrototypeLibrary");
    m_pLevelLibrary->Serialize(node, false);
    XmlString xmlData = node->getXML();

    CCryMemFile file;
    file.Write(xmlData.c_str(), xmlData.length());
    QString filename = Path::Make(path, "LevelPrototypes.xml");
    levelPakFile.UpdateFile(filename.toUtf8().data(), file);
}
