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

#include "AssetEditorWidget.h"

#include <API/ToolsApplicationAPI.h>
#include <API/EditorAssetSystemAPI.h>

#include <AssetEditor/AssetEditorBus.h>
#include <AssetEditor/Resources/rcc_AssetEditorResources.h>
#include <AssetEditor/ui_AssetEditorToolbar.h>
#include <AssetEditor/ui_AssetEditorStatusBar.h>

#include <AssetBrowser/AssetSelectionModel.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <SourceControl/SourceControlAPI.h>

#include <UI/PropertyEditor/PropertyRowWidget.hxx>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <QVBoxLayout>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QAction>

namespace AzToolsFramework
{
    namespace AssetEditor
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
                    [id, asset, assetFullPath, serializeContext, callback](bool /*success*/, const SourceControlFileInfo& info)
                    {
                        if (!info.IsReadOnly())
                        {
                        AZ::Outcome<bool, AZStd::string> outcome = AZ::Success(true);
                        AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::EventResult(outcome, id, &AzToolsFramework::AssetEditor::AssetEditorValidationRequests::IsAssetDataValid, asset);
                        if (!outcome.IsSuccess())
                        {
                            callback(false, outcome.GetError());
                        }
                        else
                        {
                            AssetEditorValidationRequestBus::Event(id, &AssetEditorValidationRequests::PreAssetSave, asset);

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

        namespace Status
        {
            static QString assetCreated = QStringLiteral("Asset created!");
            static QString assetSaved = QStringLiteral("Asset saved!");
            static QString assetLoaded = QStringLiteral("Asset loaded!");
            static QString unableToSave = QStringLiteral("Failed to save asset due to validation error, check the log!");
            static QString emptyString = QStringLiteral("");
        }

        //////////////////////////////////
        // AssetEditorWidgetUserSettings
        //////////////////////////////////

        void AssetEditorWidgetUserSettings::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetEditorWidgetUserSettings>()
                    ->Version(0)
                    ->Field("m_showPreviewMessage", &AssetEditorWidgetUserSettings::m_lastSavePath)
                    ->Field("m_snapDistance", &AssetEditorWidgetUserSettings::m_recentPaths)
                    ;
            }
        }

        AssetEditorWidgetUserSettings::AssetEditorWidgetUserSettings()
        {
            char assetRoot[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@devassets@", assetRoot, AZ_MAX_PATH_LEN);

            m_lastSavePath = assetRoot;
        }

        void AssetEditorWidgetUserSettings::AddRecentPath(const AZStd::string& recentPath)
        {
            m_recentPaths.emplace(m_recentPaths.begin(), recentPath);

            while (m_recentPaths.size() > 10)
            {
                m_recentPaths.pop_back();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetEditorWidget
        //////////////////////////////////////////////////////////////////////////

        const AZ::Crc32 k_assetEditorWidgetSettings = AZ_CRC("AssetEditorSettings", 0xfe740dee);

        AssetEditorWidget::AssetEditorWidget(QWidget* parent)
            : QWidget(parent)
            , m_statusBar(new Ui::AssetEditorStatusBar())
            , m_dirty(true)
            , m_isNewAsset(true)
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

            QWidget* statusBarWidget = new QWidget(this);
            m_statusBar->setupUi(statusBarWidget);
            m_statusBar->textEdit->setPlainText(Status::emptyString);

            mainLayout->addWidget(statusBarWidget);

            PopulateGenericAssetTypes();

            QMenuBar* mainMenu = new QMenuBar();
            QMenu* fileMenu = mainMenu->addMenu(tr("&File"));
            // Add Create New Asset menu and populate it with all asset types that have GenericAssetHandler
            QMenu* newAssetMenu = fileMenu->addMenu(tr("&New"));

            for (const auto& assetType : m_genericAssetTypes)
            {
                QString assetTypeName;
                AZ::AssetTypeInfoBus::EventResult(assetTypeName, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

                if (!assetTypeName.isEmpty())
                {
                    QAction* newAssetAction = newAssetMenu->addAction(assetTypeName);
                    connect(newAssetAction, &QAction::triggered, this, [assetType, this]() { CreateAssetImpl(assetType); });
                }
            }

            QAction* openAssetAction = fileMenu->addAction("&Open");
            connect(openAssetAction, &QAction::triggered, this, &AssetEditorWidget::OpenAssetWithDialog);

            m_recentFileMenu = fileMenu->addMenu("Open Recent");

            m_saveAssetAction = fileMenu->addAction("&Save");
            m_saveAssetAction->setShortcut(QKeySequence::Save);
            m_saveAssetAction->setEnabled(true);
            connect(m_saveAssetAction, &QAction::triggered, this, &AssetEditorWidget::SaveAsset);

            m_saveAsAssetAction = fileMenu->addAction("&Save As");
            m_saveAsAssetAction->setShortcut(QKeySequence::SaveAs);
            m_saveAsAssetAction->setEnabled(true);

            connect(m_saveAsAssetAction, &QAction::triggered, this, &AssetEditorWidget::SaveAssetAs);

            QMenu* viewMenu = mainMenu->addMenu(tr("&View"));

            QAction* expandAll = viewMenu->addAction("&Expand All");
            connect(expandAll, &QAction::triggered, this, &AssetEditorWidget::ExpandAll);

            QAction* collapseAll = viewMenu->addAction("&Collapse All");
            connect(collapseAll, &QAction::triggered, this, &AssetEditorWidget::CollapseAll);

            mainLayout->setMenuBar(mainMenu);

            setLayout(mainLayout);

            AzFramework::AssetCatalogEventBus::Handler::BusConnect();

            m_userSettings = AZ::UserSettings::CreateFind<AssetEditorWidgetUserSettings>(k_assetEditorWidgetSettings, AZ::UserSettings::CT_LOCAL);

            PopulateRecentMenu();
        }

        void AssetEditorWidget::CreateAsset(AZ::Data::AssetType assetType)
        {
            auto typeIter = AZStd::find_if(m_genericAssetTypes.begin(), m_genericAssetTypes.end(), [assetType](const AZ::Data::AssetType& testType) { return assetType == testType; });

            if (typeIter != m_genericAssetTypes.end())
            {
                CreateAssetImpl(assetType);
            }
            else
            {
                AZ_Assert(false, "The AssetEditorWidget only supports Generic Asset Types, make sure your type has a handler that implements GenericAssetHandler");
            }
        }

        void AssetEditorWidget::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_dirty = false;

            AZ::Data::AssetBus::Handler::BusDisconnect(asset.GetId());

            m_inMemoryAsset = asset;

            UpdatePropertyEditor(m_inMemoryAsset);

            if (m_isNewAsset)
            {
                SetStatusText(Status::assetCreated);
            }
            else
            {
                SetStatusText(Status::assetLoaded);
            }
        }

        void AssetEditorWidget::UpdatePropertyEditor(AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            m_propertyEditor->ClearInstances();

            AZ::Crc32 saveStateKey;
            saveStateKey.Add(&asset.GetId(), sizeof(AZ::Data::AssetId));

            // LW-DV ML Integration:  Fix!  (possible regression of LY-92830)
            // Set the assetOriginal to the loaded one, so it updates state properly when an asset is saved
            //m_assetOriginal = asset;
            // LW-DV ML Integration:  Fix!  (possible regression of LY-92830)

            m_propertyEditor->SetSavedStateKey(saveStateKey);
            m_propertyEditor->AddInstance(asset.Get(), asset.GetType(), nullptr);

            m_propertyEditor->InvalidateAll();
            m_propertyEditor->setEnabled(true);

        }

        void AssetEditorWidget::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_dirty = false;
            m_saveAssetAction->setEnabled(false);
            m_propertyEditor->ClearInstances();

            QString errString = tr("Failed to load %1!").arg(asset.GetHint().c_str());
            AZ_Error("Asset Editor", false, errString.toUtf8());
            QMessageBox::warning(this, tr("Error!"), errString);
        }

        bool AssetEditorWidget::TrySave(const AZStd::function<void()>& savedCallback)
        {
            if (m_isNewAsset || !m_dirty)
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
                    #pragma warning(disable: 4573)    // the usage of 'X' requires the compiler to capture 'this' but the current default capture mode 

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

        bool AssetEditorWidget::SaveAsDialog(AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            AZStd::vector<AZStd::string> assetTypeExtensions;
            AZ::AssetTypeInfoBus::Event(asset.GetType(), &AZ::AssetTypeInfo::GetAssetTypeExtensions, assetTypeExtensions);
            QString filter;
            if (assetTypeExtensions.empty())
            {
                filter = tr("All Files (*.*)");
            }
            else
            {
                QString assetTypeName;
                AZ::AssetTypeInfoBus::EventResult(assetTypeName, asset.GetType(), &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
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

            const QString saveAs = QFileDialog::getSaveFileName(nullptr, tr("Save As..."), m_userSettings->m_lastSavePath.c_str(), filter);

            if (!saveAs.isEmpty())
            {
                AZStd::string targetFilePath(saveAs.toUtf8().constData());

                m_propertyEditor->ForceQueuedInvalidation();

                AZStd::vector<char> byteBuffer;
                AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);

                auto assetHandler = const_cast<AZ::Data::AssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(asset.GetType()));
                if (assetHandler->SaveAssetData(asset, &byteStream))
                {
                    AZ::IO::FileIOStream fileStream(targetFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
                    if (fileStream.IsOpen())
                    {
                        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, targetFilePath);
                        m_expectedAddedAssetPath = targetFilePath;

                        SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, targetFilePath.c_str(), true, [](bool, const SourceControlFileInfo&) {});

                        fileStream.Write(byteBuffer.size(), byteBuffer.data());

                        m_isNewAsset = false;

                        AZStd::string fileName = targetFilePath;

                        if (AzFramework::StringFunc::Path::Normalize(fileName))
                        {
                            AzFramework::StringFunc::Path::StripPath(fileName);
                            m_currentAsset = fileName.c_str();

                            m_userSettings->m_lastSavePath = targetFilePath;
                            AZStd::size_t findIndex = m_userSettings->m_lastSavePath.find(fileName.c_str());

                            if (findIndex != AZStd::string::npos)
                            {
                                m_userSettings->m_lastSavePath = m_userSettings->m_lastSavePath.substr(0, findIndex);
                            }
                        }

                        AddRecentPath(targetFilePath);

                        SetStatusText(Status::assetCreated);
                    }

                    return true;
                }
                else
                {
                    SetStatusText(Status::unableToSave);
                    return false;
                }
            }

            return false;
        }

        void AssetEditorWidget::OpenAsset(const AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // If an unsaved asset is open, ask.
            if (TrySave([this]() { OpenAssetWithDialog(); }))
            {
                return;
            }

            AZ::Data::AssetInfo typeInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(typeInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, asset.GetId());

            // Check if there is an asset to open
            if (typeInfo.m_relativePath.empty())
            {
                return;
            }

            bool hasResult = false;
            AZStd::string fullPath;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(hasResult, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, typeInfo.m_relativePath, fullPath);

            AZStd::string fileName = typeInfo.m_relativePath;

            if (AzFramework::StringFunc::Path::Normalize(fileName))
            {
                AzFramework::StringFunc::Path::StripPath(fileName);
                m_currentAsset = fileName.c_str();
            }

            AddRecentPath(fullPath.c_str());
            m_isNewAsset = false;

            LoadAsset(asset.GetId(), asset.GetType());
        }

        void AssetEditorWidget::OpenAssetWithDialog()
        {
            // If an unsaved asset is open, ask.
            if (TrySave([this]() { OpenAssetWithDialog(); }))
            {
                return;
            }

            AssetSelectionModel selection = AssetSelectionModel::AssetTypesSelection(m_genericAssetTypes);
            EditorRequests::Bus::Broadcast(&EditorRequests::BrowseForAssets, selection);
            if (selection.IsValid())
            {
                auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
                m_currentAsset = product->GetName().c_str();

                AddRecentPath(product->GetFullPath());

                LoadAsset(product->GetAssetId(), product->GetAssetType());
                m_isNewAsset = false;
            }
        }

        void AssetEditorWidget::OpenAssetFromPath(const AZStd::string& assetPath)
        {
            bool hasResult = false;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(hasResult, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, assetPath.c_str(), assetInfo, watchFolder);

            if (hasResult)
            {
                AZStd::string fileName = assetPath;

                if (AzFramework::StringFunc::Path::Normalize(fileName))
                {
                    AzFramework::StringFunc::Path::StripPath(fileName);
                    m_currentAsset = fileName.c_str();
                }

                AZ::Data::AssetInfo typeInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(typeInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetInfo.m_assetId);

                LoadAsset(assetInfo.m_assetId, typeInfo.m_assetType);
                m_isNewAsset = false;
            }
        }

        void AssetEditorWidget::LoadAsset(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType)
        {
            auto asset = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType);
            if (asset.IsReady())
            {
                OnAssetReady(asset);
            }
            else
            {
                AZ::Data::AssetBus::Handler::BusConnect(asset.GetId());

                // Need to disable editing until OnAssetReady.
                m_propertyEditor->setEnabled(false);
            }
        }

        void AssetEditorWidget::SaveAsset()
        {
            if (m_isNewAsset)
            {
                SaveAsDialog(m_inMemoryAsset);
            }
            else if (m_dirty)
            {
                AssetCheckoutAndSaveCommon(m_inMemoryAsset.Get()->GetId(), m_inMemoryAsset, m_serializeContext,
                [this](bool success, const AZStd::string& error)
                {
                    if (success)
                    {
                        m_dirty = false;
                        m_saveAssetAction->setEnabled(false);
                        m_saveAsAssetAction->setEnabled(false);

                        Q_EMIT OnAssetSavedSignal();
                        m_propertyEditor->QueueInvalidation(Refresh_AttributesAndValues);
                        SetStatusText(Status::assetSaved);
                    }
                    else
                    {
                        // Don't want to dirty the asset here, since we are setting ourselves into a weird state since we were unable to save.
                        m_dirty = true;
                        m_saveAssetAction->setEnabled(false);
                        m_saveAsAssetAction->setEnabled(false);

                        SetStatusText(Status::unableToSave);
                        Q_EMIT OnAssetSaveFailedSignal(error);
                    }
                }
                );
            }
        }

        void AssetEditorWidget::SaveAssetAs()
        {
            SaveAsDialog(m_inMemoryAsset);
        }

        void AssetEditorWidget::OnNewAsset()
        {
            // This only works if we've already made a new asset or opened an asset of a given type
            // We use that knowledge to create another asset of the same type.
            if (!m_inMemoryAsset.GetType().IsNull())
            {
                CreateAssetImpl(m_inMemoryAsset.GetType());
            }
        }

        void AssetEditorWidget::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
            if (assetInfo.m_assetType == m_inMemoryAsset.GetType()
                && strstr(m_expectedAddedAssetPath.c_str(), assetInfo.m_relativePath.c_str()) != 0)
            {
                m_expectedAddedAssetPath.clear();
                m_recentlyAddedAssetPath = assetInfo.m_relativePath;

                LoadAsset(assetId, assetInfo.m_assetType);
                m_isNewAsset = false;
            }
        }

        void AssetEditorWidget::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
            if (assetInfo.m_assetType == m_inMemoryAsset.GetType()
                && assetInfo.m_relativePath.compare(m_recentlyAddedAssetPath) == 0)
            {
                m_recentlyAddedAssetPath.clear();

                // The file was removed due to errors, but the user needs to be 
                // given a chance to fix the errors and try again.
                m_isNewAsset = true;
                m_currentAsset = "New Asset";
                m_propertyEditor->setEnabled(true);

                DirtyAsset();
            }
        }

        void AssetEditorWidget::PopulateGenericAssetTypes()
        {
            auto enumerateCallback =
                [this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*typeId*/) -> bool
                {
                    AZ::Data::AssetType assetType = classData->m_typeId;

                    // make sure this is not base class                    
                    if (assetType == AZ::AzTypeInfo<AZ::Data::AssetData>::Uuid()
                        || AZ::FindAttribute(AZ::Edit::Attributes::EnableForAssetEditor, classData->m_attributes) == nullptr)
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

        void AssetEditorWidget::CreateAssetImpl(AZ::Data::AssetType assetType)
        {
            m_isNewAsset = true;

            // If an unsaved asset is open, ask.
            if (TrySave([this, assetType]() { CreateAssetImpl(assetType); }))
            {
                return;
            }

            if (m_inMemoryAsset)
            {
                m_inMemoryAsset.Release();
            }

            AZ::Data::AssetId newAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
            m_inMemoryAsset = AZ::Data::AssetManager::Instance().CreateAsset(newAssetId, assetType);

            DirtyAsset();

            m_propertyEditor->ClearInstances();
            m_currentAsset = "New Asset";

            UpdatePropertyEditor(m_inMemoryAsset);

            ExpandAll();
            SetStatusText(Status::assetCreated);
        }

        void AssetEditorWidget::AfterPropertyModified(InstanceDataNode* /*node*/)
        {
            DirtyAsset();
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

        void AssetEditorWidget::BeforePropertyModified(InstanceDataNode* node)
        {
            AssetEditorValidationRequestBus::Event(m_inMemoryAsset.Get()->GetId(), &AssetEditorValidationRequests::BeforePropertyEdit, node, m_inMemoryAsset);
        }

        void AssetEditorWidget::ExpandAll()
        {
            m_propertyEditor->ExpandAll();
        }

        void AssetEditorWidget::CollapseAll()
        {
            m_propertyEditor->CollapseAll();
        }

        void AssetEditorWidget::DirtyAsset()
        {
            m_dirty = true;
            m_saveAssetAction->setEnabled(true);
            m_saveAsAssetAction->setEnabled(true);

            SetStatusText(Status::emptyString);
        }

        void AssetEditorWidget::SetStatusText(const QString& assetStatus)
        {
            QString statusString;

            if (m_dirty)
            {
                statusString = QString("%1*");
            }
            else
            {
                statusString = QString("%1");
            }

            statusString = statusString.arg(m_currentAsset).arg(assetStatus);

            if (!assetStatus.isEmpty())
            {
                statusString.append(" - ");
                statusString.append(assetStatus);
            }

            m_statusBar->textEdit->setPlainText(statusString);
        }

        void AssetEditorWidget::AddRecentPath(const AZStd::string& recentPath)
        {
            m_userSettings->AddRecentPath(recentPath);
            PopulateRecentMenu();
        }

        void AssetEditorWidget::PopulateRecentMenu()
        {
            m_recentFileMenu->clear();

            if (m_userSettings->m_recentPaths.empty())
            {
                m_recentFileMenu->setEnabled(false);
            }
            else
            {
                m_recentFileMenu->setEnabled(true);

                for (const AZStd::string& recentFile : m_userSettings->m_recentPaths)
                {
                    bool hasResult = false;
                    AZStd::string relativePath;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(hasResult, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, recentFile, relativePath);

                    if (hasResult)
                    {
                        QAction* action = m_recentFileMenu->addAction(relativePath.c_str());
                        connect(action, &QAction::triggered, this, [recentFile, this]() { this->OpenAssetFromPath(recentFile); });
                    }
                }
            }
        }

    } // namespace AssetEditor
} // namespace AzToolsFramework

#include <AssetEditor/AssetEditorWidget.moc>
