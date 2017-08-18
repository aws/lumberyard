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

#include "StdAfx.h"
#include <AssetBrowserContextProvider.h>
#include <AssetImporterPlugin.h>
#include <QtCore/QMimeData.h>
#include <QMenu>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h> // for ebus events
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

namespace AZ
{
    AssetBrowserContextProvider::AssetBrowserContextProvider()
    {
        BusConnect();
    }
    
    AssetBrowserContextProvider::~AssetBrowserContextProvider()
    {
        BusDisconnect();
    }

    void AssetBrowserContextProvider::StartDrag(QMimeData* /*data*/)
    {
    }

    void AssetBrowserContextProvider::AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<AssetBrowserEntry*>& entries)
    {
        auto entryIt = AZStd::find_if(entries.begin(), entries.end(), 
            [](const AssetBrowserEntry* entry) -> bool { return entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source; });
        if (entryIt == entries.end())
        {
            return;
        }
        SourceAssetBrowserEntry* source = azrtti_cast<SourceAssetBrowserEntry*>(*entryIt);
        
        AZStd::unordered_set<AZStd::string> extensions;
        EBUS_EVENT(AZ::SceneAPI::Events::AssetImportRequestBus, GetSupportedFileExtensions, extensions);
        if (extensions.empty())
        {
            return;
        }

        AZStd::string sourcePath = source->GetFullPath();
        AZStd::string targetExtension = source->GetExtension();
        
        for (AZStd::string potentialExtension : extensions)
        {
            const char* extension = potentialExtension.c_str();

            if (AzFramework::StringFunc::Equal(extension, targetExtension.c_str()))
            {
                QAction* editImportSettingsAction = menu->addAction("Edit Settings...", [sourcePath]()
                {
                    AssetImporterPlugin::GetInstance()->EditImportSettings(sourcePath);
                });

                using namespace AzToolsFramework;
                EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBus::Events::RegisterAction, editImportSettingsAction, QString("AssetBrowserContextMenu EditImportSettings"));
            }
        }
    }
}
