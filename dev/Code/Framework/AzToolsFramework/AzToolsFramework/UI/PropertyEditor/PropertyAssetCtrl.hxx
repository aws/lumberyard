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

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include "PropertyEditorAPI.h"
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorAssetReference.h>

#include <QLabel>

class QPushButton;
class QDragEnterEvent;
class QMimeData;

namespace AzToolsFramework
{
    // defines a property control for picking base assets.
    // note that we can specialize individual asset types (texture) to show previews and such by making specialized handlers, but
    // at the very least we need a base editor for asset properties in general.
    class PropertyAssetCtrl
        : public QWidget
        , protected AssetSystemBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(PropertyAssetCtrl, AZ::SystemAllocator, 0);

        using EditCallbackType = AZ::Edit::AttributeFunction<void(const AZ::Data::AssetId&, const AZ::Data::AssetType&)>;

        PropertyAssetCtrl(QWidget *pParent = NULL, QString optionalValidDragDropExtensions = QString());
        virtual ~PropertyAssetCtrl();

        QWidget* GetFirstInTabOrder() { return m_label; }
        QWidget* GetLastInTabOrder() { return m_clearButton; }
        void UpdateTabOrder();

        const AZ::Data::AssetId& GetCurrentAssetID() const { return m_currentAssetID; }
        const AZ::Data::AssetType& GetCurrentAssetType() const { return m_currentAssetType; }
        const AZStd::string GetCurrentAssetHint() const { return m_currentAssetHint; }

        bool eventFilter(QObject* obj, QEvent* event) override;
        virtual void dragEnterEvent(QDragEnterEvent* event) override;
        virtual void dragLeaveEvent(QDragLeaveEvent* event) override;
        virtual void dropEvent(QDropEvent* event) override;

    signals:
        void OnAssetIDChanged(AZ::Data::AssetId newAssetID);

    protected:
        QString m_optionalValidDragDropExtensions;
        AZ::Data::AssetId m_currentAssetID;
        AZ::Data::AssetType m_currentAssetType;
        AZStd::string m_currentAssetHint;
        QLabel* m_label;
        QPushButton* m_errorButton = nullptr;
        QPushButton* m_editButton;
        QPushButton* m_clearButton;
        QPushButton* m_browseButton;
        void* m_editNotifyTarget;
        EditCallbackType* m_editNotifyCallback;

        bool IsCorrectMimeData(const QMimeData* pData, AZ::Data::AssetId* pAssetId = nullptr, AZ::Data::AssetType* pAssetType = nullptr) const;
        void ClearErrorButton();
        void UpdateErrorButton(const AZStd::string& errorLog);

        //////////////////////////////////////////////////////////////////////////
        // AssetSystemBus
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
        void SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
        //////////////////////////////////////////////////////////////////////////

    public slots:
        void SetEditNotifyTarget(void* editNotifyTarget);
        void SetEditNotifyCallback(EditCallbackType* editNotifyCallback);
        void SetEditButtonEnabled(bool enabled);
        void SetEditButtonVisible(bool visible);
        void SetEditButtonIcon(const QIcon& icon);
        void SetEditButtonTooltip(QString tooltip);
        void SetCurrentAssetID(const AZ::Data::AssetId& newID);
        void SetCurrentAssetType(const AZ::Data::AssetType& newType);
        void SetCurrentAssetID(const AZ::Data::AssetId& newID, const AZ::Data::AssetType& newType);
        void SetCurrentAssetHint(const AZStd::string& hint);
        void PopupAssetBrowser();
        void ClearAsset();
        void UpdateAssetDisplay();
        void OnEditButtonClicked();
        void ShowContextMenu(const QPoint& pos);
    };

    class AssetPropertyHandlerDefault
        : QObject
        , public PropertyHandler<AZ::Data::Asset<AZ::Data::AssetData>, PropertyAssetCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AssetPropertyHandlerDefault, AZ::SystemAllocator, 0);

        virtual const AZ::Uuid& GetHandledType() const override;
        virtual AZ::u32 GetHandlerName(void) const override  { return AZ_CRC("Asset", 0x02af5a5c); }
        virtual bool IsDefaultHandler() const override { return true; }
        virtual QWidget* GetFirstInTabOrder(PropertyAssetCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyAssetCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyAssetCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        virtual void ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    class SimpleAssetPropertyHandlerDefault
        : QObject
        , public PropertyHandler<AzFramework::SimpleAssetReferenceBase, PropertyAssetCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(SimpleAssetPropertyHandlerDefault, AZ::SystemAllocator, 0);

        virtual AZ::u32 GetHandlerName(void) const override  { return AZ_CRC("SimpleAssetRef", 0x49f51d54); }
        virtual bool IsDefaultHandler() const override { return true; }
        virtual QWidget* GetFirstInTabOrder(PropertyAssetCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyAssetCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyAssetCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        virtual void ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterAssetPropertyHandler();
} // namespace AzToolsFramework
