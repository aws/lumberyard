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
#include "AudioControlsWriter.h"
#include <StringUtils.h>
#include <CryFile.h>
#include <ISystem.h>
#include <ATLControlsModel.h>
#include <ISourceControl.h>
#include <IEditor.h>
#include <IAudioSystem.h>
#include <IAudioSystemEditor.h>
#include <IAudioSystemControl.h>
#include <QtUtil.h>
#include <Util/PathUtil.h>

#include <QModelIndex>
#include <QStandardItemModel>
#include <QFileInfo>

#include <IFileUtil.h>

using namespace PathUtil;

namespace AudioControls
{
    const string CAudioControlsWriter::ms_sLevelsFolder = "levels";
    const string CAudioControlsWriter::ms_sLibraryExtension = ".xml";

    //-------------------------------------------------------------------------------------------//
    string TypeToTag(EACEControlType eType)
    {
        switch (eType)
        {
        case eACET_RTPC:
            return Audio::SATLXmlTags::sATLRtpcTag;
        case eACET_TRIGGER:
            return Audio::SATLXmlTags::sATLTriggerTag;
        case eACET_SWITCH:
            return Audio::SATLXmlTags::sATLSwitchTag;
        case eACET_SWITCH_STATE:
            return Audio::SATLXmlTags::sATLSwitchStateTag;
        case eACET_PRELOAD:
            return Audio::SATLXmlTags::sATLPreloadRequestTag;
        case eACET_ENVIRONMENT:
            return Audio::SATLXmlTags::sATLEnvironmentTag;
        }
        return "";
    }

    //-------------------------------------------------------------------------------------------//
    CAudioControlsWriter::CAudioControlsWriter(CATLControlsModel* pATLModel, QStandardItemModel* pLayoutModel, IAudioSystemEditor* pAudioSystemImpl, std::set<string>& previousLibraryPaths)
        : m_pATLModel(pATLModel)
        , m_pLayoutModel(pLayoutModel)
        , m_pAudioSystemImpl(pAudioSystemImpl)
    {
        if (pATLModel && pLayoutModel && pAudioSystemImpl)
        {
            m_pLayoutModel->blockSignals(true);

            int i = 0;
            QModelIndex index = pLayoutModel->index(i, 0);
            while (index.isValid())
            {
                WriteLibrary(QtUtil::ToString(index.data(Qt::DisplayRole).toString()), index);
                ++i;
                index = index.sibling(i, 0);
            }

            // Delete libraries that don't exist anymore from disk
            std::set<string> librariesToDelete;
            std::set_difference(previousLibraryPaths.begin(), previousLibraryPaths.end(), m_foundLibraryPaths.begin(), m_foundLibraryPaths.end(),
                std::inserter(librariesToDelete, librariesToDelete.begin()));

            for (auto it = librariesToDelete.begin(); it != librariesToDelete.end(); ++it)
            {
                DeleteLibraryFile((*it));
            }
            previousLibraryPaths = m_foundLibraryPaths;

            m_pLayoutModel->blockSignals(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteLibrary(const string& sLibraryName, QModelIndex root)
    {
        if (root.isValid())
        {
            TLibraryStorage library;
            int i = 0;
            QModelIndex child = root.child(0, 0);
            while (child.isValid())
            {
                WriteItem(child, "", library, root.data(eDR_MODIFIED).toBool());
                child = root.child(++i, 0);
            }

            const char* controlsPath = nullptr;
            Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);

            TLibraryStorage::const_iterator it = library.begin();
            TLibraryStorage::const_iterator end = library.end();
            for (; it != end; ++it)
            {
                string sLibraryPath;
                const string sScope = it->first;
                if (sScope.empty())
                {
                    // no scope, file at the root level
                    sLibraryPath = controlsPath + sLibraryName + ms_sLibraryExtension;
                }
                else
                {
                    // with scope, inside level folder
                    sLibraryPath = controlsPath + ms_sLevelsFolder + GetSlash() + sScope + GetSlash() + sLibraryName + ms_sLibraryExtension;
                }

                // should be able to change this back to GamePathToFullPath once a path normalization bug has been fixed:
                string sFullFilePath(Path::GetEditingGameDataFolder().c_str());
                sFullFilePath += GetSlash() + sLibraryPath;
                sFullFilePath.MakeLower();
                m_foundLibraryPaths.insert(sFullFilePath);

                const SLibraryScope& libScope = it->second;
                if (libScope.bDirty)
                {
                    XmlNodeRef pFileNode = GetISystem()->CreateXmlNode(Audio::SATLXmlTags::sRootNodeTag);
                    pFileNode->setAttr(Audio::SATLXmlTags::sATLNameAttribute, sLibraryName);

                    for (int i = 0; i < eACET_NUM_TYPES; ++i)
                    {
                        if (i != eACET_SWITCH_STATE) // switch_states are written inside the switches
                        {
                            if (libScope.pNodes[i]->getChildCount() > 0)
                            {
                                pFileNode->addChild(libScope.pNodes[i]);
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
                        pFileNode->saveToFile(sFullFilePath);
                    }
                    else
                    {
                        // save the file, CheckOutFile will add it, since it's new
                        pFileNode->saveToFile(sFullFilePath);
                        CheckOutFile(sFullFilePath);
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteItem(QModelIndex index, const string& path, TLibraryStorage& library, bool bParentModified)
    {
        if (index.isValid())
        {
            if (index.data(eDR_TYPE) == eIT_FOLDER)
            {
                int i = 0;
                QModelIndex child = index.child(0, 0);
                while (child.isValid())
                {
                    string sNewPath = path.empty() ? "" : path + "/";
                    sNewPath += QtUtil::ToString(index.data(Qt::DisplayRole).toString());
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
                        scope.bDirty = true;
                        QStandardItem* pItem = m_pLayoutModel->itemFromIndex(index);
                        if (pItem)
                        {
                            pItem->setData(false, eDR_MODIFIED);
                        }
                    }
                    WriteControlToXML(scope.pNodes[pControl->GetType()], pControl, path);
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
        QModelIndex child = index.child(0, 0);
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
    void CAudioControlsWriter::WriteControlToXML(XmlNodeRef pNode, CATLControl* pControl, const string& sPath)
    {
        const EACEControlType type = pControl->GetType();
        XmlNodeRef pChildNode = pNode->createNode(TypeToTag(type));
        pChildNode->setAttr(Audio::SATLXmlTags::sATLNameAttribute, pControl->GetName());
        if (!sPath.empty())
        {
            pChildNode->setAttr("path", sPath);
        }

        if (type == eACET_SWITCH)
        {
            const size_t size = pControl->ChildCount();
            for (size_t i = 0; i < size; ++i)
            {
                WriteControlToXML(pChildNode, pControl->GetChild(i), "");
            }
        }
        else if (type == eACET_PRELOAD)
        {
            if (pControl->IsAutoLoad())
            {
                pChildNode->setAttr(Audio::SATLXmlTags::sATLTypeAttribute, Audio::SATLXmlTags::sATLDataLoadType);
            }
            WritePlatformsToXML(pChildNode, pControl);
            uint nNumGroups = m_pATLModel->GetConnectionGroupCount();
            for (uint i = 0; i < nNumGroups; ++i)
            {
                XmlNodeRef pGroupNode = pChildNode->createNode(Audio::SATLXmlTags::sATLConfigGroupTag);
                string sGroupName = m_pATLModel->GetConnectionGroupAt(i);
                pGroupNode->setAttr(Audio::SATLXmlTags::sATLNameAttribute, sGroupName);
                WriteConnectionsToXML(pGroupNode, pControl, sGroupName);
                if (pGroupNode->getChildCount() > 0)
                {
                    pChildNode->addChild(pGroupNode);
                }
            }
        }
        else
        {
            WriteConnectionsToXML(pChildNode, pControl);
        }

        pNode->addChild(pChildNode);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::WriteConnectionsToXML(XmlNodeRef pNode, CATLControl* pControl, const string& sGroup)
    {
        if (pControl && m_pAudioSystemImpl)
        {
            TXMLNodeList defaultNodes;
            TXMLNodeList& otherNodes = stl::find_in_map_ref(pControl->m_connectionNodes, sGroup, defaultNodes);

            TXMLNodeList::const_iterator end = std::remove_if(otherNodes.begin(), otherNodes.end(), [](const SRawConnectionData& node) { return node.bValid; });
            otherNodes.erase(end, otherNodes.end());

            TXMLNodeList::const_iterator it = otherNodes.begin();
            end = otherNodes.end();
            for (; it != end; ++it)
            {
                pNode->addChild(it->xmlNode);
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
    void CAudioControlsWriter::WritePlatformsToXML(XmlNodeRef pNode, CATLControl* pControl)
    {
        XmlNodeRef pPlatformsNode = pNode->createNode(Audio::SATLXmlTags::sATLPlatformsTag);
        uint nNumPlatforms = m_pATLModel->GetPlatformCount();
        for (uint j = 0; j < nNumPlatforms; ++j)
        {
            XmlNodeRef pPlatform = pPlatformsNode->createNode("Platform");
            string sPlatformName = m_pATLModel->GetPlatformAt(j);
            pPlatform->setAttr(Audio::SATLXmlTags::sATLNameAttribute, sPlatformName);
            pPlatform->setAttr(Audio::SATLXmlTags::sATLConfigGroupAttribute, m_pATLModel->GetConnectionGroupAt(pControl->GetGroupForPlatform(sPlatformName)));
            pPlatformsNode->addChild(pPlatform);
        }
        pNode->addChild(pPlatformsNode);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::CheckOutFile(const string& filepath)
    {
        IEditor* pEditor = GetIEditor();
        IFileUtil* pFileUtil = pEditor ? pEditor->GetFileUtil() : nullptr;
        if (pFileUtil)
        {
            pFileUtil->CheckoutFile(filepath.c_str(), nullptr);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsWriter::DeleteLibraryFile(const string& filepath)
    {
        IEditor* pEditor = GetIEditor();
        IFileUtil* pFileUtil = pEditor ? pEditor->GetFileUtil() : nullptr;
        if (pFileUtil)
        {
            pFileUtil->DeleteFromSourceControl(filepath.c_str(), nullptr);
        }
    }
} // namespace AudioControls
