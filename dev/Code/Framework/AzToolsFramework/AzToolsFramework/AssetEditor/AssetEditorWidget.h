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
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

#include <QWidget>

namespace AZ
{
    class SerializeContext;
}

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;

    namespace AssetEditor
    {
        /**
         * Provides ability to create, edit, and save reflected assets.
         */
        class AssetEditorWidget
            : public QWidget
            , private AZ::Data::AssetBus::Handler
            , private AzFramework::AssetCatalogEventBus::Handler
            , private AzToolsFramework::IPropertyEditorNotify
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetEditorWidget, AZ::SystemAllocator, 0);

            explicit AssetEditorWidget(QWidget* parent = nullptr);
            ~AssetEditorWidget() override = default;

            void SetAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            bool IsDirty() const { return m_dirty; }
            bool TrySave(const AZStd::function<void()>& savedCallback);

        public Q_SLOTS:
            void OpenAsset();
            void SaveAsset();

        Q_SIGNALS:
            void OnAssetSavedSignal();
            void OnAssetSaveFailedSignal(const AZStd::string& error);
            void OnAssetRevertedSignal();
            void OnAssetRevertFailedSignal(const AZStd::string& error);
            void OnAssetOpenedSignal(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

        protected: // IPropertyEditorNotify

            void AfterPropertyModified(InstanceDataNode* /*node*/) override;
            void RequestPropertyContextMenu(InstanceDataNode*, const QPoint&) override;

            void BeforePropertyModified(InstanceDataNode* /*node*/) override {}
            void SetPropertyEditingActive(InstanceDataNode* /*node*/) override {}
            void SetPropertyEditingComplete(InstanceDataNode* /*node*/) override {}
            void SealUndoStack() override {};

            void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId) override;

        private:
            AZStd::vector<AZ::Data::AssetType> m_genericAssetTypes;

            ReflectedPropertyEditor* m_propertyEditor;

            AZ::Data::Asset<AZ::Data::AssetData> m_asset;
            AZ::Data::Asset<AZ::Data::AssetData> m_assetOriginal;

            AZStd::string m_newAssetPath;
            AZ::Data::AssetType m_newAssetType;

            bool m_dirty = false;

            AZ::SerializeContext* m_serializeContext = nullptr;

            QAction* m_saveAssetAction;

            void PopulateGenericAssetTypes();
            void CreateAsset(AZ::Data::AssetType assetType);
        };
    } // namespace AssetEditor
} // namespace AzToolsFramework
