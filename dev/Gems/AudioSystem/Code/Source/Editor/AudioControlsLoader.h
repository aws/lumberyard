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

#include <AzCore/std/containers/set.h>

#include <ACETypes.h>
#include <AudioControl.h>
#include <IAudioConnection.h>

#include <IXml.h>

#include <QString>

class QStandardItemModel;
class QStandardItem;

namespace AudioControls
{
    class CATLControlsModel;
    class IAudioSystemEditor;

    //-------------------------------------------------------------------------------------------//
    class CAudioControlsLoader
    {
    public:
        CAudioControlsLoader(CATLControlsModel* pATLModel, QStandardItemModel* pLayoutModel, IAudioSystemEditor* pAudioSystemImpl);
        AZStd::set<AZStd::string> GetLoadedFilenamesList();
        void LoadAll();
        void LoadControls();
        void LoadScopes();

    private:
        void LoadAllLibrariesInFolder(const AZStd::string_view folderPath, const AZStd::string_view level);
        void LoadControlsLibrary(XmlNodeRef pRoot, const AZStd::string_view sFilepath, const AZStd::string_view sLevel, const AZStd::string_view sFilename);
        CATLControl* LoadControl(XmlNodeRef pNode, QStandardItem* pFolder, const AZStd::string_view sScope);

        void LoadPreloadConnections(XmlNodeRef pNode, CATLControl* pControl);
        void LoadConnections(XmlNodeRef root, CATLControl* pControl);

        void CreateDefaultControls();
        QStandardItem* AddControl(CATLControl* pControl, QStandardItem* pFolder);

        void LoadSettings();
        void LoadScopesImpl(const AZStd::string_view path);

        QStandardItem* AddUniqueFolderPath(QStandardItem* pParent, const QString& sPath);
        QStandardItem* AddFolder(QStandardItem* pParent, const QString& sName);

        CATLControl* CreateInternalSwitchState(CATLControl* parentControl, const AZStd::string& switchName, const AZStd::string& stateName);

        CATLControlsModel* m_pModel;
        QStandardItemModel* m_pLayout;
        IAudioSystemEditor* m_pAudioSystemImpl;
        AZStd::set<AZStd::string> m_loadedFilenames;
    };
} // namespace AudioControls
