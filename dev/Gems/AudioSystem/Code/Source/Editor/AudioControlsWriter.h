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

#pragma once

#include <CryString.h>
#include <IAudioConnection.h>
#include <AudioControl.h>
#include <ATLCommon.h>
#include <IXml.h>
#include <QModelIndex>
#include <ISystem.h>

class QStandardItemModel;

namespace AudioControls
{
    class CATLControlsModel;
    class IAudioSystemEditor;

    //-------------------------------------------------------------------------------------------//
    struct SLibraryScope
    {
        SLibraryScope()
            : bDirty(false)
        {
            pNodes[eACET_TRIGGER] = GetISystem()->CreateXmlNode(Audio::SATLXmlTags::sTriggersNodeTag);
            pNodes[eACET_RTPC] = GetISystem()->CreateXmlNode(Audio::SATLXmlTags::sRtpcsNodeTag);
            pNodes[eACET_SWITCH] = GetISystem()->CreateXmlNode(Audio::SATLXmlTags::sSwitchesNodeTag);
            pNodes[eACET_ENVIRONMENT] = GetISystem()->CreateXmlNode(Audio::SATLXmlTags::sEnvironmentsNodeTag);
            pNodes[eACET_PRELOAD] = GetISystem()->CreateXmlNode(Audio::SATLXmlTags::sPreloadsNodeTag);
        }
        XmlNodeRef pNodes[eACET_NUM_TYPES];
        bool bDirty;
    };

    typedef std::map<string, SLibraryScope> TLibraryStorage;

    //-------------------------------------------------------------------------------------------//
    class CAudioControlsWriter
    {
    public:
        CAudioControlsWriter(CATLControlsModel* pATLModel, QStandardItemModel* pLayoutModel, IAudioSystemEditor* pAudioSystemImpl, std::set<string>& previousLibraryPaths);

    private:
        void WriteLibrary(const string& sLibraryName, QModelIndex root);
        void WriteItem(QModelIndex index, const string& sPath, TLibraryStorage& library, bool bParentModified);
        void WriteControlToXML(XmlNodeRef pNode, CATLControl* pControl, const string& sPath);
        void WriteConnectionsToXML(XmlNodeRef pNode, CATLControl* pControl, const string& sGroup = "");
        void WritePlatformsToXML(XmlNodeRef pNode, CATLControl* pControl);
        bool IsItemModified(QModelIndex index);

        void CheckOutFile(const string& filepath);
        void DeleteLibraryFile(const string& filepath);

        CATLControlsModel* m_pATLModel;
        QStandardItemModel* m_pLayoutModel;
        IAudioSystemEditor* m_pAudioSystemImpl;

        std::set<string> m_foundLibraryPaths;

        static const char* ms_sLevelsFolder;
        static const char* ms_sLibraryExtension;
    };
} // namespace AudioControls
