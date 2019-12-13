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

#include <AudioControlsWriter.h>

#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <ACEEnums.h>
#include <ATLControlsModel.h>
#include <CryFile.h>
#include <IAudioSystem.h>
#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <IEditor.h>
#include <Include/IFileUtil.h>
#include <Include/ISourceControl.h>
#include <ISystem.h>
#include <StringUtils.h>
#include <Util/PathUtil.h>

#include <QModelIndex>
#include <QStandardItemModel>
#include <QFileInfo>


using namespace PathUtil;

namespace AudioControls
{
    namespace WriterStrings
    {
        static constexpr const char* LevelsSubFolder = "levels";
        static constexpr const char* LibraryExtension = ".xml";

    } // namespace WriterStrings

    //-------------------------------------------------------------------------------------------//
    AZStd::string_view TypeToTag(EACEControlType eType)
    {
        switch (eType)
        {
        case eACET_RTPC:
            return Audio::ATLXmlTags::ATLRtpcTag;
        case eACET_TRIGGER:
            return Audio::ATLXmlTags::ATLTriggerTag;
        case eACET_SWITCH:
            return Audio::ATLXmlTags::ATLSwitchTag;
        case eACET_SWITCH_STATE:
            return Audio::ATLXmlTags::ATLSwitchStateTag;
        case eACET_PRELOAD:
            return Audio::ATLXmlTags::ATLPreloadRequestTag;
        case eACET_ENVIRONMENT:
            return Audio::ATLXmlTags::ATLEnvironmentTag;
        }
        return "";
    }

    //-------------------------------------------------------------------------------------------//
    CAudioControlsWriter::CAudioControlsWriter(CATLControlsModel* pATLModel, QStandardItemModel* pLayoutModel, IAudioSystemEditor* pAudioSystemImpl, AZStd::set<AZStd::string>& previousLibraryPaths)
        : m_pATLModel(pATLModel)
        , m_pLayoutModel(pLayoutModel)
        , m_pAudioSystemImpl(pAudioSystemImpl)
    {
        if (pATLModel && pLayoutModel && pAudioSystemImpl)
        {
            m_pLayoutModel->blockSignals(true);

            int i = 0;
            QModelIndex index = m_pLayoutModel->index(i, 0);
            while (index.isValid())
            {
                WriteLibrary(index.data(Qt::DisplayRole).toString().toUtf8().data(), index);
                index = index.sibling(++i, 0);
            }


            // Delete libraries that don't exist anymore from disk
            AZStd::set<AZStd::string> librariesToDelete;
            AZStd::set_difference(
                previousLibraryPaths.begin(), previousLibraryPaths.end(),
                m_foundLibraryPaths.begin(), m_foundLibraryPaths.end(),
                AZStd::inserter(librariesToDelete, librariesToDelete.begin())
            );

            for (auto it = librariesToDelete.begin(); it != librariesToDelete.end(); ++it)
            {
                DeleteLibraryFile((*it).c_str());
            }

            previousLibraryPaths = m_foundLibraryPaths;

            m_pLayoutModel->blockSignals(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteLibrary(const AZStd::string_view sLibraryName, QModelIndex root)
    {
        if (root.isValid())
        {
            TLibraryStorage library;
            int i = 0;
            QModelIndex child = root.child(i, 0);
            while (child.isValid())
            {
                WriteItem(child, "", library, root.data(eDR_MODIFIED).toBool());
                child = root.child(++i, 0);
            }

            const char* controlsPath = nullptr;
            Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);

            for (auto& libraryPair : library)
            {
                AZStd::string sLibraryPath;
                const AZStd::string& sScope = libraryPair.first;
                if (sScope.empty())
                {
                    // no scope, file at the root level
                    sLibraryPath.append(controlsPath);
                    AzFramework::StringFunc::Path::Join(sLibraryPath.c_str(), sLibraryName.data(), sLibraryPath);
                    sLibraryPath.append(WriterStrings::LibraryExtension);
                }
                else
                {
                    // with scope, inside level folder
                    sLibraryPath.append(controlsPath);
                    sLibraryPath.append(WriterStrings::LevelsSubFolder);
                    AzFramework::StringFunc::Path::Join(sLibraryPath.c_str(), sScope.c_str(), sLibraryPath);
                    AzFramework::StringFunc::Path::Join(sLibraryPath.c_str(), sLibraryName.data(), sLibraryPath);
                    sLibraryPath.append(WriterStrings::LibraryExtension);
                }

                // should be able to change this back to GamePathToFullPath once a path normalization bug has been fixed:
                AZStd::string sFullFilePath;
                AzFramework::StringFunc::Path::Join(Path::GetEditingGameDataFolder().c_str(), sLibraryPath.c_str(), sFullFilePath);
                AZStd::to_lower(sFullFilePath.begin(), sFullFilePath.end());
                m_foundLibraryPaths.insert(sFullFilePath.c_str());

                const SLibraryScope& libScope = libraryPair.second;
                if (libScope.m_isDirty)
                {
                    XmlNodeRef pFileNode = GetISystem()->CreateXmlNode(Audio::ATLXmlTags::RootNodeTag);
                    pFileNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, sLibraryName.data());

                    for (int i = 0; i < eACET_NUM_TYPES; ++i)
                    {
                        if (i != eACET_SWITCH_STATE) // switch_states are written inside the switches
                        {
                            if (libScope.m_nodes[i]->getChildCount() > 0)
                            {
                                pFileNode->addChild(libScope.m_nodes[i]);
                            }
                        }
                    }

                    if (QFileInfo::exists(sFullFilePath.c_str()))
                    {
                        const DWORD fileAttributes = GetFileAttributes(sFullFilePath.c_str());
                        if (fileAttributes & FILE_ATTRIBUTE_READONLY)
                        {
                            // file is read-only
                            CheckOutFile(sFullFilePath);
                        }
                        pFileNode->saveToFile(sFullFilePath.c_str());
                    }
                    else
                    {
                        // save the file, CheckOutFile will add it, since it's new
                        pFileNode->saveToFile(sFullFilePath.c_str());
                        CheckOutFile(sFullFilePath);
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteItem(QModelIndex index, const AZStd::string& path, TLibraryStorage& library, bool bParentModified)
    {
        if (index.isValid())
        {
            if (index.data(eDR_TYPE) == eIT_FOLDER)
            {
                int i = 0;
                QModelIndex child = index.child(i, 0);
                while (child.isValid())
                {
                    AZStd::string sNewPath = path.empty() ? "" : path + "/";
                    sNewPath += index.data(Qt::DisplayRole).toString().toUtf8().data();
                    WriteItem(child, sNewPath, library, index.data(eDR_MODIFIED).toBool() || bParentModified);
                    child = index.child(++i, 0);
                }

                QStandardItem* pItem = m_pLayoutModel->itemFromIndex(index);
                if (pItem)
                {
                    pItem->setData(false, eDR_MODIFIED);
                }
            }
            else
            {
                CATLControl* pControl = m_pATLModel->GetControlByID(index.data(eDR_ID).toUInt());
                if (pControl)
                {
                    SLibraryScope& scope = library[pControl->GetScope()];
                    if (IsItemModified(index) || bParentModified)
                    {
                        scope.m_isDirty = true;
                        QStandardItem* pItem = m_pLayoutModel->itemFromIndex(index);
                        if (pItem)
                        {
                            pItem->setData(false, eDR_MODIFIED);
                        }
                    }
                    WriteControlToXml(scope.m_nodes[pControl->GetType()], pControl, path);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    bool CAudioControlsWriter::IsItemModified(QModelIndex index)
    {
        if (index.data(eDR_MODIFIED).toBool() == true)
        {
            return true;
        }

        int i = 0;
        QModelIndex child = index.child(i, 0);
        while (child.isValid())
        {
            if (IsItemModified(child))
            {
                return true;
            }
            child = index.child(++i, 0);
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteControlToXml(XmlNodeRef pNode, CATLControl* pControl, const AZStd::string_view sPath)
    {
        const EACEControlType type = pControl->GetType();
        XmlNodeRef pChildNode = pNode->createNode(TypeToTag(type).data());
        pChildNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, pControl->GetName().c_str());
        if (!sPath.empty())
        {
            pChildNode->setAttr("path", sPath.data());
        }

        if (type == eACET_SWITCH)
        {
            const size_t size = pControl->ChildCount();
            for (size_t i = 0; i < size; ++i)
            {
                WriteControlToXml(pChildNode, pControl->GetChild(i), "");
            }
        }
        else if (type == eACET_PRELOAD)
        {
            if (pControl->IsAutoLoad())
            {
                pChildNode->setAttr(Audio::ATLXmlTags::ATLTypeAttribute, Audio::ATLXmlTags::ATLDataLoadType);
            }
            WritePlatformsToXml(pChildNode, pControl);
            uint nNumGroups = m_pATLModel->GetConnectionGroupCount();
            for (uint i = 0; i < nNumGroups; ++i)
            {
                XmlNodeRef pGroupNode = pChildNode->createNode(Audio::ATLXmlTags::ATLConfigGroupTag);
                AZStd::string sGroupName = m_pATLModel->GetConnectionGroupAt(i);
                pGroupNode->setAttr(Audio::ATLXmlTags::ATLNameAttribute, sGroupName.c_str());
                WriteConnectionsToXml(pGroupNode, pControl, sGroupName);
                if (pGroupNode->getChildCount() > 0)
                {
                    pChildNode->addChild(pGroupNode);
                }
            }
        }
        else
        {
            WriteConnectionsToXml(pChildNode, pControl);
        }

        pNode->addChild(pChildNode);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteConnectionsToXml(XmlNodeRef pNode, CATLControl* pControl, const AZStd::string& sGroup)
    {
        if (pControl && m_pAudioSystemImpl)
        {
            TXmlNodeList defaultNodes;
            TXmlNodeList& otherNodes = defaultNodes;
            auto itNodes = pControl->m_connectionNodes.find(sGroup);
            if (itNodes != pControl->m_connectionNodes.end())
            {
                otherNodes = itNodes->second;
            }

            TXmlNodeList::const_iterator end = AZStd::remove_if(otherNodes.begin(), otherNodes.end(), [](const SRawConnectionData& node) { return node.m_isValid; });
            otherNodes.erase(end, otherNodes.end());

            TXmlNodeList::const_iterator it = otherNodes.begin();
            end = otherNodes.end();
            for (; it != end; ++it)
            {
                pNode->addChild(it->m_xmlNode);
            }

            const int size = pControl->ConnectionCount();
            for (int i = 0; i < size; ++i)
            {
                TConnectionPtr pConnection = pControl->GetConnectionAt(i);
                if (pConnection)
                {
                    if (pConnection->GetGroup() == sGroup)
                    {
                        XmlNodeRef pChild = m_pAudioSystemImpl->CreateXMLNodeFromConnection(pConnection, pControl->GetType());
                        if (pChild)
                        {
                            pNode->addChild(pChild);
                            pControl->m_connectionNodes[sGroup].push_back(SRawConnectionData(pChild, true));
                        }
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WritePlatformsToXml(XmlNodeRef pNode, CATLControl* pControl)
    {
        XmlNodeRef pPlatformsNode = pNode->createNode(Audio::ATLXmlTags::ATLPlatformsTag);
        uint nNumPlatforms = m_pATLModel->GetPlatformCount();
        for (uint j = 0; j < nNumPlatforms; ++j)
        {
            XmlNodeRef pPlatform = pPlatformsNode->createNode("Platform");
            AZStd::string sPlatformName = m_pATLModel->GetPlatformAt(j);
            pPlatform->setAttr(Audio::ATLXmlTags::ATLNameAttribute, sPlatformName.c_str());
            pPlatform->setAttr(Audio::ATLXmlTags::ATLConfigGroupAttribute, m_pATLModel->GetConnectionGroupAt(pControl->GetGroupForPlatform(sPlatformName)).c_str());
            pPlatformsNode->addChild(pPlatform);
        }
        pNode->addChild(pPlatformsNode);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::CheckOutFile(const AZStd::string& filepath)
    {
        IEditor* pEditor = GetIEditor();
        IFileUtil* pFileUtil = pEditor ? pEditor->GetFileUtil() : nullptr;
        if (pFileUtil)
        {
            pFileUtil->CheckoutFile(filepath.c_str(), nullptr);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::DeleteLibraryFile(const AZStd::string& filepath)
    {
        IEditor* pEditor = GetIEditor();
        IFileUtil* pFileUtil = pEditor ? pEditor->GetFileUtil() : nullptr;
        if (pFileUtil)
        {
            pFileUtil->DeleteFromSourceControl(filepath.c_str(), nullptr);
        }
    }
} // namespace AudioControls
