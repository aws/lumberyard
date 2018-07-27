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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/FolderThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntryCache.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QVariant>
#include <QMimeData>
#include <QUrl>

using namespace AzFramework;
using namespace AzToolsFramework::AssetDatabase;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {

        const char* GEMS_FOLDER_NAME = "Gems";


        QString AssetBrowserEntry::AssetEntryTypeToString(AssetEntryType assetEntryType)
        {
            switch (assetEntryType)
            {
            case AssetEntryType::Root:
                return QObject::tr("Root");
            case AssetEntryType::Folder:
                return QObject::tr("Folder");
            case AssetEntryType::Source:
                return QObject::tr("Source");
            case AssetEntryType::Product:
                return QObject::tr("Product");
            default:
                return QObject::tr("Unknown");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetBrowserEntry
        //////////////////////////////////////////////////////////////////////////
        const char* AssetBrowserEntry::m_columnNames[] =
        {
            "Name",
            "Source ID",
            "Fingerprint",
            "Guid",
            "ScanFolder ID",
            "Product ID",
            "Job ID",
            "Job Key",
            "Sub ID",
            "Asset Type",
            "Platform",
            "Class ID",
            "Display Name"
        };

        AssetBrowserEntry::AssetBrowserEntry()
            : AssetBrowserEntry("")
        {}

        AssetBrowserEntry::AssetBrowserEntry(const char* name)
            : QObject()
            , m_name(name)
            , m_displayName(name)
        {
        }

        AssetBrowserEntry::~AssetBrowserEntry()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_dirtyThumbnailsSet.erase(this);
            }
            RemoveChildren();
        }

        void AssetBrowserEntry::AddChild(AssetBrowserEntry* child)
        {
            child->m_parentAssetEntry = this;

            UpdateChildPaths(child);

            AssetBrowserModelRequestBus::Broadcast(&AssetBrowserModelRequests::BeginAddEntry, this);
            child->m_row = static_cast<int>(m_children.size());
            m_children.push_back(child);
            AssetBrowserModelRequestBus::Broadcast(&AssetBrowserModelRequests::EndAddEntry, this);
            AssetBrowserModelNotificationBus::Broadcast(&AssetBrowserModelNotifications::EntryAdded, child);
        }

        void AssetBrowserEntry::RemoveChild(AssetBrowserEntry* child)
        {
            if (!child)
            {
                return;
            }

            if (child->m_row >= m_children.size())
            {
                return;
            }

            if (child != m_children[child->m_row])
            {
                return;
            }

            AssetBrowserModelRequestBus::Broadcast(&AssetBrowserModelRequests::BeginRemoveEntry, child);
            auto it = m_children.erase(m_children.begin() + child->m_row);

            // decrement the row of all children after the removed child
            while (it != m_children.end())
            {
                (*it++)->m_row--;
            }

            child->m_parentAssetEntry = nullptr;
            AssetBrowserModelRequestBus::Broadcast(&AssetBrowserModelRequests::EndRemoveEntry);
            AssetBrowserModelNotificationBus::Broadcast(&AssetBrowserModelNotifications::EntryRemoved, child);

            delete child;
        }

        void AssetBrowserEntry::RemoveChildren()
        {
            while (!m_children.empty())
            {
                // child entries are removed from the end of the list, because this will incur minimum effort to update their rows
                RemoveChild(*m_children.rbegin());
            }
        }

        QVariant AssetBrowserEntry::data(int column) const
        {
            switch (static_cast<Column>(column))
            {
            case Column::Name:
                return QString::fromUtf8(m_name.c_str());
            case Column::DisplayName:
                return QString::fromUtf8(m_displayName.c_str());
            default:
                return QVariant();
            }
        }

        int AssetBrowserEntry::row() const
        {
            return m_row;
        }

        bool AssetBrowserEntry::FromMimeData(const QMimeData* mimeData, AZStd::vector<AssetBrowserEntry*>& entries)
        {
            if (!mimeData)
            {
                return false;
            }

            for (auto format : mimeData->formats())
            {
                if (format != GetMimeType())
                {
                    continue;
                }

                QByteArray arrayData = mimeData->data(format);
                AZ::IO::MemoryStream ms(arrayData.constData(), arrayData.size());
                AssetBrowserEntry* entry = AZ::Utils::LoadObjectFromStream<AssetBrowserEntry>(ms, nullptr);
                if (entry)
                {
                    entries.push_back(entry);
                }
            }
            return entries.size() > 0;
        }

        void AssetBrowserEntry::AddToMimeData(QMimeData* mimeData) const
        {
            if (!mimeData)
            {
                return;
            }

            AZStd::vector<char> buffer;

            AZ::IO::ByteContainerStream<AZStd::vector<char> > byteStream(&buffer);
            AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, this, this->RTTI_GetType());

            QByteArray dataArray(buffer.data(), static_cast<int>(sizeof(char) * buffer.size()));
            mimeData->setData(GetMimeType(), dataArray);
            mimeData->setUrls({ QUrl::fromLocalFile(GetFullPath().c_str()) });
        }

        QString AssetBrowserEntry::GetMimeType()
        {
            return "editor/assetinformation/entry";
        }

        void AssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<AssetBrowserEntry>()
                    ->Field("m_name", &AssetBrowserEntry::m_name)
                    ->Field("m_children", &AssetBrowserEntry::m_children)
                    ->Field("m_row", &AssetBrowserEntry::m_row)
                    ->Version(1);
            }
        }

        const AZStd::string& AssetBrowserEntry::GetName() const
        {
            return m_name;
        }

        const AZStd::string& AssetBrowserEntry::GetDisplayName() const
        {
            return m_displayName;
        }

        const AZStd::string& AssetBrowserEntry::GetRelativePath() const
        {
            return m_relativePath;
        }

        const AZStd::string& AssetBrowserEntry::GetFullPath() const
        {
            return m_fullPath;
        }

        const AssetBrowserEntry* AssetBrowserEntry::GetChild(int index) const
        {
            if (m_children.size() < index)
            {
                return nullptr;
            }
            return m_children[index];
        }

        int AssetBrowserEntry::GetChildCount() const
        {
            return static_cast<int>(m_children.size());
        }

        AssetBrowserEntry* AssetBrowserEntry::GetParent() const
        {
            return m_parentAssetEntry;
        }

        void AssetBrowserEntry::SetThumbnailKey(SharedThumbnailKey thumbnailKey)
        {
            if (m_thumbnailKey)
            {
                disconnect(m_thumbnailKey.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &AssetBrowserEntry::ThumbnailUpdated);
            }
            m_thumbnailKey = thumbnailKey;
            connect(m_thumbnailKey.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &AssetBrowserEntry::ThumbnailUpdated);
        }

        SharedThumbnailKey AssetBrowserEntry::GetThumbnailKey() const
        {
            return m_thumbnailKey;
        }

        void AssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            child->PathsUpdated();
        }

        void AssetBrowserEntry::PathsUpdated()
        {
            SetThumbnailKey(CreateThumbnailKey());
        }

        void AssetBrowserEntry::ThumbnailUpdated()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_dirtyThumbnailsSet.insert(this);
            }
        }
        //////////////////////////////////////////////////////////////////////////
        // RootAssetBrowserEntry
        //////////////////////////////////////////////////////////////////////////
        RootAssetBrowserEntry::RootAssetBrowserEntry()
            : AssetBrowserEntry("(ROOT)")
        {
        }

        void RootAssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<RootAssetBrowserEntry>()
                    ->Version(1);
            }
        }

        AssetBrowserEntry::AssetEntryType RootAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Root;
        }

        void RootAssetBrowserEntry::Update(const char* devPath)
        {
            RemoveChildren();

            m_devPath = devPath;

            // there is no "Gems" scan folder registered in db, create one manually
            auto gemFolder = CreateFolder((m_devPath + AZ_CORRECT_DATABASE_SEPARATOR + GEMS_FOLDER_NAME).c_str(), this, true, true);
            gemFolder->m_displayName = GEMS_FOLDER_NAME;
        }

        void RootAssetBrowserEntry::RemoveProduct(const AZ::Data::AssetId& assetId)
        {
            auto product = ProductAssetBrowserEntry::GetProductByAssetId(assetId);
            if (!product)
            {
                return;
            }
            RemoveEntry(product);
        }

        void RootAssetBrowserEntry::RemoveSource(const AZ::Uuid& sourceUuid)
        {
            AssetBrowserEntry* source = nullptr;
            // avoid having to const-cast here, look it up directly in the (internal/private) cache
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                source = cache->m_sourceIdMap[sourceUuid];
            }

            if (!source)
            {
                return;
            }

            RemoveEntry(source);
        }

        void RootAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            child->m_relativePath = child->m_name;
            child->m_fullPath = child->m_name;
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        SharedThumbnailKey RootAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(ThumbnailKey);
        }

        AssetBrowserEntry* RootAssetBrowserEntry::CreateFolder(const char* folderName, AssetBrowserEntry* parent, bool isScanFolder, bool isGemsFolder)
        {
            auto it = AZStd::find_if(parent->m_children.begin(), parent->m_children.end(), [folderName](AssetBrowserEntry* entry)
                    {
                        return StringFunc::Equal(entry->m_name.c_str(), folderName);
                    });
            if (it != parent->m_children.end())
            {
                return *it;
            }
            AssetBrowserEntry* folder = aznew FolderAssetBrowserEntry(folderName, isScanFolder, isGemsFolder);
            parent->AddChild(folder);
            return folder;
        }

        AssetBrowserEntry* RootAssetBrowserEntry::CreateFolders(const char* relativePath, AssetBrowserEntry* parent, bool isScanFolder, bool isGemsFolder)
        {
            auto children(parent->m_children);
            int n = 0;

            // check if folder with the same name already exists
            // step through every character in relativePath and compare to each child's relative path of suggested parent
            // if a character @n in child's rel path mismatches character at n in relativePath, remove that child from further search
            while (!children.empty() && relativePath[n])
            {
                AZStd::vector<AssetBrowserEntry*> toRemove;
                for (auto child : children)
                {
                    auto& childPath = azrtti_istypeof<RootAssetBrowserEntry*>(parent) ? child->m_fullPath : child->m_relativePath;

                    // child's path mismatched, remove it from search candidates
                    if (childPath.length() == n || childPath[n] != relativePath[n])
                    {
                        toRemove.push_back(child);

                        // it is possible that child may be a closer parent, substitute it as new potential parent
                        // e.g. child->m_relativePath = 'Gems', relativePath = 'Gems/Assets', old parent = root, new parent = Gems
                        if (childPath.length() == n && relativePath[n] == AZ_CORRECT_DATABASE_SEPARATOR)
                        {
                            parent = child;
                            relativePath += n; // advance relative path n characters since the parent has changed
                            n = 0; // Once the relative path pointer is advanced, reset n
                        }
                    }
                }
                for (auto entry : toRemove)
                {
                    children.erase(AZStd::remove(children.begin(), children.end(), entry), children.end());
                }
                n++;
            }

            // filter out the remaining children that don't end with '/' or '\0'
            // for example if folderName = "foo", while children may still remain with names like "foo123",
            // which is not the same folder
            AZStd::vector<AssetBrowserEntry*> toRemove;
            for (auto child : children)
            {
                auto& childPath = azrtti_istypeof<RootAssetBrowserEntry*>(parent) ? child->m_fullPath : child->m_relativePath;
                // check if there are non-null characters remaining @n
                if (childPath.length() > n)
                {
                    toRemove.push_back(child);
                }
            }
            for (auto entry : toRemove)
            {
                children.erase(AZStd::remove(children.begin(), children.end(), entry), children.end());
            }

            // at least one child remains, this means the folder with this name already exists, return it
            if (!children.empty())
            {
                parent = children.front();
            }
            // if it's a scanfolder, then do not create folders leading to it
            // e.g. instead of 'C:\dev\SampleProject' just create 'SampleProject'
            else if (parent->GetEntryType() == AssetEntryType::Root)
            {
                AZStd::string folderName;
                StringFunc::Path::Split(relativePath, nullptr, nullptr, &folderName);
                parent = CreateFolder(folderName.c_str(), parent, true);
                parent->m_fullPath = relativePath;
            }
            // otherwise create all missing folders
            else
            {
                n = 0;
                AZStd::string folderName(strlen(relativePath) + 1, '\0');
                // iterate through relativePath until the first '/'
                while (relativePath[n] && relativePath[n] != AZ_CORRECT_DATABASE_SEPARATOR)
                {
                    folderName[n] = relativePath[n];
                    n++;
                }
                if (n > 0)
                {
                    parent = CreateFolder(folderName.c_str(), parent, isScanFolder, isGemsFolder);
                }
                // n+1 also skips the '/' character
                if (relativePath[n] && relativePath[n + 1])
                {
                    parent = CreateFolders(relativePath + n + 1, parent);
                }
            }
            return parent;
        }

        void RootAssetBrowserEntry::AddAsset(const CombinedDatabaseEntry& combinedDatabaseEntry)
        {
            // do not create "dev" folder
            if (StringFunc::Equal(combinedDatabaseEntry.m_displayName.c_str(), "root"))
            {
                return;
            }

            auto relativePath = combinedDatabaseEntry.m_scanFolder;
            auto parent = CreateFolders(relativePath.c_str(), this, true);

            if (!combinedDatabaseEntry.m_displayName.empty())
            {
                // some folder names have slashes in them (e.g. Gems/Assets/...), displayName does not need to show entire path
                StringFunc::Path::Split(combinedDatabaseEntry.m_displayName.c_str(), nullptr, nullptr, &parent->m_displayName);
            }

            AZStd::string sourcePathWithoutOutputPrefix = combinedDatabaseEntry
                    .m_sourceName.substr(combinedDatabaseEntry.m_outputPrefix.length());
            StringFunc::AssetDatabasePath::Normalize(sourcePathWithoutOutputPrefix);
            if (sourcePathWithoutOutputPrefix[0] == AZ_CORRECT_DATABASE_SEPARATOR)
            {
                sourcePathWithoutOutputPrefix = sourcePathWithoutOutputPrefix.substr(1);
            }

            AZStd::string sourcePath;
            AZStd::string sourceName;
            AZStd::string sourceExtension;

            StringFunc::Path::Split(sourcePathWithoutOutputPrefix.c_str(), nullptr, &sourcePath, &sourceName, &sourceExtension);

            parent = CreateFolders(sourcePath.c_str(), parent);

            // if entry already exists don't add it
            AssetBrowserEntry* source = nullptr;
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                auto iteratorFound = cache->m_sourceIdMap.find(combinedDatabaseEntry.m_sourceGuid);
                if (iteratorFound != cache->m_sourceIdMap.end())
                {
                    source = iteratorFound->second;
                }
            }
          
            if (!source)
            {
                source = aznew SourceAssetBrowserEntry(
                        (sourceName + sourceExtension).c_str(),
                        sourceExtension.c_str(),
                        combinedDatabaseEntry.m_sourceID,
                        combinedDatabaseEntry.m_scanFolderID,
                        combinedDatabaseEntry.m_sourceGuid
                        );
                parent->AddChild(source);
            }
          

            if (combinedDatabaseEntry.m_productName.empty())
            {
                return;
            }

            // create product
            AZStd::string productPath;
            AZStd::string productName;
            AZStd::string productExtension;

            StringFunc::Path::Split(combinedDatabaseEntry.m_productName.c_str(), nullptr, &productPath, &productName, &productExtension);
            productName += productExtension;

            AZ::Data::AssetId productAssetId(combinedDatabaseEntry.m_sourceGuid, combinedDatabaseEntry.m_subID);

            // only add this if the product does not exist.
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                auto iteratorFound = cache->m_assetIdMap.find(productAssetId);
                if (iteratorFound != cache->m_assetIdMap.end())
                {
                    return;
                }
            }

            auto product = aznew ProductAssetBrowserEntry(
                    productName.c_str(),
                    combinedDatabaseEntry.m_fingerprint,
                    combinedDatabaseEntry.m_productID,
                    combinedDatabaseEntry.m_jobID,
                    combinedDatabaseEntry.m_jobKey.c_str(),
                    productAssetId,
                    combinedDatabaseEntry.m_assetType,
                    combinedDatabaseEntry.m_platform.c_str());
            source->AddChild(product);
        }

        void RootAssetBrowserEntry::RemoveEntry(AssetBrowserEntry* entry)
        {
            auto parent = entry->GetParent();
            if (!parent)
            {
                return;
            }

            parent->RemoveChild(entry);

            if (!parent->GetChildCount() && parent->GetEntryType() == AssetEntryType::Folder)
            {
                RemoveEntry(parent);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // FolderAssetBrowserEntry
        //////////////////////////////////////////////////////////////////////////
        FolderAssetBrowserEntry::FolderAssetBrowserEntry()
            : FolderAssetBrowserEntry("", false, false)
        {}

        FolderAssetBrowserEntry::FolderAssetBrowserEntry(const char* name, bool isScanFolder, bool isGemsFolder)
            : AssetBrowserEntry(name)
            , m_isScanFolder(isScanFolder)
            , m_isGemsFolder(isGemsFolder)
        {
        }

        void FolderAssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<FolderAssetBrowserEntry, AssetBrowserEntry>()
                    ->Field("m_isScanFolder", &FolderAssetBrowserEntry::m_isScanFolder)
                    ->Version(1);
            }
        }

        AssetBrowserEntry::AssetEntryType FolderAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Folder;
        }

        bool FolderAssetBrowserEntry::GetIsScanFolder() const
        {
            return m_isScanFolder;
        }

        void FolderAssetBrowserEntry::SetIsScanFolder(bool value)
        {
            m_isScanFolder = value;
        }

        bool FolderAssetBrowserEntry::IsGemsFolder() const
        {
            return m_isGemsFolder;
        }

        void FolderAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            child->m_relativePath = m_relativePath + AZ_CORRECT_DATABASE_SEPARATOR + child->m_name;
            child->m_fullPath = m_fullPath + AZ_CORRECT_DATABASE_SEPARATOR + child->m_name;
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        SharedThumbnailKey FolderAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(FolderThumbnailKey, m_fullPath.c_str(), IsGemsFolder());
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceAssetBrowserEntry
        //////////////////////////////////////////////////////////////////////////
        SourceAssetBrowserEntry::SourceAssetBrowserEntry()
            : AssetBrowserEntry()
            , m_sourceID(0)
            , m_scanFolderID(0) {}

        SourceAssetBrowserEntry::SourceAssetBrowserEntry(
            const char* name,
            const char* extension,
            AZ::s64 sourceID,
            AZ::s64 scanFolderID,
            const AZ::Uuid& sourceUuid)
            : AssetBrowserEntry(name)
            , m_extension(extension)
            , m_sourceID(sourceID)
            , m_scanFolderID(scanFolderID)
            , m_sourceUuid(sourceUuid)
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_sourceIdMap[sourceUuid] = this;
            }
        }

        SourceAssetBrowserEntry::~SourceAssetBrowserEntry()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_sourceIdMap.erase(m_sourceUuid);
            }
        }

        QVariant SourceAssetBrowserEntry::data(int column) const
        {
            switch (static_cast<Column>(column))
            {
            case Column::SourceID:
            {
                return QVariant(m_sourceID);
            }
            case Column::ScanFolderID:
            {
                return QVariant(m_scanFolderID);
            }
            default:
            {
                return AssetBrowserEntry::data(column);
            }
            }
        }

        void SourceAssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SourceAssetBrowserEntry, AssetBrowserEntry>()
                    ->Version(2)
                    ->Field("m_sourceID", &SourceAssetBrowserEntry::m_sourceID)
                    ->Field("m_scanFolderID", &SourceAssetBrowserEntry::m_scanFolderID)
                    ->Field("m_sourceUuid", &SourceAssetBrowserEntry::m_sourceUuid)
                    ->Field("m_extension", &SourceAssetBrowserEntry::m_extension);
            }
        }

        AssetBrowserEntry::AssetEntryType SourceAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Source;
        }
        const AZStd::string& SourceAssetBrowserEntry::GetExtension() const
        {
            return m_extension;
        }

        const AZ::Uuid& SourceAssetBrowserEntry::GetSourceUuid() const
        {
            return m_sourceUuid;
        }

        AZ::s64 SourceAssetBrowserEntry::GetSourceID() const
        {
            return m_sourceID;
        }

        AZ::s64 SourceAssetBrowserEntry::GetScanFolderID() const
        {
            return m_scanFolderID;
        }

        AZ::Data::AssetType SourceAssetBrowserEntry::GetPrimaryAssetType() const
        {
            AZStd::vector<const ProductAssetBrowserEntry*> products;
            GetChildren<ProductAssetBrowserEntry>(products);
            for (const ProductAssetBrowserEntry* product : products)
            {
                AZ::Data::AssetType productType = product->GetAssetType();
                if (productType != AZ::Data::s_invalidAssetType)
                {
                    return productType;
                }
            }

            return AZ::Data::s_invalidAssetType;
        }

        bool SourceAssetBrowserEntry::HasProductType(const AZ::Data::AssetType& assetType) const
        {
            AZStd::vector<const ProductAssetBrowserEntry*> products;
            GetChildren<ProductAssetBrowserEntry>(products);
            for (const ProductAssetBrowserEntry* product : products)
            {
                AZ::Data::AssetType productType = product->GetAssetType();
                if (productType  == assetType)
                {
                    return true;
                }
            }
            return false;
        }

        const SourceAssetBrowserEntry* SourceAssetBrowserEntry::GetSourceByAssetId(const AZ::Uuid& sourceId)
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                return cache->m_sourceIdMap[sourceId];
            }
            return nullptr;
        }

        void SourceAssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            child->m_fullPath = m_fullPath;
            AssetBrowserEntry::UpdateChildPaths(child);
        }

        void SourceAssetBrowserEntry::PathsUpdated()
        {
            AssetBrowserEntry::PathsUpdated();
            UpdateSourceControlThumbnail();
        }

        void SourceAssetBrowserEntry::UpdateSourceControlThumbnail()
        {
            if (m_sourceControlThumbnailKey)
            {
                disconnect(m_sourceControlThumbnailKey.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &AssetBrowserEntry::ThumbnailUpdated);
            }
            m_sourceControlThumbnailKey = MAKE_TKEY(SourceControlThumbnailKey, m_fullPath.c_str());
            connect(m_sourceControlThumbnailKey.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &AssetBrowserEntry::ThumbnailUpdated);
        }

        SharedThumbnailKey SourceAssetBrowserEntry::GetThumbnailKey() const
        {
            // if at least 1 product is present, try using its thumbnail instead
            if (GetChildCount() > 0)
            {
                return m_children.front()->GetThumbnailKey();
            }
            return AssetBrowserEntry::GetThumbnailKey();
        }

        SharedThumbnailKey SourceAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(SourceThumbnailKey, m_fullPath.c_str());
        }

        SharedThumbnailKey SourceAssetBrowserEntry::GetSourceControlThumbnailKey() const
        {
            return m_sourceControlThumbnailKey;
        }

        //////////////////////////////////////////////////////////////////////////
        // ProductAssetBrowserEntry
        //////////////////////////////////////////////////////////////////////////
        ProductAssetBrowserEntry::ProductAssetBrowserEntry()
            : AssetBrowserEntry()
            , m_fingerprint(0)
            , m_productID(0)
            , m_jobID(0) {}

        ProductAssetBrowserEntry::ProductAssetBrowserEntry(
            const char* name,
            AZ::u32 fingerprint,
            AZ::s64 productID,
            AZ::s64 jobID,
            const char* jobKey,
            AZ::Data::AssetId assetId,
            AZ::Data::AssetType assetType,
            const char* platform)
            : AssetBrowserEntry(name)
            , m_fingerprint(fingerprint)
            , m_productID(productID)
            , m_jobID(jobID)
            , m_jobKey(jobKey)
            , m_assetId(assetId)
            , m_assetType(assetType)
            , m_platform(platform)
        {
            m_assetType.ToString(m_assetTypeString);

            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_assetIdMap[assetId] = this;
            }

            AZ::Data::AssetCatalogRequestBus::BroadcastResult(m_relativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_assetId);

            AZStd::string productName;
            StringFunc::Path::Split(name, nullptr, nullptr, &productName, nullptr);
            AZStd::string assetTypeName;
            AZ::AssetTypeInfoBus::EventResult(assetTypeName, m_assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
            if (!assetTypeName.empty())
            {
                m_displayName = AZStd::string::format("%s (%s)", productName.c_str(), assetTypeName.c_str());
            }
            else
            {
                m_displayName = productName;
            }
        }

        ProductAssetBrowserEntry::~ProductAssetBrowserEntry()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_assetIdMap.erase(m_assetId);
            }
        }

        QVariant ProductAssetBrowserEntry::data(int column) const
        {
            switch (static_cast<Column>(column))
            {
            case Column::ProductID:
            {
                return QVariant(m_productID);
            }

            case Column::JobID:
            {
                return QVariant(m_jobID);
            }

            case Column::JobKey:
            {
                return QString::fromUtf8(m_jobKey.c_str());
            }

            case Column::SubID:
            {
                return QVariant(m_assetId.m_subId);
            }

            case Column::Platform:
            {
                return QString::fromUtf8(m_platform.c_str());
            }
            default:
            {
                return AssetBrowserEntry::data(column);
            }
            }
        }

        void ProductAssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ProductAssetBrowserEntry, AssetBrowserEntry>()
                    ->Field("m_fingerprint", &ProductAssetBrowserEntry::m_fingerprint)
                    ->Field("m_productID", &ProductAssetBrowserEntry::m_productID)
                    ->Field("m_jobID", &ProductAssetBrowserEntry::m_jobID)
                    ->Field("m_jobKey", &ProductAssetBrowserEntry::m_jobKey)
                    ->Field("m_assetId", &ProductAssetBrowserEntry::m_assetId)
                    ->Field("m_assetType", &ProductAssetBrowserEntry::m_assetType)
                    ->Field("m_platform", &ProductAssetBrowserEntry::m_platform)
                    ->Version(1);
            }
        }

        AssetBrowserEntry::AssetEntryType ProductAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Product;
        }

        AZ::u32 ProductAssetBrowserEntry::GetFingerprint() const
        {
            return m_fingerprint;
        }

        AZ::s64 ProductAssetBrowserEntry::GetProductID() const
        {
            return m_productID;
        }

        AZ::s64 ProductAssetBrowserEntry::GetJobID() const
        {
            return m_jobID;
        }

        const AZStd::string& ProductAssetBrowserEntry::GetJobKey() const
        {
            return m_jobKey;
        }

        const AZ::Data::AssetId& ProductAssetBrowserEntry::GetAssetId() const
        {
            return m_assetId;
        }

        const AZ::Data::AssetType& ProductAssetBrowserEntry::GetAssetType() const
        {
            return m_assetType;
        }

        const AZStd::string& ProductAssetBrowserEntry::GetPlatform() const
        {
            return m_platform;
        }

        const AZStd::string& ProductAssetBrowserEntry::GetAssetTypeString() const
        {
            return m_assetTypeString;
        }

        ProductAssetBrowserEntry* ProductAssetBrowserEntry::GetProductByAssetId(const AZ::Data::AssetId& assetId)
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                return cache->m_assetIdMap[assetId];
            }
            return nullptr;
        }

        void ProductAssetBrowserEntry::ThumbnailUpdated()
        {
            // if source is displaying product's thumbnail, then it needs to also listen to its ThumbnailUpdated
            if (m_parentAssetEntry)
            {
                if (EntryCache* cache = EntryCache::GetInstance())
                {
                    cache->m_dirtyThumbnailsSet.insert(m_parentAssetEntry);
                }
            }
        }

        SharedThumbnailKey AssetBrowser::ProductAssetBrowserEntry::GetThumbnailKey() const
        {
            QString iconPath;
            AZ::AssetTypeInfoBus::EventResult(iconPath, m_assetType, &AZ::AssetTypeInfo::GetBrowserIcon);
            if (iconPath.isEmpty())
            {
                return m_parentAssetEntry->AssetBrowserEntry::GetThumbnailKey();
            }
            return AssetBrowserEntry::GetThumbnailKey();
        }

        SharedThumbnailKey ProductAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(Thumbnailer::ProductThumbnailKey, m_assetId);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include <AssetBrowser/AssetBrowserEntry.moc>