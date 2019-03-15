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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/XML/rapidxml.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <QAbstractItemModel>
#include <QList>
#include <QModelIndex>
#include <QMenu>
#include <QSettings>
#include <QVariant>

#include <SliceFavorites/SliceFavoritesBus.h>

class QTreeView;
class QMimeData;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
    }
}

namespace SliceFavorites
{
    class FavoriteData
    {
    public:

        static QString GetMimeType() { return "sliceFavorites/favoriteData"; }

        typedef enum
        {
            DataType_Unknown = 0,
            DataType_Folder,
            DataType_Favorite
        } FavoriteType;

        FavoriteData()
            : m_type(DataType_Unknown)
        {
        }

        FavoriteData(FavoriteType type)
            : m_type(type)
        {
        }

        FavoriteData(const QString& name, FavoriteType type = DataType_Favorite)
            : m_name(name)
            , m_type(type)
        {
        }

        FavoriteData(const QString& name, const AZ::Data::AssetId assetId, FavoriteType type = DataType_Favorite)
            : m_name(name)
            , m_assetId(assetId)
            , m_type(type)
        {
        }

        ~FavoriteData();

        int LoadFromXML(AZ::rapidxml::xml_node<char>* node);
        int AddToXML(AZ::rapidxml::xml_node<char>* node, AZ::rapidxml::xml_document<char>* xmlDoc) const;

        QString m_name;
        AZ::Data::AssetId m_assetId;
        FavoriteType m_type;

        QList<FavoriteData*> m_children;
        FavoriteData* m_parent = nullptr;

        void appendChild(FavoriteData* child);
        FavoriteData* child(int row);
        int childCount() const;
        int columnCount() const;
        QVariant data(int role) const;
        int row() const;
        FavoriteData* parentItem() { return m_parent; }

        int GetNumFoldersInHierarchy() const;
        int GetNumFavoritesInHierarchy() const;

        void Reset();

    private:
        int GetNumOfType(FavoriteType type) const;
    };

    class FavoriteDataModel 
        : public QAbstractItemModel
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
    {
        Q_OBJECT

    public:
        explicit FavoriteDataModel(QObject *parent = nullptr);
        ~FavoriteDataModel();

        QVariant data(const QModelIndex &index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &index) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;

        void SetupModelData();

        size_t GetNumFavorites();
        void EnumerateFavorites(const AZStd::function<void(const AZ::Data::AssetId& assetId)>& callback);

        QMenu* GetFavoritesMenu() { return m_favoritesMenu.get(); }

        QModelIndex AddNewFolder(const QModelIndex& parent);

        void RemoveFavorite(const QModelIndex& indexToRemove);

        QModelIndex GetModelIndexForParent(const FavoriteData* child) const;
        QModelIndex GetModelIndexForFavorite(const FavoriteData* favorite) const;
        FavoriteData* GetFavoriteDataFromModelIndex(const QModelIndex& modelIndex) const;

        void CountFoldersAndFavoritesFromIndices(QModelIndexList& indices, int& numFolders, int& numFavorites);
        int GetNumFavoritesAndFolders();

        void ClearAll();

        bool HasFavoritesOrFolders() const;

        int ImportFavorites(const QString& importFileName);
        int ExportFavorites(const QString& exportFileName) const;

    Q_SIGNALS:
        void DataModelChanged();
        void ExpandIndex(const QModelIndex& index, bool expanded);
        void DisplayWarning(const QString& title, const QString& message);

    private:

        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId) override;

        ////////////////////////////////////////////////////////////////////////
        // AztoolsFramework::EditorEvents::Bus::Handler overrides
        void PopulateEditorGlobalContextMenu_SliceSection(QMenu* menu, const AZ::Vector2& point, int flags) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        void AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<FavoriteData> m_rootItem = nullptr;
        AZStd::unique_ptr<QMenu> m_favoritesMenu = nullptr;

        using FavoriteMap = AZStd::unordered_map<AZStd::string, FavoriteData*>;
        using FavoriteList = QList<FavoriteData*>;

        FavoriteMap m_favoriteMap;

        bool IsFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product) const;
        void AddFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product, const QModelIndex parent = QModelIndex());
        void RemoveFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product);

        void RemoveFavorite(const AZ::Data::AssetId& assetId);

        void UpdateFavorites();
        void LoadFavorites();
        void SaveFavorites();
        void WriteChildren(QSettings& settings, FavoriteList& currentList);
        void ReadChildren(QSettings& settings, FavoriteList& currentList);
        void BuildChildToParentMap();
        void UpdateChildren(FavoriteData* parent);
        void RebuildMenu();
        void AddFavoriteToMenu(const FavoriteData* favorite, QMenu* menu);
        void RemoveFavorite(const FavoriteData* toRemove);
        void RemoveFromFavoriteMap(const FavoriteData* toRemove, bool removeChildren = true);

        QString GetProjectName();

        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;

        void GetSelectedIndicesFromMimeData(QList<FavoriteData*>& results, const QByteArray& buffer) const;

        const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* GetSliceProductFromBrowserEntry(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const;
        bool FavoriteDataModel::IsSliceAssetType(const AZ::Data::AssetType& type) const;

        bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
        bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    };
}
