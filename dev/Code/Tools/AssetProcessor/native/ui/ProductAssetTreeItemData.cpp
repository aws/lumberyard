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
#include "ProductAssetTreeItemData.h"

#include <AzCore/std/smart_ptr/make_shared.h>

namespace AssetProcessor
{

    AZStd::shared_ptr<ProductAssetTreeItemData> ProductAssetTreeItemData::MakeShared(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo, const AZStd::string& assetDbName, QString name, bool isFolder, const AZ::Uuid& uuid)
    {
        return AZStd::make_shared<ProductAssetTreeItemData>(databaseInfo, assetDbName, name, isFolder, uuid);
    }

    ProductAssetTreeItemData::ProductAssetTreeItemData(
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder,
        const AZ::Uuid& uuid) :
        AssetTreeItemData(assetDbName, name, isFolder, uuid)
    {
        if (databaseInfo)
        {
            m_hasDatabaseInfo = true;
            m_databaseInfo = *databaseInfo;
        }
        else
        {
            m_hasDatabaseInfo = false;
        }

    }
}
