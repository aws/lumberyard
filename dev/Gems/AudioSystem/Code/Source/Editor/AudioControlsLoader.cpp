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

#include <AudioControlsLoader.h>

#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <ACEEnums.h>
#include <ATLCommon.h>
#include <ATLControlsModel.h>
#include <IAudioSystem.h>
#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <QAudioControlTreeWidget.h>

#include <CryFile.h>
#include <CryPath.h>
#include <IEditor.h>
#include <ISystem.h>
#include <StringUtils.h>
#include <Util/PathUtil.h>

#include <QStandardItem>

using namespace PathUtil;

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    namespace LoaderStrings
    {
        static constexpr const char* LevelsSubFolder = "levels";
        static constexpr const char* ConfigFilename = "config.xml";

    } // namespace LoaderStrings

    //-------------------------------------------------------------------------------------------//
    EACEControlType TagToType(const AZStd::string_view tag)
    {
        // maybe a simple map would be better.
        if (tag == Audio::ATLXmlTags::ATLTriggerTag)
        {
            return eACET_TRIGGER;
        }
        else if (tag == Audio::ATLXmlTags::ATLSwitchTag)
        {
            return eACET_SWITCH;
        }
        else if (tag == Audio::ATLXmlTags::ATLSwitchStateTag)
        {
            return eACET_SWITCH_STATE;
        }
        else if (tag == Audio::ATLXmlTags::ATLRtpcTag)
        {
            return eACET_RTPC;
        }
        else if (tag == Audio::ATLXmlTags::ATLEnvironmentTag)
        {
            return eACET_ENVIRONMENT;
        }
        else if (tag == Audio::ATLXmlTags::ATLPreloadRequestTag)
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

        // Get the partial path (relative under asset root) where the controls live.
        const char* controlsPath = nullptr;
        Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);

        // Get the full path up to asset root.
        AZStd::string controlsFullPath(Path::GetEditingGameDataFolder());
        AzFramework::StringFunc::Path::Join(controlsFullPath.c_str(), controlsPath, controlsFullPath);

        // load the global controls
        LoadAllLibrariesInFolder(controlsFullPath, "");

        // load the level specific controls
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;

        AZStd::string searchMask;
        AzFramework::StringFunc::Path::Join(controlsFullPath.c_str(), LoaderStrings::LevelsSubFolder, searchMask);
        AzFramework::StringFunc::Path::Join(searchMask.c_str(), "*.*", searchMask, false, true, false);
        intptr_t handle = pCryPak->FindFirst(searchMask.c_str(), &fd);
        if (handle != -1)
        {
            do
            {
                if (fd.attrib & _A_SUBDIR)
                {
                    AZStd::string_view name = fd.name;
                    if (name != "." && name != "..")
                    {
                        LoadAllLibrariesInFolder(controlsFullPath, name);
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

        AZStd::string configFilePath;
        AzFramework::StringFunc::Path::Join(controlsPath, LoaderStrings::ConfigFilename, configFilePath);

        XmlNodeRef root = GetISystem()->LoadXmlFromFile(configFilePath.c_str());
        if (root)
        {
            AZStd::string rootTag = root->getTag();
            if (rootTag.compare("ACBConfig") == 0 || rootTag.compare("ACEConfig") == 0)
            {
                int size = root->getChildCount();
                for (int i = 0; i < size; ++i)
                {
                    XmlNodeRef child = root->getChild(i);
                    if (child)
                    {
                        AZStd::string tag = child->getTag();
                        AZStd::string name = child->getAttr("name");
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
            m_pModel->AddPlatform("Xenia");
            m_pModel->AddPlatform("Provo");

            // lumberyard: should we write out the config.xml file if it doesn't exist?
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadAllLibrariesInFolder(const AZStd::string_view folderPath, const AZStd::string_view level)
    {
        AZStd::string path(folderPath);
        if (path.back() != AZ_CORRECT_FILESYSTEM_SEPARATOR)
        {
            path.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        }

        if (!level.empty())
        {
            path.append(LoaderStrings::LevelsSubFolder);
            path.append(GetSlash());
            path.append(level);
            path.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        }

        AZStd::string searchPath = path + "*.xml";
        ICryPak* pCryPak = gEnv->pCryPak;
        _finddata_t fd;
        intptr_t handle = pCryPak->FindFirst(searchPath.c_str(), &fd);
        if (handle != -1)
        {
            do
            {
                AZStd::string filename = path + fd.name;
                AzFramework::StringFunc::Path::Normalize(filename);
                XmlNodeRef root = GetISystem()->LoadXmlFromFile(filename.c_str());
                if (root)
                {
                    AZStd::string tag = root->getTag();
                    if (tag == Audio::ATLXmlTags::RootNodeTag)
                    {
                        AZStd::to_lower(filename.begin(), filename.end());
                        m_loadedFilenames.insert(filename.c_str());
                        AZStd::string file = fd.name;
                        if (root->haveAttr(Audio::ATLXmlTags::ATLNameAttribute))
                        {
                            file = root->getAttr(Audio::ATLXmlTags::ATLNameAttribute);
                        }
                        AzFramework::StringFunc::Path::StripExtension(file);
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
    void CAudioControlsLoader::LoadControlsLibrary(XmlNodeRef pRoot, const AZStd::string_view sFilepath, const AZStd::string_view sLevel, const AZStd::string_view sFilename)
    {
        QStandardItem* pRootFolder = AddUniqueFolderPath(m_pLayout->invisibleRootItem(), QString(sFilename.data()));
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
    CATLControl* CAudioControlsLoader::LoadControl(XmlNodeRef pNode, QStandardItem* pFolder, const AZStd::string_view sScope)
    {
        CATLControl* pControl = nullptr;
        if (pNode)
        {
            QStandardItem* pParent = AddUniqueFolderPath(pFolder, QString(pNode->getAttr("path")));
            if (pParent)
            {
                const AZStd::string sName = pNode->getAttr(Audio::ATLXmlTags::ATLNameAttribute);
                const EACEControlType eControlType = TagToType(pNode->getTag());
                pControl = m_pModel->CreateControl(sName, eControlType);
                if (pControl)
                {
                    QStandardItem* pItem = new QAudioControlItem(QString(pControl->GetName().c_str()), pControl);
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
        AZStd::string levelsFolderPath;
        AzFramework::StringFunc::Path::Join(Path::GetEditingGameDataFolder().c_str(), LoaderStrings::LevelsSubFolder, levelsFolderPath);
        LoadScopesImpl(levelsFolderPath);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsLoader::LoadScopesImpl(const AZStd::string_view sLevelsFolder)
    {
        AZStd::string search;
        AzFramework::StringFunc::Path::Join(sLevelsFolder.data(), "*.*", search, false, true, false);
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;
        intptr_t handle = pCryPak->FindFirst(search.c_str(), &fd);      // !!!
        if (handle != -1)
        {
            do
            {
                AZStd::string name = fd.name;
                if (name != "." && name != ".." && !name.empty())
                {
                    if (fd.attrib & _A_SUBDIR)
                    {
                        AzFramework::StringFunc::Path::Join(sLevelsFolder.data(), name.c_str(), search);
                        LoadScopesImpl(search);
                    }
                    else
                    {
                        AZStd::string extension;
                        AzFramework::StringFunc::Path::GetExtension(name.c_str(), extension, false);
                        if (extension.compare("cry") == 0 || extension.compare("ly") == 0)
                        {
                            AzFramework::StringFunc::Path::StripExtension(name);
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
    AZStd::set<AZStd::string> CAudioControlsLoader::GetLoadedFilenamesList()
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
            if (!m_pModel->FindControl(ATLInternalControlNames::GetFocusName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(ATLInternalControlNames::GetFocusName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(ATLInternalControlNames::LoseFocusName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(ATLInternalControlNames::LoseFocusName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(ATLInternalControlNames::MuteAllName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(ATLInternalControlNames::MuteAllName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(ATLInternalControlNames::UnmuteAllName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(ATLInternalControlNames::UnmuteAllName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(ATLInternalControlNames::DoNothingName, eACET_TRIGGER, ""))
            {
                AddControl(m_pModel->CreateControl(ATLInternalControlNames::DoNothingName, eACET_TRIGGER), pFolder);
            }

            if (!m_pModel->FindControl(ATLInternalControlNames::ObjectSpeedName, eACET_RTPC, ""))
            {
                AddControl(m_pModel->CreateControl(ATLInternalControlNames::ObjectSpeedName, eACET_RTPC), pFolder);
            }

            QStandardItem* pSwitch = nullptr;
            CATLControl* pControl = m_pModel->FindControl(ATLInternalControlNames::ObstructionOcclusionCalcName, eACET_SWITCH, "");
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
                pControl = m_pModel->CreateControl(ATLInternalControlNames::ObstructionOcclusionCalcName, eACET_SWITCH);
                pSwitch = AddControl(pControl, pFolder);
            }

            if (pSwitch)
            {
                CATLControl* pChild = nullptr;
                if (!m_pModel->FindControl(ATLInternalControlNames::OOCIgnoreStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, ATLInternalControlNames::ObstructionOcclusionCalcName, ATLInternalControlNames::OOCIgnoreStateName);
                    AddControl(pChild, pSwitch);
                }

                if (!m_pModel->FindControl(ATLInternalControlNames::OOCSingleRayStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, ATLInternalControlNames::ObstructionOcclusionCalcName, ATLInternalControlNames::OOCSingleRayStateName);
                    AddControl(pChild, pSwitch);
                }

                if (!m_pModel->FindControl(ATLInternalControlNames::OOCMultiRayStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, ATLInternalControlNames::ObstructionOcclusionCalcName, ATLInternalControlNames::OOCMultiRayStateName);
                    AddControl(pChild, pSwitch);
                }
            }

            pSwitch = nullptr;
            pControl = m_pModel->FindControl(ATLInternalControlNames::ObjectVelocityTrackingName, eACET_SWITCH, "");
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
                pControl = m_pModel->CreateControl(ATLInternalControlNames::ObjectVelocityTrackingName, eACET_SWITCH);
                pSwitch = AddControl(pControl, pFolder);
            }

            if (pSwitch)
            {
                CATLControl* pChild = nullptr;
                if (!m_pModel->FindControl(ATLInternalControlNames::OVTOnStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, ATLInternalControlNames::ObjectVelocityTrackingName, ATLInternalControlNames::OVTOnStateName);
                    AddControl(pChild, pSwitch);
                }

                if (!m_pModel->FindControl(ATLInternalControlNames::OVTOffStateName, eACET_SWITCH_STATE, "", pControl))
                {
                    pChild = CreateInternalSwitchState(pControl, ATLInternalControlNames::ObjectVelocityTrackingName, ATLInternalControlNames::OVTOffStateName);
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
                const AZStd::string sTag = pNode->getTag();
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
            AZStd::string type = pNode->getAttr(Audio::ATLXmlTags::ATLTypeAttribute);
            if (type.compare(Audio::ATLXmlTags::ATLDataLoadType) == 0)
            {
                pControl->SetAutoLoad(true);
            }
            else
            {
                pControl->SetAutoLoad(false);
            }

            // Read all the platform definitions for this control
            // <ATLPlatforms>
            XmlNodeRef pPlatformsGroup = pNode->findChild(Audio::ATLXmlTags::ATLPlatformsTag);
            if (pPlatformsGroup)
            {
                const int nNumPlatforms = pPlatformsGroup->getChildCount();
                for (int i = 0; i < nNumPlatforms; ++i)
                {
                    XmlNodeRef pPlatformNode = pPlatformsGroup->getChild(i);
                    const AZStd::string sPlatformName = pPlatformNode->getAttr(Audio::ATLXmlTags::ATLNameAttribute);
                    const AZStd::string sGroupName = pPlatformNode->getAttr(Audio::ATLXmlTags::ATLConfigGroupAttribute);
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
                const AZStd::string sTag = pGroupNode->getTag();
                if (sTag.compare(Audio::ATLXmlTags::ATLConfigGroupTag) != 0)
                {
                    continue;
                }
                const AZStd::string sGroupName = pGroupNode->getAttr(Audio::ATLXmlTags::ATLNameAttribute);

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
        QStandardItem* pItem = new QAudioControlItem(QString(pControl->GetName().c_str()), pControl);
        if (pItem)
        {
            pItem->setData(true, eDR_MODIFIED);
            pFolder->appendRow(pItem);
        }
        return pItem;
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CAudioControlsLoader::CreateInternalSwitchState(CATLControl* parentControl, const AZStd::string& switchName, const AZStd::string& stateName)
    {
        CATLControl* childControl = m_pModel->CreateControl(stateName, eACET_SWITCH_STATE, parentControl);

        XmlNodeRef requestNode = GetISystem()->CreateXmlNode(Audio::ATLXmlTags::ATLSwitchRequestTag);
        requestNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, switchName.c_str());
        XmlNodeRef valueNode = requestNode->createNode(Audio::ATLXmlTags::ATLValueTag);
        valueNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, stateName.c_str());
        requestNode->addChild(valueNode);

        childControl->m_connectionNodes[AudioControls::g_sDefaultGroup].push_back(SRawConnectionData(requestNode, false));
        return childControl;
    }
} // namespace AudioControls
