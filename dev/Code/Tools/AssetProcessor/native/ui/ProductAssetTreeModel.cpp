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

#include "ProductAssetTreeModel.h"
#include "ProductAssetTreeItemData.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <native/utilities/assetUtils.h>

namespace AssetProcessor
{

    ProductAssetTreeModel::ProductAssetTreeModel(QObject *parent) :
        AssetTreeModel(parent)
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Handler::BusConnect();
    }

    ProductAssetTreeModel::~ProductAssetTreeModel()
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Handler::BusDisconnect();
    }

    void ProductAssetTreeModel::ResetModel()
    {
        m_productToTreeItem.clear();
        m_productIdToTreeItem.clear();
        AZStd::string databaseLocation;
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Broadcast(&AzToolsFramework::AssetDatabase::AssetDatabaseRequests::GetAssetDatabaseLocation, databaseLocation);

        if (databaseLocation.empty())
        {
            return;
        }

        m_dbConnection.QueryProductsTable(
            [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product)
        {
            AddOrUpdateEntry(product, true);
            return true; // return true to continue iterating over additional results, we are populating a container
        });
    }

    void ProductAssetTreeModel::OnProductFileChanged(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
    {
        // Model changes need to be run on the main thread.
        AZ::SystemTickBus::QueueFunction([&, entry]()
        {
            AddOrUpdateEntry(entry, false);
        });
    }

    void ProductAssetTreeModel::RemoveAsset(AZ::s64 productId)
    {
        auto existingProduct = m_productIdToTreeItem.find(productId);
        if (existingProduct == m_productIdToTreeItem.end() || !existingProduct->second)
        {
            // If the product being removed wasn't cached, then something has gone wrong. Reset the model.
            Reset();
            return;
        }
        RemoveAssetTreeItem(existingProduct->second);
    }

    void ProductAssetTreeModel::RemoveAssetTreeItem(AssetTreeItem* assetToRemove)
    {
        if (!assetToRemove)
        {
            return;
        }
        AssetTreeItem* parent = assetToRemove->GetParent();
        QModelIndex parentIndex = createIndex(parent->GetRow(), 0, parent);

        beginRemoveRows(parentIndex, assetToRemove->GetRow(), assetToRemove->GetRow());

        m_productToTreeItem.erase(assetToRemove->GetData()->m_assetDbName);
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(assetToRemove->GetData());
        if (productItemData && productItemData->m_hasDatabaseInfo)
        {
            m_productIdToTreeItem.erase(productItemData->m_databaseInfo.m_productID);
        }
        parent->EraseChild(assetToRemove);

        endRemoveRows();

        RemoveFoldersIfEmpty(parent);
    }

    void ProductAssetTreeModel::RemoveFoldersIfEmpty(AssetTreeItem* itemToCheck)
    {
        // Don't attempt to remove invalid items, non-folders, folders that still have items in them, or the root.
        if (!itemToCheck || !itemToCheck->GetData()->m_isFolder || itemToCheck->getChildCount() > 0 || !itemToCheck->GetParent())
        {
            return;
        }
        RemoveAssetTreeItem(itemToCheck);
    }

    void ProductAssetTreeModel::OnProductFileRemoved(AZ::s64 productId)
    {
        // UI changes need to be done on the main thread.
        AZ::SystemTickBus::QueueFunction([&, productId]()
        {
            RemoveAsset(productId);
        });
    }

    void ProductAssetTreeModel::OnProductFilesRemoved(const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& products)
    {
        // UI changes need to be done on the main thread.
        AZ::SystemTickBus::QueueFunction([&, products]()
        {
            for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
            {
                RemoveAsset(product.m_productID);
            }
        });
    }

    QModelIndex ProductAssetTreeModel::GetIndexForProduct(const AZStd::string& product)
    {
        auto productItem = m_productToTreeItem.find(product);
        if (productItem == m_productToTreeItem.end())
        {
            return QModelIndex();
        }
        return createIndex(productItem->second->GetRow(), 0, productItem->second);
    }

    void ProductAssetTreeModel::AddOrUpdateEntry(
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product,
        bool modelIsResetting)
    {
        AZStd::vector<AZStd::string> tokens;
        AzFramework::StringFunc::Tokenize(product.m_productName.c_str(), tokens, AZ_CORRECT_DATABASE_SEPARATOR, false, true);

        if (tokens.empty())
        {
            AZ_Warning("AssetProcessor", false, "Product id %d has an invalid name: %s", product.m_productID, product.m_productName.c_str());
            return;
        }

        AssetTreeItem* parentItem = m_root.get();
        AZStd::string fullFolderName;
        for (int i = 0; i < tokens.size() - 1; ++i)
        {
            AzFramework::StringFunc::AssetDatabasePath::Join(fullFolderName.c_str(), tokens[i].c_str(), fullFolderName);
            AssetTreeItem* nextParent = parentItem->GetChildFolder(tokens[i].c_str());
            if (!nextParent)
            {
                if (!modelIsResetting)
                {
                    QModelIndex parentIndex = parentItem == m_root.get() ? QModelIndex() : createIndex(parentItem->GetRow(), 0, parentItem);
                    beginInsertRows(parentIndex, parentItem->getChildCount(), parentItem->getChildCount());
                }
                nextParent = parentItem->CreateChild(ProductAssetTreeItemData::MakeShared(nullptr, fullFolderName, tokens[i].c_str(), true, AZ::Uuid::CreateNull()));
                m_productToTreeItem[fullFolderName] = nextParent;
                // m_productIdToTreeItem is not used for folders, folders don't have product IDs.

                if (!modelIsResetting)
                {
                    endInsertRows();
                }
            }
            parentItem = nextParent;
        }

        AZ::Uuid sourceId;
        m_dbConnection.QuerySourceByProductID(
            product.m_productID,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
        {
            sourceId = sourceEntry.m_sourceGuid;
            return true;
        });

        if (!modelIsResetting)
        {
            QModelIndex parentIndex = parentItem == m_root.get() ? QModelIndex() : createIndex(parentItem->GetRow(), 0, parentItem);
            beginInsertRows(parentIndex, parentItem->getChildCount(), parentItem->getChildCount());
        }

        m_productToTreeItem[product.m_productName] =
            parentItem->CreateChild(ProductAssetTreeItemData::MakeShared(&product, product.m_productName, tokens[tokens.size() - 1].c_str(), false, sourceId));
        m_productIdToTreeItem[product.m_productID] = m_productToTreeItem[product.m_productName];

        if (!modelIsResetting)
        {
            endInsertRows();
        }
    }
}
