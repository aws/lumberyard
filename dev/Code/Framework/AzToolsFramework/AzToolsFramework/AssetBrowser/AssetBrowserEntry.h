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
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>

#include <AzToolsFramework/Thumbnails/Thumbnail.h>

#include <QObject>
#include <QModelIndex>

class QMimeData;

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    using namespace Thumbnailer;

    namespace AssetDatabase
    {
        class CombinedDatabaseEntry;
    }

    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class RootAssetBrowserEntry;
        class FolderAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        class ProductAssetBrowserEntry;

        //! AssetBrowserEntry is a base class for asset tree view entry
        class AssetBrowserEntry
            : public QObject
        {
            friend class AssetBrowserModel;
            friend class AssetBrowserFilterModel;
            friend class AssetBrowserEntry;
            friend class RootAssetBrowserEntry;
            friend class FolderAssetBrowserEntry;
            friend class SourceAssetBrowserEntry;
            friend class ProductAssetBrowserEntry;

            Q_OBJECT
        public:
            enum class AssetEntryType
            {
                Root,
                Folder,
                Source,
                Product
            };

            static QString AssetEntryTypeToString(AssetEntryType assetEntryType);
            
            //NOTE: this list should be in sync with m_columnNames[] in the cpp file
            enum class Column
            {
                Name,
                SourceID,
                Fingerprint,
                Guid,
                ScanFolderID,
                ProductID,
                JobID,
                JobKey,
                SubID,
                AssetType,
                Platform,
                ClassID,
                DisplayName,
                Count
            };

            static const char* m_columnNames[static_cast<int>(Column::Count)];

        protected:
            AssetBrowserEntry();
            explicit AssetBrowserEntry(const char* name);

        public:
            AZ_RTTI(AssetBrowserEntry, "{67679F9E-055D-43BE-A2D0-FB4720E5302A}");

            virtual ~AssetBrowserEntry();

            virtual QVariant data(int column) const;
            int row() const;
            static bool FromMimeData(const QMimeData* mimeData, AZStd::vector<AssetBrowserEntry*>& entries);
            void AddToMimeData(QMimeData* mimeData) const;
            static QString GetMimeType();
            static void Reflect(AZ::ReflectContext* context);
            virtual AssetEntryType GetEntryType() const = 0;

            //! Actual name of the asset or folder
            const AZStd::string& GetName() const;
            //! Display name represents how entry is shown in asset browser
            const AZStd::string& GetDisplayName() const;
            //! Return path relative to scan folder
            const AZStd::string& GetRelativePath() const;
            //! Return absolute path. If called on product, return source absolute path
            const AZStd::string& GetFullPath() const;

            //! Get immediate children of specific type
            template<typename EntryType>
            void GetChildren(AZStd::vector<const EntryType*>& entries) const;

            //! Recurse through the tree down to get all entries of specific type
            template<typename EntryType>
            void GetChildrenRecursively(AZStd::vector<const EntryType*>& entries) const;

            ///! Utility function:  Given a Qt QMimeData pointer, your callbackFunction will be called for each entry of that type it finds in there.
            template <typename EntryType>
            static void ForEachEntryInMimeData(const QMimeData* mimeData, AZStd::function<void(const EntryType*)> callbackFunction);

            //! Get child by index
            const AssetBrowserEntry* GetChild(int index) const;
            //! Get number of children
            int GetChildCount() const;
            //! Get immediate parent
            AssetBrowserEntry* GetParent() const;

            virtual SharedThumbnailKey GetThumbnailKey() const;
            void SetThumbnailKey(SharedThumbnailKey thumbnailKey);
            virtual SharedThumbnailKey CreateThumbnailKey() = 0;

        protected:
            AZStd::string m_name;
            AZStd::string m_displayName;
            AZStd::string m_relativePath;
            AZStd::string m_fullPath;
            AZStd::vector<AssetBrowserEntry*> m_children;
            AssetBrowserEntry* m_parentAssetEntry = nullptr;

            virtual void AddChild(AssetBrowserEntry* child);
            void RemoveChild(AssetBrowserEntry* child);
            void RemoveChildren();

            //! When child is added, its paths are updated relative to this entry
            virtual void UpdateChildPaths(AssetBrowserEntry* child) const;
            virtual void PathsUpdated();

        protected Q_SLOTS:
            virtual void ThumbnailUpdated();

        private:
            SharedThumbnailKey m_thumbnailKey;
            //! index in its parent's m_children list
            int m_row = 0;

            AZ_DISABLE_COPY_MOVE(AssetBrowserEntry);
        };

        //! RootAssetBrowserEntry is a root node for Asset Browser tree view, it's not related to any asset path.
        class RootAssetBrowserEntry
            : public AssetBrowserEntry
        {
        public:
            AZ_RTTI(RootAssetBrowserEntry, "{A35CA80E-E1EB-420B-8BFE-B7792E3CCEDB}");
            AZ_CLASS_ALLOCATOR(RootAssetBrowserEntry, AZ::SystemAllocator, 0);

            RootAssetBrowserEntry();

            static void Reflect(AZ::ReflectContext* context);

            AssetEntryType GetEntryType() const override;

            //! Update root node to new dev location
            void Update(const char* devPath);
            //! Add asset from database
            void AddAsset(const AssetDatabase::CombinedDatabaseEntry& combinedDatabaseEntry);
            //! Remove product by assetId
            static void RemoveProduct(const AZ::Data::AssetId& assetId);
            //! Remove source by sourceUuid
            static void RemoveSource(const AZ::Uuid& sourceUuid);

            SharedThumbnailKey CreateThumbnailKey() override;

        protected:
            void UpdateChildPaths(AssetBrowserEntry* child) const override;

        private:
            AZ_DISABLE_COPY_MOVE(RootAssetBrowserEntry);

            AZStd::string m_devPath;

            //! Create folder entry child
            AssetBrowserEntry* CreateFolder(const char* folderName, AssetBrowserEntry* parent, bool isScanFolder = false, bool isGemsFolder = false);
            //! Recursively create folder structure leading to relative path from parent
            AssetBrowserEntry* CreateFolders(const char* relativePath, AssetBrowserEntry* parent, bool isScanFolder = false, bool isGemsFolder = false);
            //! Remove entry and recursively trim its parent(s) if it has no children left
            static void RemoveEntry(AssetBrowserEntry* entry);
        };

        //! FolderAssetBrowserEntry is a class for any folder.
        class FolderAssetBrowserEntry
            : public AssetBrowserEntry
        {
        public:
            AZ_RTTI(FolderAssetBrowserEntry, "{938E6FCD-1582-4B63-A7EA-5C4FD28CABDC}", AssetBrowserEntry);
            AZ_CLASS_ALLOCATOR(FolderAssetBrowserEntry, AZ::SystemAllocator, 0);

            FolderAssetBrowserEntry();
            FolderAssetBrowserEntry(const char* name, bool isScanFolder, bool isGemsFolder);

            static void Reflect(AZ::ReflectContext* context);

            AssetEntryType GetEntryType() const override;
            bool GetIsScanFolder() const;
            void SetIsScanFolder(bool value);
            bool IsGemsFolder() const;

            SharedThumbnailKey CreateThumbnailKey() override;

        protected:
            void UpdateChildPaths(AssetBrowserEntry* child) const override;

        private:
            bool m_isScanFolder;
            bool m_isGemsFolder;

            AZ_DISABLE_COPY_MOVE(FolderAssetBrowserEntry);
        };

        //! SourceAssetBrowserEntry represents source entry.
        class SourceAssetBrowserEntry
            : public AssetBrowserEntry
        {
        public:
            AZ_RTTI(SourceAssetBrowserEntry, "{9FD4FF76-4CC3-4E96-953F-5BF63C2E1F1D}", AssetBrowserEntry);
            AZ_CLASS_ALLOCATOR(SourceAssetBrowserEntry, AZ::SystemAllocator, 0);

            SourceAssetBrowserEntry();
            SourceAssetBrowserEntry(const char* name, const char* extension, AZ::s64 sourceID, AZ::s64 scanFolderID, const AZ::Uuid& sourceUuid);
            ~SourceAssetBrowserEntry() override;

            QVariant data(int column) const override;
            static void Reflect(AZ::ReflectContext* context);
            AssetEntryType GetEntryType() const override;

            const AZStd::string& GetExtension() const;
            AZ::s64 GetSourceID() const;
            AZ::s64 GetScanFolderID() const;

            //! returns the asset type of the first child (product) that isn't an invalid type.
            AZ::Data::AssetType GetPrimaryAssetType() const;

            //! Returns true if any children (products) are the given asset type
            bool HasProductType(const AZ::Data::AssetType& assetType) const;
            SharedThumbnailKey GetThumbnailKey() const override;
            SharedThumbnailKey CreateThumbnailKey() override;
            SharedThumbnailKey GetSourceControlThumbnailKey() const;
            const AZ::Uuid& GetSourceUuid() const;

            static const SourceAssetBrowserEntry* GetSourceByAssetId(const AZ::Uuid& sourceUuid);

        protected:
            void UpdateChildPaths(AssetBrowserEntry* child) const override;
            void PathsUpdated() override;

        private:
            AZStd::string m_extension;
            AZ::s64 m_sourceID;
            AZ::s64 m_scanFolderID;
            AZ::Uuid m_sourceUuid;
            SharedThumbnailKey m_sourceControlThumbnailKey;

            void UpdateSourceControlThumbnail();

            AZ_DISABLE_COPY_MOVE(SourceAssetBrowserEntry);
        };

        //! ProductAssetBrowserEntry represents product entrypp.
        class ProductAssetBrowserEntry
            : public AssetBrowserEntry
        {
        public:
            AZ_RTTI(ProductAssetBrowserEntry, "{52C02087-D68B-4E9D-BB8A-01E43CE51BA2}", AssetBrowserEntry);
            AZ_CLASS_ALLOCATOR(ProductAssetBrowserEntry, AZ::SystemAllocator, 0);

            ProductAssetBrowserEntry();
            ProductAssetBrowserEntry(
                const char* name,
                AZ::u32 fingerprint,
                AZ::s64 productID,
                AZ::s64 jobID,
                const char* jobKey,
                AZ::Data::AssetId assetId,
                AZ::Data::AssetType assetType,
                const char* platform);
            ~ProductAssetBrowserEntry() override;

            QVariant data(int column) const override;
            static void Reflect(AZ::ReflectContext* context);
            AssetEntryType GetEntryType() const override;

            AZ::u32 GetFingerprint() const;
            AZ::s64 GetProductID() const;
            AZ::s64 GetJobID() const;
            const AZStd::string& GetJobKey() const;
            const AZ::Data::AssetId& GetAssetId() const;
            const AZ::Data::AssetType& GetAssetType() const;
            const AZStd::string& GetPlatform() const;
            const AZStd::string& GetAssetTypeString() const;
            SharedThumbnailKey GetThumbnailKey() const override;
            SharedThumbnailKey CreateThumbnailKey() override;

            static ProductAssetBrowserEntry* GetProductByAssetId(const AZ::Data::AssetId& assetId);

            void ThumbnailUpdated() override;

        private:
            AZ::u32 m_fingerprint;
            AZ::s64 m_productID;
            AZ::s64 m_jobID;
            AZStd::string m_jobKey;
            AZ::Data::AssetId m_assetId;
            AZ::Data::AssetType m_assetType;
            AZStd::string m_platform;
            AZStd::string m_assetTypeString;

            AZ_DISABLE_COPY_MOVE(ProductAssetBrowserEntry);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

Q_DECLARE_METATYPE(const AzToolsFramework::AssetBrowser::AssetBrowserEntry*)

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.inl>