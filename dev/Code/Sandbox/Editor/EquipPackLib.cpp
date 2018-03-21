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
#include "EquipPackLib.h"
#include "EquipPack.h"
#include "GameEngine.h"
#include "Util/FileUtil.h"

#include <IEditorGame.h>

#define EQUIPMENTPACKS_PATH "/Libs/EquipmentPacks/"

CEquipPackLib::CEquipPackLib()
{
}

CEquipPackLib::~CEquipPackLib()
{
    Reset();
}

CEquipPack* CEquipPackLib::CreateEquipPack(const QString& name)
{
    TEquipPackMap::iterator iter = m_equipmentPacks.find(name);
    if (iter != m_equipmentPacks.end())
    {
        return 0;
    }

    CEquipPack* pPack = new CEquipPack(this);
    if (!pPack)
    {
        return 0;
    }
    pPack->SetName(name);
    m_equipmentPacks.insert(TEquipPackMap::value_type(name, pPack));
    return pPack;
}

// currently we ignore the bDeleteFromDisk.
// will have to be manually removed via Source control
bool CEquipPackLib::RemoveEquipPack(const QString& name, bool /* bDeleteFromDisk */)
{
    TEquipPackMap::iterator iter = m_equipmentPacks.find(name);
    if (iter == m_equipmentPacks.end())
    {
        return false;
    }
    delete iter->second;
    m_equipmentPacks.erase(iter);

#if 0
    if (bDeleteFromDisk)
    {
        QString path = (Path::GetEditingGameDataFolder() + EQUIPMENTPACKS_PATH).c_str();
        path += name;
        path += ".xml";
        bool bOK = CFileUtil::OverwriteFile(path);
        if (bOK)
        {
            ::DeleteFile(path.toUtf8().data());
        }
    }
#endif

    return true;
}

CEquipPack* CEquipPackLib::FindEquipPack(const QString& name)
{
    TEquipPackMap::iterator iter = m_equipmentPacks.find(name);
    if (iter == m_equipmentPacks.end())
    {
        return 0;
    }
    return iter->second;
}

bool CEquipPackLib::RenameEquipPack(const QString& name, const QString& newName)
{
    TEquipPackMap::iterator iter = m_equipmentPacks.find(name);
    if (iter == m_equipmentPacks.end())
    {
        return false;
    }

    CEquipPack* pPack = iter->second;
    pPack->SetName(newName);
    m_equipmentPacks.erase(iter);
    m_equipmentPacks.insert(TEquipPackMap::value_type(newName, pPack));
    return true;
}

bool CEquipPackLib::LoadLibs(bool bExportToGame)
{
    IEquipmentSystemInterface* pESI = GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface();
    if (bExportToGame && pESI)
    {
        pESI->DeleteAllEquipmentPacks();
    }

    QString path = (Path::GetEditingGameDataFolder() + EQUIPMENTPACKS_PATH).c_str();
    // std::vector<CString> files;
    // CFileEnum::ScanDirectory(path, "*.xml", files);

    IFileUtil::FileArray files;
    CFileUtil::ScanDirectory(path, "*.xml", files, false);

    for (int iFile = 0; iFile < files.size(); ++iFile)
    {
        QString filename = path;
        filename += files[iFile].filename;
        // CryLogAlways("Filename '%s'", filename);

        XmlNodeRef node = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
        if (node == 0)
        {
            Warning("CEquipPackLib:LoadLibs: Cannot load pack from file '%s'", filename.toUtf8().data());
            continue;
        }

        if (node->isTag("EquipPack") == false)
        {
            Warning("CEquipPackLib:LoadLibs: File '%s' is not an equipment pack. Skipped.", filename.toUtf8().data());
            continue;
        }

        QString packName;
        if (!node->getAttr("name", packName))
        {
            packName = "Unnamed";
        }

        CEquipPack* pPack = CreateEquipPack(packName);
        if (pPack == 0)
        {
            continue;
        }
        pPack->Load(node);
        pPack->SetModified(false);

        if (bExportToGame && pESI)
        {
            pESI->LoadEquipmentPack(node);
        }
    }

    return true;
}

bool CEquipPackLib::SaveLibs(bool bExportToGame)
{
    IEquipmentSystemInterface* pESI = GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface();
    if (bExportToGame && pESI)
    {
        pESI->DeleteAllEquipmentPacks();
    }

    QString libsPath = (Path::GetEditingGameDataFolder() + EQUIPMENTPACKS_PATH).c_str();
    if (!libsPath.isEmpty())
    {
        CFileUtil::CreateDirectory(libsPath.toUtf8().data());
    }

    for (TEquipPackMap::iterator iter = m_equipmentPacks.begin(); iter != m_equipmentPacks.end(); ++iter)
    {
        CEquipPack* pPack = iter->second;
        XmlNodeRef packNode = XmlHelpers::CreateXmlNode("EquipPack");
        pPack->Save(packNode);
        if (pPack->IsModified())
        {
            QString path (libsPath);
            path += iter->second->GetName();
            path += ".xml";
            bool bOK = XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), packNode, path.toUtf8().data());
            if (bOK)
            {
                pPack->SetModified(false);
            }
        }
        if (bExportToGame && pESI)
        {
            pESI->LoadEquipmentPack(packNode);
        }
    }
    return true;
}

void CEquipPackLib::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bResetWhenLoad)
{
    if (!xmlNode)
    {
        return;
    }
    if (bLoading)
    {
        if (bResetWhenLoad)
        {
            Reset();
        }
        XmlNodeRef equipXMLNode = xmlNode->findChild("EquipPacks");
        if (equipXMLNode)
        {
            for (int i = 0; i < equipXMLNode->getChildCount(); ++i)
            {
                XmlNodeRef node = equipXMLNode->getChild(i);
                if (node->isTag("EquipPack") == false)
                {
                    continue;
                }
                QString packName;
                if (!node->getAttr("name", packName))
                {
                    CLogFile::FormatLine("Warning: Unnamed EquipPack found !");
                    packName = "Unnamed";
                    node->setAttr("name", packName.toUtf8().data());
                }
                CEquipPack* pCurPack = CreateEquipPack(packName);
                if (!pCurPack)
                {
                    CLogFile::FormatLine("Warning: Unable to create EquipPack %s !", packName.toUtf8().data());
                    continue;
                }
                pCurPack->Load(node);
                pCurPack->SetModified(false);

                auto pESI = GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface();
                if (pESI != nullptr)
                {
                    pESI->LoadEquipmentPack(node);
                }
            }
        }
    }
    else
    {
        XmlNodeRef equipXMLNode = xmlNode->newChild("EquipPacks");
        for (TEquipPackMap::iterator iter = m_equipmentPacks.begin(); iter != m_equipmentPacks.end(); ++iter)
        {
            XmlNodeRef packNode = equipXMLNode->newChild("EquipPack");
            iter->second->Save(packNode);
        }
    }
}

void CEquipPackLib::Reset()
{
    for (TEquipPackMap::iterator iter = m_equipmentPacks.begin(); iter != m_equipmentPacks.end(); ++iter)
    {
        delete iter->second;
    }
    m_equipmentPacks.clear();
}

void CEquipPackLib::ExportToGame()
{
    IEquipmentSystemInterface* pESI = GetIEditor()->GetGameEngine()->GetIEquipmentSystemInterface();
    if (pESI)
    {
        pESI->DeleteAllEquipmentPacks();
        for (TEquipPackMap::iterator iter = m_equipmentPacks.begin(); iter != m_equipmentPacks.end(); ++iter)
        {
            XmlNodeRef packNode = XmlHelpers::CreateXmlNode("EquipPack");
            iter->second->Save(packNode);
            pESI->LoadEquipmentPack(packNode);
        }
    }
}
