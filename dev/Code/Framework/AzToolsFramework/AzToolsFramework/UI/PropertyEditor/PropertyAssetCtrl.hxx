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
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QCompleter>
#include <QLabel>
#include <QMovie>
#include <QStyledItemDelegate>
#include <QLineEdit>
AZ_POP_DISABLE_WARNING

class QPushButton;
class QDragEnterEvent;
class QMimeData;

namespace AzToolsFramework
{
    class AssetCompleterModel;
    class AssetCompleterListView;

    //! Subclasses the lineEdit to show loading animation
    class LineEditLoading
        : public QLineEdit
        , private AZ::TickBus::Handler
    {
        Q_OBJECT

    public:
        explicit LineEditLoading(QWidget *parent = nullptr);
        ~LineEditLoading();

        void StartLoading();
        void StopLoading();

    protected:
        void paintEvent(QPaintEvent *event) override;

        // AZ::TickBus ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        QMovie m_loadIcon;
        bool m_isLoading = false;
    };

    //! Defines a property control for picking base assets.
    //! We can specialize individual asset types (texture) to show previews and such by making specialized handlers, but
    //! at the very least we need a base editor for asset properties in general.
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

        QWidget* GetFirstInTabOrder() { return m_lineEdit; }
        QWidget* GetLastInTabOrder() { return m_clearButton; }
        void UpdateTabOrder();

        // Resolved asset for this control, this is the user selection with a fallback to the default asset(if exists)
        const AZ::Data::AssetId& GetCurrentAssetID() const { return m_selectedAssetID.IsValid() ? m_selectedAssetID : m_defaultAssetID; }
        const AZ::Data::AssetType& GetCurrentAssetType() const { return m_currentAssetType; }
        const AZStd::string GetCurrentAssetHint() const { return m_currentAssetHint; }

        // User's assetId selection in the UI
        const AZ::Data::AssetId& GetSelectedAssetID() const { return m_selectedAssetID; }

        bool eventFilter(QObject* obj, QEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dropEvent(QDropEvent* event) override;

        virtual AssetSelectionModel GetAssetSelectionModel() { return AssetSelectionModel::AssetTypeSelection(GetCurrentAssetType()); }

    signals:
        void OnAssetIDChanged(AZ::Data::AssetId newAssetID);

    protected:
        QPushButton* m_errorButton = nullptr;
        QPushButton* m_editButton = nullptr;
        QPushButton* m_clearButton = nullptr;
        QPushButton* m_browseButton = nullptr;

        AZ::Data::AssetId m_selectedAssetID;
        AZStd::string m_currentAssetHint;

        QCompleter* m_completer = nullptr;
        AssetCompleterModel* m_model = nullptr;
        AssetCompleterListView* m_view = nullptr;

        AZ::Data::AssetId m_defaultAssetID;

        AZ::Data::AssetType m_currentAssetType;

        LineEditLoading* m_lineEdit = nullptr;
        QLabel* m_label = nullptr;

        AZStd::string m_defaultAssetHint;

        void* m_editNotifyTarget = nullptr;
        EditCallbackType* m_editNotifyCallback = nullptr;
        QString m_optionalValidDragDropExtensions;

        static const int s_autocompleteAfterNumberOfChars = 3;
        int m_lineEditLastCursorPosition = 0;

        bool m_completerIsConfigured = false;
        bool m_completerIsActive = false;
        bool m_incompleteFilename = false;
        bool m_unnamedType = false;

        bool IsCorrectMimeData(const QMimeData* pData, AZ::Data::AssetId* pAssetId = nullptr, AZ::Data::AssetType* pAssetType = nullptr) const;
        void ClearErrorButton();
        void UpdateErrorButton(const AZStd::string& errorLog);
        virtual const AZStd::string GetFolderSelection() const { return AZStd::string(); }
        virtual void SetFolderSelection(const AZStd::string& /* folderPath */) {}
        virtual void ClearAssetInternal();

        void ConfigureAutocompleter();
        void EnableAutocompleter();
        void DisableAutocompleter();

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
        void SetClearButtonEnabled(bool enable);
        void SetClearButtonVisible(bool visible);
        void SetSelectedAssetID(const AZ::Data::AssetId& newID);
        void SetCurrentAssetType(const AZ::Data::AssetType& newType);
        void SetSelectedAssetID(const AZ::Data::AssetId& newID, const AZ::Data::AssetType& newType);
        void SetCurrentAssetHint(const AZStd::string& hint);
        void SetDefaultAssetID(const AZ::Data::AssetId& defaultID);
        void PopupAssetBrowser();
        void ClearAsset();
        void UpdateAssetDisplay();
        void OnEditButtonClicked();
        void OnAutocomplete(const QModelIndex& index);
        void OnTextChange(const QString& text);
        void OnReturnPressed();
        void ShowContextMenu(const QPoint& pos);

    private:
        const QModelIndex GetSourceIndex(const QModelIndex& index);
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
        virtual AZ::u32 GetHandlerName(void) const override { return AZ_CRC("Asset", 0x02af5a5c); }
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

        virtual AZ::u32 GetHandlerName(void) const override { return AZ_CRC("SimpleAssetRef", 0x49f51d54); }
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
