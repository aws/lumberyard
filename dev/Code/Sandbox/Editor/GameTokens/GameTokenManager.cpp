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
#include "GameTokenManager.h"

#include "GameTokenItem.h"
#include "GameTokenLibrary.h"

#include "GameEngine.h"

#include "DataBaseDialog.h"
#include "GameTokenDialog.h"

#define GAMETOCKENS_LIBS_PATH "Libs/GameTokens/"

namespace
{
    const char* kGameTokensFile = "GameTokens.xml";
    const char* kGameTokensRoot = "GameTokens";
};

//////////////////////////////////////////////////////////////////////////
// CGameTokenManager implementation.
//////////////////////////////////////////////////////////////////////////
CGameTokenManager::CGameTokenManager()
{
    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);
    m_bUniqGuidMap = false;
    m_bUniqNameMap = true;
}

//////////////////////////////////////////////////////////////////////////
CGameTokenManager::~CGameTokenManager()
{
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenManager::ClearAll()
{
    CBaseLibraryManager::ClearAll();
    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem* CGameTokenManager::MakeNewItem()
{
    return new CGameTokenItem;
}
//////////////////////////////////////////////////////////////////////////
CBaseLibrary* CGameTokenManager::MakeNewLibrary()
{
    return new CGameTokenLibrary(static_cast<IBaseLibraryManager*>(this));
}
//////////////////////////////////////////////////////////////////////////
void CGameTokenManager::SaveAllLibs()
{
    bool levelLibModified = m_pLevelLibrary ? m_pLevelLibrary->IsModified() : false;

    CBaseLibraryManager::SaveAllLibs();

    if (levelLibModified)
    {
        CryMessageBox("The Level library contains unsaved changes.\r\n"
            "These changes will be saved next time you save the level.", 
            "Information", MB_ICONINFORMATION | MB_OK);
    }
}
//////////////////////////////////////////////////////////////////////////
QString CGameTokenManager::GetRootNodeName()
{
    return "GameTokensLibrary";
}
//////////////////////////////////////////////////////////////////////////
QString CGameTokenManager::GetLibsPath()
{
    if (m_libsPath.isEmpty())
    {
        m_libsPath = GAMETOCKENS_LIBS_PATH;
    }
    return m_libsPath;
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenManager::Export(XmlNodeRef& node)
{
    XmlNodeRef libs = node->newChild("GameTokensLibraryReferences");
    for (int i = 0; i < GetLibraryCount(); i++)
    {
        IDataBaseLibrary* pLib = GetLibrary(i);
        // Level libraries are saved in level.
        if (pLib->IsLevelLibrary())
        {
            continue;
        }
        // For Non-Level libs (Global) we save a reference (name)
        // as these are stored in the Game/Libs directory
        XmlNodeRef libNode = libs->newChild("Library");
        libNode->setAttr("Name", pLib->GetName().toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    CBaseLibraryManager::OnEditorNotifyEvent(event);


    switch (event)
    {
    case eNotify_OnBeginGameMode:
    case eNotify_OnEndGameMode:
        // Restore default values on modified tokens.
    {
        for (ItemsNameMap::iterator it = m_itemsNameMap.begin(); it != m_itemsNameMap.end(); ++it)
        {
            CGameTokenItem* pToken = (CGameTokenItem*)&*it->second;
            TFlowInputData data;
            pToken->GetValue(data);
            pToken->SetValue(data, true);
        }
    }
    break;
    }
}


//////////////////////////////////////////////////////////////////////////
void CGameTokenManager::Save()
{
    CTempFileHelper helper((GetIEditor()->GetLevelDataFolder() + kGameTokensFile).toUtf8().data());

    XmlNodeRef root = XmlHelpers::CreateXmlNode(kGameTokensRoot);
    Serialize(root, false);
    root->saveToFile(helper.GetTempFilePath().toUtf8().data());

    helper.UpdateFile(false);
}


//////////////////////////////////////////////////////////////////////////
bool CGameTokenManager::Load()
{
    QString filename = GetIEditor()->GetLevelDataFolder() + kGameTokensFile;
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
    if (root && !_stricmp(root->getTag(), kGameTokensRoot))
    {
        Serialize(root, true);
        return true;
    }
    return false;
}
