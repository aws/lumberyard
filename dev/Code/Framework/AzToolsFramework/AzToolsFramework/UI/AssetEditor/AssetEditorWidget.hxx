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

#include <QtWidgets/QWidget>
#include <QtWidgets/QDialog>

#include <AzCore/Asset/AssetCommon.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

namespace AZ
{
    class SerializeContext;
}

class QDialogButtonBox;

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;

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

        AssetEditorWidget(QWidget* parent = nullptr, AZ::SerializeContext* serializeContext = nullptr);
        ~AssetEditorWidget() {};

        void SetAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        bool IsDirty() const { return m_dirty; }

    public Q_SLOTS:
        void SaveAsset();
        void RevertAsset();

Q_SIGNALS:
        void OnAssetSavedQSignal();
        void OnAssetSaveFailed(const AZStd::string& error);
        void OnAssetReverted();
        void OnAssetRevertFailed(const AZStd::string& error);

        void OnAssetOpened(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

    protected: // IPropertyEditorNotify

        void AfterPropertyModified(InstanceDataNode* /*node*/) override;
        void RequestPropertyContextMenu(InstanceDataNode*, const QPoint&) override;

        void BeforePropertyModified(InstanceDataNode* /*node*/) override {}
        void SetPropertyEditingActive(InstanceDataNode* /*node*/) override {}
        void SetPropertyEditingComplete(InstanceDataNode* /*node*/) override {}
        void SealUndoStack() override {};

    protected:

        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId) override;

        ReflectedPropertyEditor* m_propertyEditor;

        AZ::Data::Asset<AZ::Data::AssetData> m_asset;
        AZ::Data::Asset<AZ::Data::AssetData> m_assetOriginal;

        AZStd::string m_newAssetPath;
        AZ::Data::AssetType m_newAssetType;

        bool m_dirty;

        AZ::SerializeContext* m_serializeContext;
    };

    /**
     * Dialog wrapper for the above widget, providing buttons and modal behavior.
     */
    class AssetEditorDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(AssetEditorDialog, AZ::SystemAllocator, 0);

        AssetEditorDialog(QWidget* parent = nullptr, AZ::SerializeContext* serializeContext = nullptr);
        ~AssetEditorDialog();

        void SetAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

    protected Q_SLOTS:

        void OnTestChangesClicked();
        void OnRevertAndCloseClicked();
        void OnSaveAndCloseClicked();

        void OnAssetSaved();
        void OnAssetSaveFailed(const AZStd::string& error);
        void OnAssetReverted();
        void OnAssetRevertFailed(const AZStd::string& error);
        void OnAssetOpened(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

    protected:

        void closeEvent(QCloseEvent* event) override;

        AssetEditorWidget* m_assetEditor;
        QDialogButtonBox* m_buttonBox;

        bool m_closeOnSuccess;
        bool m_closeOnFail;
    };
} // namespace AzToolsFramework
