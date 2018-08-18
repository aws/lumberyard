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
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/AssetEditor/AssetEditorWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

#include <QVBoxLayout>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QAction>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AzToolsFramework
{
    namespace AssetEditor
    {
        namespace
        {
            using AssetCheckoutAndSaveCallback = AZStd::function<void(bool, const AZStd::string&)>;

            void AssetCheckoutAndSaveCommon(const AZ::Data::AssetId& id, AZ::Data::Asset<AZ::Data::AssetData> asset, AZ::SerializeContext* serializeContext, AssetCheckoutAndSaveCallback callback)
            {
                AZStd::string assetPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, id);

                AZStd::string assetFullPath;
                AssetSystemRequestBus::Broadcast(&AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, assetPath, assetFullPath);

                if (!assetFullPath.empty())
                {
                    using SCCommandBus = SourceControlCommandBus;
                    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, assetFullPath.c_str(), true,
                        [asset, assetFullPath, serializeContext, callback](bool /*success*/, const SourceControlFileInfo& info)
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
        AssetEditorWidget::AssetEditorWidget(QWidget* parent)
            : QWidget(parent)
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            m_propertyEditor = new ReflectedPropertyEditor(this);
            m_propertyEditor->Setup(m_serializeContext, this, true, 250);
            m_propertyEditor->show();

            QVBoxLayout* mainLayout = new QVBoxLayout();
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            mainLayout->addWidget(m_propertyEditor);

            PopulateGenericAssetTypes();

            QMenuBar* mainMenu = new QMenuBar();
            QMenu* fileMenu = mainMenu->addMenu(tr("&File"));
            // Add Create New Asset menu and populate it with all asset types that have GenericAssetHandler
            QMenu* newAssetMenu = fileMenu->addMenu(tr("&New"));

            for (const auto& assetType : m_genericAssetTypes)
            {
                QString assetTypeName;
                AZ::AssetTypeInfoBus::EventResult(assetTypeName, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
                QAction* newAssetAction = newAssetMenu->addAction(assetTypeName);
                connect(newAssetAction, &QAction::triggered, this, [assetType, this]() { CreateAsset(assetType); });
            }

            QAction* openAssetAction = fileMenu->addAction("&Open");
            connect(openAssetAction, &QAction::triggered, this, &AssetEditorWidget::OpenAsset);

            m_saveAssetAction = fileMenu->addAction("&Save");
            m_saveAssetAction->setEnabled(false);
            connect(m_saveAssetAction, &QAction::triggered, this, &AssetEditorWidget::SaveAsset);

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
                m_dirty = false;
                m_saveAssetAction->setEnabled(false);
                m_propertyEditor->ClearInstances();

                OnAssetOpenedSignal(AZ::Data::Asset<AZ::Data::AssetData>());
            }
        }

        void AssetEditorWidget::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_dirty = false;
            m_saveAssetAction->setEnabled(false);
            m_propertyEditor->ClearInstances();

            m_asset = AZ::Data::AssetManager::Instance().CreateAsset(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), asset.GetType());

            AZStd::vector<AZ::u8> buffer;
            AZ::IO::ByteContainerStream<decltype(buffer)> stream(&buffer);
            auto assetHandler = const_cast<AZ::Data::AssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(asset.GetType()));
            if (!assetHandler->SaveAssetData(asset, &stream))
            {
                m_asset = AZ::Data::Asset<AZ::Data::AssetData>();
                AZ_Error("Asset Editor", false, "Failed to create working asset.");
                return;
            }

            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            if (!assetHandler->LoadAssetData(m_asset, &stream, {}))
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

            OnAssetOpenedSignal(m_assetOriginal);
        }

        void AssetEditorWidget::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        bool AssetEditorWidget::TrySave(const AZStd::function<void()>& savedCallback)
        {
            if (!m_dirty)
            {
                return false;
            }

            const int result = QMessageBox::question(this,
                    tr("Save Changes?"),
                    tr("Changes have been made to the asset during this session. Would you like to save prior to closing?"),
                    QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

            if (result == QMessageBox::No)
            {
                return false;
            }
            if (result == QMessageBox::Yes)
            {
                if (savedCallback)
                {
                    #pragma warning(push)
                    #pragma warning(disable: 4573)	// the usage of 'X' requires the compiler to capture 'this' but the current default capture mode 

                    auto conn = AZStd::make_shared<QMetaObject::Connection>();
                    *conn = connect(this, &AssetEditorWidget::OnAssetSavedSignal, this, [conn, savedCallback]()
                            {
                                disconnect(*conn);
                                savedCallback();
                            });

                    #pragma warning(pop)
                }
                SaveAsset();
            }
            return true;
        }

        void AssetEditorWidget::OpenAsset()
        {
            using namespace AssetBrowser;

            if (TrySave([this]() { OpenAsset(); }))
            {
                return;
            }

            AssetSelectionModel selection = AssetSelectionModel::AssetTypesSelection(m_genericAssetTypes);
            EditorRequests::Bus::Broadcast(&EditorRequests::BrowseForAssets, selection);
            if (selection.IsValid())
            {
                auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
                AZ::Data::Asset<AZ::Data::AssetData> asset =
                    AZ::Data::AssetManager::Instance().GetAsset(product->GetAssetId(), product->GetAssetType(), false);
                SetAsset(asset);
            }
        }

        void AssetEditorWidget::SaveAsset()
        {
            AssetCheckoutAndSaveCommon(m_assetOriginal.GetId(), m_asset, m_serializeContext,
                [this](bool success, const AZStd::string& error)
                {
                    if (success)
                    {
                        m_dirty = false;
                        m_saveAssetAction->setEnabled(false);
                        Q_EMIT OnAssetSavedSignal();
                    }
                    else
                    {
                        Q_EMIT OnAssetSaveFailedSignal(error);
                    }
                }
                );
        }

        void AssetEditorWidget::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);

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

        void AssetEditorWidget::PopulateGenericAssetTypes()
        {
            auto enumerateCallback =
                [this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*typeId*/) -> bool
                {
                    AZ::Data::AssetType assetType = classData->m_typeId;

                    // make sure this is not base class
                    if (assetType == AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid())
                    {
                        return true;
                    }

                    // narrow down all reflected AssetTypeInfos to those that have a GenericAssetHandler
                    AZ::Data::AssetManager& manager = AZ::Data::AssetManager::Instance();
                    if (const AZ::Data::AssetHandler* assetHandler = manager.GetHandler(classData->m_typeId))
                    {
                        if (!azrtti_istypeof<AzFramework::GenericAssetHandlerBase>(assetHandler))
                        {
                            // its not the generic asset handler.
                            return true;
                        }
                    }

                    m_genericAssetTypes.push_back(assetType);
                    return true;
                };

            m_serializeContext->EnumerateDerived(
                enumerateCallback,
                AZ::Uuid::CreateNull(),
                AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid()
                );
        }

        void AssetEditorWidget::CreateAsset(AZ::Data::AssetType assetType)
        {
            if (TrySave([this, assetType]() { CreateAsset(assetType); }))
            {
                return;
            }

            char assetRoot[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@devassets@", assetRoot, AZ_MAX_PATH_LEN);

            AZStd::vector<AZStd::string> assetTypeExtensions;
            AZ::AssetTypeInfoBus::Event(assetType, &AZ::AssetTypeInfo::GetAssetTypeExtensions, assetTypeExtensions);
            QString filter;
            if (assetTypeExtensions.empty())
            {
                filter = tr("All Files (*.*)");
            }
            else
            {
                QString assetTypeName;
                AZ::AssetTypeInfoBus::EventResult(assetTypeName, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
                filter.append(assetTypeName);
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
                AZStd::string targetFilePath(saveAs.toUtf8().constData());

                AZ::Data::Asset<AZ::Data::AssetData> newAsset =
                    AZ::Data::AssetManager::Instance().CreateAsset(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), assetType);

                AZ::IO::FileIOStream fileStream(targetFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
                if (fileStream.IsOpen())
                {
                    auto assetHandler = const_cast<AZ::Data::AssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(assetType));
                    if (assetHandler->SaveAssetData(newAsset, &fileStream))
                    {
                        m_newAssetPath = targetFilePath;
                        m_newAssetType = assetType;
                        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, m_newAssetPath);
                        SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit,
                            targetFilePath.c_str(), true, [](bool, const SourceControlFileInfo&) {});
                    }
                }
            }
        }

        void AssetEditorWidget::AfterPropertyModified(InstanceDataNode* /*node*/)
        {
            m_dirty = true;
            m_saveAssetAction->setEnabled(true);
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
    } // namespace AssetEditor
} // namespace AzToolsFramework

#include <AssetEditor/AssetEditorWidget.moc>
