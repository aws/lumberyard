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
#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/UI/AssetEditor/AssetEditorWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QFileDialog>

namespace AzToolsFramework
{
    namespace
    {
        using AssetCheckoutAndSaveCallback = AZStd::function<void(bool, const AZStd::string&)>;

        void AssetCheckoutAndSaveCommon(const AZ::Data::AssetId& id, AZ::Data::Asset<AZ::Data::AssetData> asset, AZ::SerializeContext* serializeContext, AssetCheckoutAndSaveCallback callback)
        {
            AZStd::string assetPath;
            EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, id);

            AZStd::string assetFullPath;
            EBUS_EVENT(AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, assetPath, assetFullPath);

            if (!assetFullPath.empty())
            {
                using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
                SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, assetFullPath.c_str(), true,
                    [id, asset, assetFullPath, serializeContext, callback](bool /*success*/, const AzToolsFramework::SourceControlFileInfo& info)
                    {
                        if (!info.IsReadOnly())
                        {
                            if (AZ::Utils::SaveObjectToFile(assetFullPath, AZ::DataStream::ST_XML, asset.Get(), asset.Get()->RTTI_GetType(), serializeContext))
                            {
                                callback(true, "");
                            }
                            else
                            {
                                AZStd::string error = AZStd::string::format("Could not write asset file: %s.", assetFullPath.c_str());
                                callback(false, error);
                            }
                        }
                        else
                        {
                            AZStd::string error = AZStd::string::format("Could not check out asset file: %s.", assetFullPath.c_str());
                            callback(false, error);
                        }
                    }
                    );
            }
            else
            {
                AZStd::string error = AZStd::string::format("Could not resolve path name for asset {%s}.", id.ToString<AZStd::string>().c_str());
                callback(false, error);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // AssetEditorWidget
    //////////////////////////////////////////////////////////////////////////
    AssetEditorWidget::AssetEditorWidget(QWidget* parent, AZ::SerializeContext* serializeContext)
        : QWidget(parent)
        , m_serializeContext(serializeContext)
        , m_dirty(false)
    {
        if (!m_serializeContext)
        {
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");
        }

        m_propertyEditor = new ReflectedPropertyEditor(this);
        m_propertyEditor->Setup(m_serializeContext, this, true, 250);
        m_propertyEditor->show();

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mainLayout->addWidget(m_propertyEditor);

        QMenuBar* mainMenu = new QMenuBar();
        QMenu* fileMenu = mainMenu->addMenu(tr("&File"));
        QMenu* newAssetMenu = fileMenu->addMenu(tr("Create &New Asset"));

        auto enumerateCallback =
            [newAssetMenu, serializeContext, this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*typeId*/) -> bool
            {
                if (classData->m_typeId == AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid())
                {
                    return true;
                }

                AZStd::string className = classData->m_editData ? classData->m_editData->m_name : classData->m_name;
                const char* classDescription = classData->m_editData ? classData->m_editData->m_description : nullptr;
                if (classDescription && classDescription[0] == '\0')
                {
                    classDescription = nullptr;
                }

                AZStd::string menuText = AZStd::string::format("%s%s%s", className.c_str(), classDescription ? " - " : "", classDescription ? classDescription : "");
                QAction* newAssetAction = newAssetMenu->addAction(tr(menuText.c_str()));
                connect(newAssetAction, &QAction::triggered, this,
                    [classData, serializeContext, className, this]()
                    {
                        if (m_dirty)
                        {
                            QMessageBox::warning(this, tr("Save Required"),
                                tr("The currently-opened asset has been modified. Please save or revert changes prior to creating a new asset."), QMessageBox::Ok, QMessageBox::Ok);
                            return;
                        }

                        char assetRoot[AZ_MAX_PATH_LEN] = { 0 };
                        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@devassets@", assetRoot, AZ_MAX_PATH_LEN);

                        AZStd::vector<AZStd::string> assetTypeExtensions;
                        EBUS_EVENT_ID(classData->m_typeId, AZ::AssetTypeInfoBus, GetAssetTypeExtensions, assetTypeExtensions);
                        QString filter;
                        if (assetTypeExtensions.empty())
                        {
                            filter = tr("All Files (*.*)");
                        }
                        else
                        {
                            filter.append(className.c_str());
                            filter.append(" (");
                            for (size_t i = 0, n = assetTypeExtensions.size(); i < n; ++i)
                            {
                                const char* ext = assetTypeExtensions[i].c_str();
                                if (ext[0] == '.')
                                {
                                    ++ext;
                                }

                                filter.append("*.");
                                filter.append(ext);
                                if (i < n - 1)
                                {
                                    filter.append(", ");
                                }
                            }
                            filter.append(")");
                        }

                        const QString saveAs = QFileDialog::getSaveFileName(nullptr, tr("Save As..."), assetRoot, filter);

                        if (!saveAs.isEmpty())
                        {
                            AZStd::string targetFilePath = saveAs.toUtf8().constData();
                            if (AZ::IO::SystemFile::Exists(targetFilePath.c_str()))
                            {
                                QMessageBox::warning(GetActiveWindow(), tr("File Already Exists"),
                                    tr("Asset cannot be created because file already exists."), QMessageBox::Ok, QMessageBox::Ok);
                                return;
                            }

                            AZ::Data::Asset<AZ::Data::AssetData> newAsset =
                                AZ::Data::AssetManager::Instance().CreateAsset(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), classData->m_typeId);

                            AZ::IO::FileIOStream fileStream(targetFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
                            if (fileStream.IsOpen())
                            {
                                if (AZ::Utils::SaveObjectToStream(fileStream, AZ::DataStream::ST_XML, newAsset.Get(), classData->m_typeId, serializeContext))
                                {
                                    m_newAssetPath = AZStd::move(targetFilePath);
                                    m_newAssetType = classData->m_typeId;
                                    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, m_newAssetPath);
                                }
                            }
                        }
                    }
                    );

                return true;
            };

        m_serializeContext->EnumerateDerived(
            enumerateCallback,
            AZ::Uuid::CreateNull(),
            AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid()
            );

        if (newAssetMenu->actions().isEmpty())
        {
            mainMenu->setVisible(false);
        }

        mainLayout->setMenuBar(mainMenu);

        setLayout(mainLayout);

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    void AssetEditorWidget::SetAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();

        m_assetOriginal = asset;
        m_asset = AZ::Data::Asset<AZ::Data::AssetData>();

        if (m_assetOriginal)
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_assetOriginal.GetId());
            m_assetOriginal.QueueLoad();
        }
        else
        {
            OnAssetOpened(AZ::Data::Asset<AZ::Data::AssetData>());
        }
    }

    void AssetEditorWidget::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_dirty = false;

        m_propertyEditor->ClearInstances();

        m_asset = AZ::Data::AssetManager::Instance().CreateAsset(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), asset.GetType());

        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<decltype(buffer)> stream(&buffer);
        if (!AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, asset.Get(), asset.Get()->RTTI_GetType(), m_serializeContext))
        {
            m_asset = AZ::Data::Asset<AZ::Data::AssetData>();
            AZ_Error("Asset Editor", false, "Failed to create working asset.");
            return;
        }

        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        if (!AZ::Utils::LoadObjectFromStreamInPlace(stream, m_serializeContext, m_asset.Get()->RTTI_GetType(), m_asset.Get(), AZ::ObjectStream::FilterDescriptor()))
        {
            m_asset = AZ::Data::Asset<AZ::Data::AssetData>();
            AZ_Error("Asset Editor", false, "Failed to initialize working asset.");
            return;
        }

        AZ_Assert(m_assetOriginal.GetId() == asset.GetId(), "Incorrect asset id.");
        AZ::Crc32 saveStateKey;
        saveStateKey.Add(&m_assetOriginal.GetId(), sizeof(AZ::Data::AssetId));
        m_propertyEditor->SetSavedStateKey(saveStateKey);
        m_propertyEditor->AddInstance(m_asset.Get(), m_asset.Get()->RTTI_GetType(), nullptr, m_assetOriginal.Get());
        m_propertyEditor->InvalidateAll();

        OnAssetOpened(m_assetOriginal);
    }

    void AssetEditorWidget::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void AssetEditorWidget::SaveAsset()
    {
        AssetCheckoutAndSaveCommon(m_assetOriginal.GetId(), m_asset, m_serializeContext,
            [this](bool success, const AZStd::string& error)
            {
                if (success)
                {
                    m_dirty = false;

                    Q_EMIT OnAssetSavedQSignal();
                }
                else
                {
                    Q_EMIT OnAssetSaveFailed(error);
                }
            }
            );
    }

    void AssetEditorWidget::RevertAsset()
    {
        if (m_asset.Get() == m_assetOriginal.Get())
        {
            Q_EMIT OnAssetReverted();
            return;
        }

        AssetCheckoutAndSaveCommon(m_assetOriginal.GetId(), m_assetOriginal, m_serializeContext,
            [this](bool success, const AZStd::string& error)
            {
                if (success)
                {
                    Q_EMIT OnAssetReverted();
                }
                else
                {
                    Q_EMIT OnAssetRevertFailed(error);
                }
            }
            );
    }

    void AssetEditorWidget::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        EBUS_EVENT_RESULT(assetInfo, AZ::Data::AssetCatalogRequestBus, GetAssetInfoById, assetId);

        // If this asset was created through this dialog, go ahead and assign it.
        if (nullptr != strstr(m_newAssetPath.c_str(), assetInfo.m_relativePath.c_str()))
        {
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset(assetId, m_newAssetType, false);
            if (asset)
            {
                SetAsset(asset);
            }
        }
    }

    void AssetEditorWidget::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId)
    {
        if (assetId.IsValid() && assetId == m_assetOriginal.GetId())
        {
            SetAsset(AZ::Data::Asset<AZ::Data::AssetData>());
        }
    }

    void AssetEditorWidget::AfterPropertyModified(InstanceDataNode* /*node*/)
    {
        m_dirty = true;
    }

    void AssetEditorWidget::RequestPropertyContextMenu(InstanceDataNode* node, const QPoint& point)
    {
        if (node && node->IsDifferentVersusComparison())
        {
            QMenu menu;
            QAction* resetAction = menu.addAction(tr("Reset value"));
            connect(resetAction, &QAction::triggered, this,
                [this, node]()
                {
                    const InstanceDataNode* orig = node->GetComparisonNode();
                    if (orig)
                    {
                        InstanceDataHierarchy::CopyInstanceData(orig, node, m_serializeContext);

                        PropertyRowWidget* widget = m_propertyEditor->GetWidgetFromNode(node);
                        if (widget)
                        {
                            widget->DoPropertyNotify();
                        }

                        m_propertyEditor->QueueInvalidation(Refresh_Values);
                    }
                }
                );
            menu.exec(point);
        }
    }

    AssetEditorDialog::AssetEditorDialog(QWidget* parent /*= nullptr*/, AZ::SerializeContext* serializeContext /*= nullptr*/)
        : QDialog(parent)
        , m_closeOnSuccess(false)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setContentsMargins(5, 5, 5, 5);

        setWindowTitle(tr("Input Bindings Editor"));

        m_assetEditor = new AssetEditorWidget(this, serializeContext);
        mainLayout->addWidget(m_assetEditor);

        setMinimumSize(QSize(400, 600));

        setLayout(mainLayout);

        // Using QButtonBox to deal with button layout and management, but re-purposing the standard buttons.
        // Save     ->  "Test Changes"
        // Close    ->  "Revert & Close"
        // Cancel   ->  "Save & Close"
        m_buttonBox = new QDialogButtonBox(this);
        m_buttonBox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Cancel | QDialogButtonBox::Close);
        m_buttonBox->button(QDialogButtonBox::Save)->setToolTip(tr("Save current asset to disk. Subscribed systems will be notified of the change, and will receive the new asset data."));
        m_buttonBox->button(QDialogButtonBox::Save)->setText(tr("Test Changes"));
        m_buttonBox->button(QDialogButtonBox::Close)->setToolTip(tr("Restore the original version of the asset on disk and close the dialog."));
        m_buttonBox->button(QDialogButtonBox::Close)->setText(tr("Revert && Close"));
        m_buttonBox->button(QDialogButtonBox::Cancel)->setToolTip(tr("Save the current asset to disk and close the dialog."));
        m_buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);

        mainLayout->addWidget(m_buttonBox);

        connect(m_buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &AssetEditorDialog::OnTestChangesClicked);
        connect(m_buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &AssetEditorDialog::OnRevertAndCloseClicked);
        connect(m_buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &AssetEditorDialog::OnSaveAndCloseClicked);

        connect(m_assetEditor, &AssetEditorWidget::OnAssetSavedQSignal, this, &AssetEditorDialog::OnAssetSaved);
        connect(m_assetEditor, &AssetEditorWidget::OnAssetSaveFailed, this, &AssetEditorDialog::OnAssetSaveFailed);
        connect(m_assetEditor, &AssetEditorWidget::OnAssetReverted, this, &AssetEditorDialog::OnAssetReverted);
        connect(m_assetEditor, &AssetEditorWidget::OnAssetRevertFailed, this, &AssetEditorDialog::OnAssetRevertFailed);
        connect(m_assetEditor, &AssetEditorWidget::OnAssetOpened, this, &AssetEditorDialog::OnAssetOpened);

        OnAssetOpened(AZ::Data::Asset<AZ::Data::AssetData>());
    }

    AssetEditorDialog::~AssetEditorDialog()
    {
    }

    void AssetEditorDialog::SetAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        m_assetEditor->SetAsset(asset);
    }

    void AssetEditorDialog::OnAssetOpened(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        if (asset)
        {
            m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Save && Close"));
            m_buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
            m_buttonBox->button(QDialogButtonBox::Close)->setEnabled(true);

            AZStd::string assetPath;
            EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());
            AZStd::string windowTitle = AZStd::string::format("Edit Asset: %s", assetPath.c_str());
            setWindowTitle(tr(windowTitle.c_str()));
        }
        else
        {
            m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));

            m_buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
            m_buttonBox->button(QDialogButtonBox::Close)->setEnabled(false);

            setWindowTitle(tr("Input Bindings Editor"));
        }
    }

    void AssetEditorDialog::OnTestChangesClicked()
    {
        m_closeOnSuccess = false;
        setEnabled(false);
        m_assetEditor->SaveAsset();
    }

    void AssetEditorDialog::OnRevertAndCloseClicked()
    {
        m_closeOnSuccess = true;
        setEnabled(false);
        m_assetEditor->RevertAsset();
    }

    void AssetEditorDialog::OnSaveAndCloseClicked()
    {
        if (m_assetEditor->IsDirty())
        {
            m_closeOnSuccess = true;
            m_assetEditor->SaveAsset();
            return;
        }

        accept();
    }

    void AssetEditorDialog::OnAssetSaved()
    {
        if (m_closeOnSuccess)
        {
            accept();
        }
        else
        {
            setEnabled(true);
        }
    }

    void AssetEditorDialog::closeEvent(QCloseEvent* event)
    {
        if (m_assetEditor->IsDirty())
        {
            const int result = QMessageBox::question(this,
                    tr("Save Changes?"),
                    tr("Changes have been made to the asset during this session. Would you like to save prior to closing?"),
                    QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

            if (result == QMessageBox::Yes)
            {
                event->ignore();
                OnSaveAndCloseClicked();
                return;
            }
            else if (result == QMessageBox::Cancel)
            {
                event->ignore();
                return;
            }
        }
        reject();
    }

    void AssetEditorDialog::OnAssetSaveFailed(const AZStd::string& error)
    {
        QMessageBox::warning(GetActiveWindow(), tr("Unable to Save Asset"),
            tr(error.c_str()), QMessageBox::Ok, QMessageBox::Ok);

        m_closeOnSuccess = false;
        setEnabled(true);
    }

    void AssetEditorDialog::OnAssetReverted()
    {
        if (m_closeOnSuccess)
        {
            reject();
        }
        else
        {
            setEnabled(true);
        }
    }

    void AssetEditorDialog::OnAssetRevertFailed(const AZStd::string& error)
    {
        QMessageBox::warning(GetActiveWindow(), tr("Unable to Revert Asset"),
            tr(error.c_str()), QMessageBox::Ok, QMessageBox::Ok);

        m_closeOnSuccess = false;
        setEnabled(true);
    }
}   //  namespace AzToolsFramework

#include <UI/AssetEditor/AssetEditorWidget.moc>
