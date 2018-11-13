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
#include "AudioControlsLoader.h"
#include <StringUtils.h>
#include <CryFile.h>
#include <ISystem.h>
#include <IAudioSystem.h>
#include <ATLCommon.h>
#include <ATLControlsModel.h>
#include <CryPath.h>
#include <IAudioSystemEditor.h>
#include <IAudioSystemControl.h>
#include <QAudioControlTreeWidget.h>
#include <QtUtil.h>
#include <Util/PathUtil.h>

#include <QStandardItem>
#include <IEditor.h>

using namespace PathUtil;

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    const char* CAudioControlsLoader::ms_sLevelsFolder = "levels";
    const char* CAudioControlsLoader::ms_sControlsLevelsFolder = "levels";
    const char* CAudioControlsLoader::ms_sConfigFile = "config.xml";

    //-------------------------------------------------------------------------------------------//
    EACEControlType TagToType(const string& tag)
    {
        // maybe a simple map would be better.
        if (tag.compare(Audio::SATLXmlTags::sATLTriggerTag) == 0)
        {
            return eACET_TRIGGER;
        }
        else if (tag.compare(Audio::SATLXmlTags::sATLSwitchTag) == 0)
        {
            return eACET_SWITCH;
        }
        else if (tag.compare(Audio::SATLXmlTags::sATLSwitchStateTag) == 0)
        {
            return eACET_SWITCH_STATE;
        }
        else if (tag.compare(Audio::SATLXmlTags::sATLRtpcTag) == 0)
        {
            return eACET_RTPC;
        }
        else if (tag.compare(Audio::SATLXmlTags::sATLEnvironmentTag) == 0)
        {
            return eACET_ENVIRONMENT;
        }
        else if (tag.compare(Audio::SATLXmlTags::sATLPreloadRequestTag) == 0)
        {
            return eACET_PRELOAD;
        }
        return eACET_NUM_TYPES;
    }

    //-------------------------------------------------------------------------------------------//
    CAudioControlsLoader::CAudioControlsLoader(CATLControlsModel* pATLModel, QStandardItemModel* pLayoutModel, IAudioSystemEditor* pAudioSystemImpl)
        : m_pModel(pATLModel)
        , m_pLayout(pLayoutModel)
        , m_pAudioSystemImpl(pAudioSystemImpl)
    {}

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadAll()
    {
        LoadScopes();
        LoadControls();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadControls()
    {
        const CUndoSuspend suspendUndo;
        LoadSettings();

        // ly: controls path
        const char* controlsPath = nullptr;
        Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);
        const string sControlsPath = string(Path::GetEditingGameDataFolder().c_str()) + GetSlash() + controlsPath;

        // load the global controls
        LoadAllLibrariesInFolder(sControlsPath, "");

        // load the level specific controls
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;
        string searchMask(sControlsPath);
        searchMask.append(ms_sControlsLevelsFolder);
        searchMask.append(GetSlash());
        searchMask.append("*.*");
        intptr_t handle = pCryPak->FindFirst(searchMask, &fd);
        if (handle != -1)
        {
            do
            {
                if (fd.attrib & _A_SUBDIR)
                {
                    string name = fd.name;
                    if (name != "." && name != "..")
                    {
                        LoadAllLibrariesInFolder(sControlsPath, name);
                        if (!m_pModel->ScopeExists(fd.name))
                        {
                            // if the control doesn't exist it
                            // means it is not a real level in the
                            // project so it is flagged as LocalOnly
                            m_pModel->AddScope(fd.name, true);
                        }
                    }
                }
            }
            while (pCryPak->FindNext(handle, &fd) >= 0);
            pCryPak->FindClose(handle);
        }
        CreateDefaultControls();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadSettings()
    {
        const char* controlsPath = nullptr;
        Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);
        string sConfigFilePath = controlsPath;
        sConfigFilePath.append(ms_sConfigFile);

        XmlNodeRef root = GetISystem()->LoadXmlFromFile(sConfigFilePath);
        if (root)
        {
            string rootTag = root->getTag();
            if (rootTag.compare("ACBConfig") == 0)
            {
                int size = root->getChildCount();
                for (int i = 0; i < size; ++i)
                {
                    XmlNodeRef child = root->getChild(i);
                    if (child)
                    {
                        string tag = child->getTag();
                        string name = child->getAttr("name");
                        if (tag.compare("Group") == 0)
                        {
                            if (!name.empty())
                            {
                                m_pModel->AddConnectionGroup(name);
                            }
                        }
                        else if (tag.compare("Platform") == 0)
                        {
                            if (!name.empty())
                            {
                                m_pModel->AddPlatform(name);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // hard code some groups if the config.xml file is missing.
            m_pModel->AddConnectionGroup("default");
            m_pModel->AddConnectionGroup("high");
            m_pModel->AddConnectionGroup("low");

            m_pModel->AddPlatform("Windows");
            m_pModel->AddPlatform("Mac");
            m_pModel->AddPlatform("Linux");
            m_pModel->AddPlatform("Android");
            m_pModel->AddPlatform("iOS");
            m_pModel->AddPlatform("AppleTV");
            m_pModel->AddPlatform("XboxOne");
            m_pModel->AddPlatform("PS4");

            // lumberyard: should we write out the config.xml file if it doesn't exist?
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadAllLibrariesInFolder(const string& folderPath, const string& level)
    {
        string path = AddSlash(folderPath);
        if (!level.empty())
        {
            path.append(ms_sControlsLevelsFolder);
            path.append(GetSlash());
            path.append(level);
            path = AddSlash(path);
        }

        string searchPath = path + "*.xml";
        ICryPak* pCryPak = gEnv->pCryPak;
        _finddata_t fd;
        intptr_t handle = pCryPak->FindFirst(searchPath, &fd);
        if (handle != -1)
        {
            do
            {
                string filename = PathUtil::ToNativePath(path + fd.name);
                XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename);
                if (root)
                {
                    string tag = root->getTag();
                    if (tag == Audio::SATLXmlTags::sRootNodeTag)
                    {
                        m_loadedFilenames.insert(filename.MakeLower());
                        string file = fd.name;
                        if (root->haveAttr(Audio::SATLXmlTags::sATLNameAttribute))
                        {
                            file = root->getAttr(Audio::SATLXmlTags::sATLNameAttribute);
                        }
                        RemoveExtension(file);
                        LoadControlsLibrary(root, folderPath, level, file);
                    }
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "(Audio Controls Editor) Failed parsing game sound file %s", filename);
                }
            }
            while (pCryPak->FindNext(handle, &fd) >= 0);

            pCryPak->FindClose(handle);
        }
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* CAudioControlsLoader::AddFolder(QStandardItem* pParent, const QString& sName)
    {
        if (pParent && !sName.isEmpty())
        {
            const int size = pParent->rowCount();
            for (int i = 0; i < size; ++i)
            {
                QStandardItem* pItem = pParent->child(i);
                if (pItem && (pItem->data(eDR_TYPE) == eIT_FOLDER) && (QString::compare(sName, pItem->text(), Qt::CaseInsensitive) == 0))
                {
                    return pItem;
                }
            }

            QStandardItem* pItem = new QFolderItem(sName);
            if (pParent && pItem)
            {
                pParent->appendRow(pItem);
                return pItem;
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* CAudioControlsLoader::AddUniqueFolderPath(QStandardItem* pParent, const QString& sPath)
    {
        QStringList folderNames = sPath.split(QRegExp("(\\\\|\\/)"), QString::SkipEmptyParts);
        const int size = folderNames.length();
        for (int i = 0; i < size; ++i)
        {
            if (!folderNames[i].isEmpty())
            {
                QStandardItem* pChild = AddFolder(pParent, folderNames[i]);
                if (pChild)
                {
                    pParent = pChild;
                }
            }
        }
        return pParent;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadControlsLibrary(XmlNodeRef pRoot, const string& sFilepath, const string& sLevel, const string& sFilename)
    {
        QStandardItem* pRootFolder = AddUniqueFolderPath(m_pLayout->invisibleRootItem(), QtUtil::ToQString(sFilename));
        if (pRootFolder && pRoot)
        {
            const int nControlTypeCount = pRoot->getChildCount();
            for (int i = 0; i < nControlTypeCount; ++i)
            {
                XmlNodeRef pNode = pRoot->getChild(i);
                const int nControlCount = pNode->getChildCount();
                for (int j = 0; j < nControlCount; ++j)
                {
                    LoadControl(pNode->getChild(j), pRootFolder, sLevel);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CAudioControlsLoader::LoadControl(XmlNodeRef pNode, QStandardItem* pFolder, const string& sScope)
    {
        CATLControl* pControl = nullptr;
        if (pNode)
        {
            QStandardItem* pParent = AddUniqueFolderPath(pFolder, QtUtil::ToQString(pNode->getAttr("path")));
            if (pParent)
            {
                const string sName = pNode->getAttr(Audio::SATLXmlTags::sATLNameAttribute);
                const EACEControlType eControlType = TagToType(pNode->getTag());
                pControl = m_pModel->CreateControl(sName, eControlType);
                if (pControl)
                {
                    QStandardItem* pItem = new QAudioControlItem(QtUtil::ToQString(pControl->GetName()), pControl);
                    if (pItem)
                    {
                        pParent->appendRow(pItem);
                    }

                    switch (eControlType)
                    {
                        case eACET_SWITCH:
                        {
                            const int nStateCount = pNode->getChildCount();
                            for (int i = 0; i < nStateCount; ++i)
                            {
                                CATLControl* pStateControl = LoadControl(pNode->getChild(i), pItem, sScope);
                                if (pStateControl)
                                {
                                    pStateControl->SetParent(pControl);
                                    pControl->AddChild(pStateControl);
                                }
                            }
                            break;
                        }
                        case eACET_PRELOAD:
                        {
                            LoadPreloadConnections(pNode, pControl);
                            break;
                        }
                        default:
                        {
                            LoadConnections(pNode, pControl);
                            break;
                        }
                    }
                    pControl->SetScope(sScope);
                }
            }
        }
        return pControl;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadScopes()
    {
        const string levelsFolderPath = string(Path::GetEditingGameDataFolder().c_str()) + GetSlash() + ms_sLevelsFolder;
        LoadScopesImpl(levelsFolderPath);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadScopesImpl(const string& sLevelsFolder)
    {
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;
        intptr_t handle = pCryPak->FindFirst(sLevelsFolder + GetSlash() + "*.*", &fd);
        if (handle != -1)
        {
            do
            {
                string name = fd.name;
                if (name != "." && name != ".." && !name.empty())
                {
                    if (fd.attrib & _A_SUBDIR)
                    {
                        LoadScopesImpl(sLevelsFolder + GetSlash() + name);
                    }
                    else
                    {
                        string extension = GetExt(name);
                        if (extension.compare("cry") == 0)
                        {
                            RemoveExtension(name);
                            m_pModel->AddScope(name);
                        }
                    }
                }
            }
            while (pCryPak->FindNext(handle, &fd) >= 0);
            pCryPak->FindClose(handle);
        }
    }

    //-------------------------------------------------------------------------------------------//
    std::set<string> CAudioControlsLoader::GetLoadedFilenamesList()
    {
        return m_loadedFilenames;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::CreateDefaultControls()
    {
        // Load default controls if the don't exist.
        // These controls need to always exist in your project
        using namespace Audio;

        QStandardItem* pFolder = AddFolder(m_pLayout->invisibleRootItem(), "default_controls");
        if (pFolder)
        {
            if (!m_pModel->FindControl(SATLInternalControlNames::sGetFocusName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(SATLInternalControlNames::sGetFocusName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(SATLInternalControlNames::sLoseFocusName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(SATLInternalControlNames::sLoseFocusName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(SATLInternalControlNames::sMuteAllName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(SATLInternalControlNames::sMuteAllName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(SATLInternalControlNames::sUnmuteAllName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(SATLInternalControlNames::sUnmuteAllName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(SATLInternalControlNames::sDoNothingName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(SATLInternalControlNames::sDoNothingName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(SATLInternalControlNames::sObjectSpeedName, eACET_RTPC, ""))
            {
                AddControl(m_pModel->CreateControl(SATLInternalControlNames::sObjectSpeedName, eACET_RTPC), pFolder);
            }

            QStandardItem* pSwitch = nullptr;
            CATLControl* pControl = m_pModel->FindControl(SATLInternalControlNames::sObstructionOcclusionCalcName, eACET_SWITCH, "");
            if (pControl)
            {
                QModelIndexList indexes = m_pLayout->match(m_pLayout->index(0, 0, QModelIndex()), eDR_ID, pControl->GetId(), 1, Qt::MatchRecursive);
                if (!indexes.empty())
                {
                    pSwitch = m_pLayout->itemFromIndex(indexes.at(0));
                }
            }
            else
            {
                pControl = m_pModel->CreateControl(SATLInternalControlNames::sObstructionOcclusionCalcName, eACET_SWITCH);
                pSwitch = AddControl(pControl, pFolder);
            }

            if (pSwitch)
            {
                CATLControl* pChild = nullptr;
                if (!m_pModel->FindControl(SATLInternalControlNames::sOOCIgnoreStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, SATLInternalControlNames::sObstructionOcclusionCalcName, SATLInternalControlNames::sOOCIgnoreStateName);
                    AddControl(pChild, pSwitch);
                }

                if (!m_pModel->FindControl(SATLInternalControlNames::sOOCSingleRayStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, SATLInternalControlNames::sObstructionOcclusionCalcName, SATLInternalControlNames::sOOCSingleRayStateName);
                    AddControl(pChild, pSwitch);
                }

                if (!m_pModel->FindControl(SATLInternalControlNames::sOOCMultiRayStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, SATLInternalControlNames::sObstructionOcclusionCalcName, SATLInternalControlNames::sOOCMultiRayStateName);
                    AddControl(pChild, pSwitch);
                }
            }

            pSwitch = nullptr;
            pControl = m_pModel->FindControl(SATLInternalControlNames::sObjectVelocityTrackingName, eACET_SWITCH, "");
            if (pControl)
            {
                QModelIndexList indexes = m_pLayout->match(m_pLayout->index(0, 0, QModelIndex()), eDR_ID, pControl->GetId(), 1, Qt::MatchRecursive);
                if (!indexes.empty())
                {
                    pSwitch = m_pLayout->itemFromIndex(indexes.at(0));
                }
            }
            else
            {
                pControl = m_pModel->CreateControl(SATLInternalControlNames::sObjectVelocityTrackingName, eACET_SWITCH);
                pSwitch = AddControl(pControl, pFolder);
            }

            if (pSwitch)
            {
                CATLControl* pChild = nullptr;
                if (!m_pModel->FindControl(SATLInternalControlNames::sOVTOnStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, SATLInternalControlNames::sObjectVelocityTrackingName, SATLInternalControlNames::sOVTOnStateName);
                    AddControl(pChild, pSwitch);
                }

                if (!m_pModel->FindControl(SATLInternalControlNames::sOVTOffStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, SATLInternalControlNames::sObjectVelocityTrackingName, SATLInternalControlNames::sOVTOffStateName);
                    AddControl(pChild, pSwitch);
                }

                if (!pFolder->hasChildren())
                {
                    m_pLayout->removeRow(pFolder->row(), m_pLayout->indexFromItem(pFolder->parent()));
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadConnections(XmlNodeRef pRoot, CATLControl* pControl)
    {
        if (pControl)
        {
            const int nSize = pRoot->getChildCount();
            for (int i = 0; i < nSize; ++i)
            {
                XmlNodeRef pNode = pRoot->getChild(i);
                const string sTag = pNode->getTag();
                if (m_pAudioSystemImpl)
                {
                    TConnectionPtr pConnection = m_pAudioSystemImpl->CreateConnectionFromXMLNode(pNode, pControl->GetType());
                    if (pConnection)
                    {
                        pControl->AddConnection(pConnection);
                    }
                    pControl->m_connectionNodes[AudioControls::g_sDefaultGroup].push_back(SRawConnectionData(pNode, pConnection != nullptr));
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadPreloadConnections(XmlNodeRef pNode, CATLControl* pControl)
    {
        if (pControl)
        {
            string type = pNode->getAttr(Audio::SATLXmlTags::sATLTypeAttribute);
            if (type.compare(Audio::SATLXmlTags::sATLDataLoadType) == 0)
            {
                pControl->SetAutoLoad(true);
            }
            else
            {
                pControl->SetAutoLoad(false);
            }

            // Read all the platform definitions for this control
            // <ATLPlatforms>
            XmlNodeRef pPlatformsGroup = pNode->findChild(Audio::SATLXmlTags::sATLPlatformsTag);
            if (pPlatformsGroup)
            {
                const int nNumPlatforms = pPlatformsGroup->getChildCount();
                for (int i = 0; i < nNumPlatforms; ++i)
                {
                    XmlNodeRef pPlatformNode = pPlatformsGroup->getChild(i);
                    const string sPlatformName = pPlatformNode->getAttr(Audio::SATLXmlTags::sATLNameAttribute);
                    const string sGroupName = pPlatformNode->getAttr(Audio::SATLXmlTags::sATLConfigGroupAttribute);
                    m_pModel->AddPlatform(sPlatformName);

                    const int nGroupID = m_pModel->GetConnectionGroupId(sGroupName);
                    if (nGroupID >= 0)
                    {
                        pControl->SetGroupForPlatform(sPlatformName, nGroupID);
                    }
                }
            }

            // Read the connection information for each of the platform groups
            const int nNumChildren = pNode->getChildCount();
            for (int i = 0; i < nNumChildren; ++i)
            {
                // <ATLConfigGroup>
                XmlNodeRef pGroupNode = pNode->getChild(i);
                const string sTag = pGroupNode->getTag();
                if (sTag.compare(Audio::SATLXmlTags::sATLConfigGroupTag) != 0)
                {
                    continue;
                }
                const string sGroupName = pGroupNode->getAttr(Audio::SATLXmlTags::sATLNameAttribute);

                const int nNumConnections = pGroupNode->getChildCount();
                for (int j = 0; j < nNumConnections; ++j)
                {
                    XmlNodeRef pConnectionNode = pGroupNode->getChild(j);
                    if (pConnectionNode && m_pAudioSystemImpl)
                    {
                        TConnectionPtr pAudioConnection = m_pAudioSystemImpl->CreateConnectionFromXMLNode(pConnectionNode, pControl->GetType());
                        if (pAudioConnection)
                        {
                            pAudioConnection->SetGroup(sGroupName);
                            pControl->AddConnection(pAudioConnection);
                        }
                        pControl->m_connectionNodes[sGroupName].push_back(SRawConnectionData(pConnectionNode, pAudioConnection != nullptr));
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    QStandardItem* CAudioControlsLoader::AddControl(CATLControl* pControl, QStandardItem* pFolder)
    {
        QStandardItem* pItem = new QAudioControlItem(QtUtil::ToQString(pControl->GetName()), pControl);
        if (pItem)
        {
            pItem->setData(true, eDR_MODIFIED);
            pFolder->appendRow(pItem);
        }
        return pItem;
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CAudioControlsLoader::CreateInternalSwitchState(CATLControl* parentControl, const string& switchName, const string& stateName)
    {
        CATLControl* childControl = m_pModel->CreateControl(stateName, eACET_SWITCH_STATE, parentControl);

        XmlNodeRef requestNode = GetISystem()->CreateXmlNode(Audio::SATLXmlTags::sATLSwitchRequestTag);
        requestNode->setAttr(Audio::SATLXmlTags::sATLNameAttribute, switchName);
        XmlNodeRef valueNode = requestNode->createNode(Audio::SATLXmlTags::sATLValueTag);
        valueNode->setAttr(Audio::SATLXmlTags::sATLNameAttribute, stateName);
        requestNode->addChild(valueNode);

        childControl->m_connectionNodes[AudioControls::g_sDefaultGroup].push_back(SRawConnectionData(requestNode, false));
        return childControl;
    }
} // namespace AudioControls
