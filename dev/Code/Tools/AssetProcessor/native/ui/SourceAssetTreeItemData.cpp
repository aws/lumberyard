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
#include "SourceAssetTreeItemData.h"

#include <AzCore/std/smart_ptr/make_shared.h>

namespace AssetProcessor
{

    AZStd::shared_ptr<SourceAssetTreeItemData> SourceAssetTreeItemData::MakeShared(
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry* sourceInfo,
        const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry* scanFolderInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder)
    {
        return AZStd::make_shared<SourceAssetTreeItemData>(sourceInfo, scanFolderInfo, assetDbName, name, isFolder);
    }

    SourceAssetTreeItemData::SourceAssetTreeItemData(
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry* sourceInfo,
        const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry* scanFolderInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder) :
        AssetTreeItemData(assetDbName, name, isFolder, sourceInfo ? sourceInfo->m_sourceGuid : AZ::Uuid::CreateNull())
    {
        if (sourceInfo && scanFolderInfo)
        {
            m_hasDatabaseInfo = true;
            m_sourceInfo = *sourceInfo;
            m_scanFolderInfo = *scanFolderInfo;
        }
        else
        {
            m_hasDatabaseInfo = false;
        }

    }
}
