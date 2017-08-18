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
#include "AssetBrowserRequestHandler.h"

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Prefab/PrefabAsset.h>
#include <AzCore/Prefab/PrefabComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetDatabase.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/UI/AssetEditor/AssetEditorWidget.hxx>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/AssetBrowser/AssetCache/AssetClassIdBus.h>
#include <AzToolsFramework/AssetBrowser/UI/Entry.h>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

#include "Viewport.h"
#include "ViewManager.h"
#include <MathConversion.h>
#include <QMessageBox>

namespace AssetBrowser
{
    using namespace UI;
    using namespace AssetCache;

    void SpawnEntityAtPoint(CViewport* viewport, int ptx, int pty, void* custom)
    {
        // Calculate the drop location.
        Vec3 pos = Vec3(ZERO);
        if (viewport)
        {
            QPoint vp(ptx, pty);
            HitContext hit;
            if (viewport->HitTest(vp, hit))
            {
                pos = hit.raySrc + hit.rayDir * hit.dist;
                pos = viewport->SnapToGrid(pos);
            }
            else
            {
                bool hitTerrain;
                pos = viewport->ViewToWorld(vp, &hitTerrain);
                if (hitTerrain)
                {
                    pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);
                }
                pos = viewport->SnapToGrid(pos);
            }
        }
        const AZ::Transform worldTransform = AZ::Transform::CreateTranslation(LYVec3ToAZVec3(pos));

        QMimeData* mimeData = static_cast<QMimeData*>(custom);

        AZStd::vector<Entry*> entries;
        if (!Entry::FromMimeData(mimeData, entries))
        {
            return;
        }

        bool entityCreated = false;

        for (auto entry : entries)
        {
            auto products = entry->GetProducts();
            if (products.size() == 0)
            {
                continue;
            }

            for (auto product : products)
            {
                // Handle instantiation of slices.
                if (product->getAssetType() == AZ::AzTypeInfo<AZ::PrefabAsset>::Uuid())
                {
                    // Instantiate the slice at the specified location.
                    AZ::Data::Asset<AZ::PrefabAsset> asset =
                        AZ::Data::AssetDatabase::Instance().GetAsset<AZ::PrefabAsset>(product->getAssetId(), false);
                    if (asset)
                    {
                        EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, InstantiateEditorSlice, asset, worldTransform);
                        entityCreated = true;
                        break;
                    }
                }
                else
                {
                    //  Add the component(s).
                    const AZ::Data::AssetId assetId = product->getAssetId();
                    AZ::Uuid componentType;
                    EBUS_EVENT_RESULT(componentType, AssetCache::AssetClassIdBus, GetClassId, product->getAssetType());
                    if (!componentType.IsNull())
                    {
                        AzToolsFramework::ScopedUndoBatch undo("Create entity from asset");

                        AZStd::string entityName;

                        // If the entity is being created from an asset, name it after said asset.
                        AZStd::string assetPath;
                        EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);
                        if (!assetPath.empty())
                        {
                            AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), entityName);
                        }

                        // If not sourced from an asset, generate a generic name.
                        if (entityName.empty())
                        {
                            entityName = AZStd::string::format("Entity%d", GetIEditor()->GetObjectManager()->GetObjectCount());
                        }

                        AZ::Entity* newEntity = aznew AZ::Entity(entityName.c_str());
                        EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, AddRequiredComponents, *newEntity);

                        // Create Entity metrics event (Drag+Drop from Asset Browser to Viewport)
                        EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, EntityCreated, newEntity->GetId());

                        AZ::Component* newComponent = newEntity->CreateComponent(componentType);
                        if (newComponent)
                        {
                            // Add Component metrics event (Drag+Drop from Asset Browser to View port to create a new entity with the component)
                            EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, ComponentAdded, newEntity->GetId(), componentType);

                        }

                        //  Set entity position.
                        auto* transformComponent = newEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();
                        if (transformComponent)
                        {
                            transformComponent->SetWorldTM(worldTransform);
                        }

                        // Add the entity to the editor context, which activates it and creates the sandbox object.
                        EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, AddEditorEntity, newEntity);

                        // set asset after components have been activated in AddEditorEntity method
                        AzToolsFramework::Components::EditorComponentBase* asEditorComponent =
                            azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(newComponent);

                        if (asEditorComponent)
                        {
                            asEditorComponent->SetPrimaryAsset(assetId);
                        }

                        // Prepare undo command last so it captures the final state of the entity.
                        AzToolsFramework::EntityCreateCommand* command = aznew AzToolsFramework::EntityCreateCommand(static_cast<AZ::u64>(newEntity->GetId()));
                        command->Capture(newEntity);
                        command->SetParent(undo.GetUndoBatch());

                        // Select the new entity (and deselect others).
                        AzToolsFramework::EntityIdList selection = { newEntity->GetId() };
                        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetSelectedEntities, selection);
                        entityCreated = true;
                        break;
                    }
                }
                if (entityCreated)
                {
                    break;
                }
            }
        }

        //  Clear the drop callback
        for (int i = 0; i < GetIEditor()->GetViewManager()->GetViewCount(); i++)
        {
            GetIEditor()->GetViewManager()->GetView(i)->SetGlobalDropCallback(nullptr, nullptr);
        }
    }

    AssetBrowserRequestHandler::AssetBrowserRequestHandler()
    {
        BusConnect();
    }


    AssetBrowserRequestHandler::~AssetBrowserRequestHandler()
    {
        BusDisconnect();
    }

    void AssetBrowserRequestHandler::StartDrag(QMimeData* data)
    {
        for (int i = 0; i < GetIEditor()->GetViewManager()->GetViewCount(); i++)
        {
            GetIEditor()->GetViewManager()->GetView(i)->SetGlobalDropCallback(&SpawnEntityAtPoint, data);
        }
    }

    void AssetBrowserRequestHandler::OnItemContextMenu(QWidget* parent, Entry* entry)
    {
        if (!entry)
        {
            return;
        }

        QStringList extraItemsFront;
        QStringList extraItemsBack;
        QString selectedOption;
        AZStd::string fullGamePath = entry->GetFullPath();
        AZStd::string fileName = entry->GetName();

        switch (entry->GetEntryType())
        {
        case Entry::AssetEntryType::Source:
        case Entry::AssetEntryType::Product:
        {
            auto products = entry->GetProducts();
            if (products.size() == 0)
            {
                return;
            }
            auto productEntry = products[0];

            AZ::SerializeContext* serializeContext = nullptr;
            EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            const AZ::SerializeContext::ClassData* assetClassData = serializeContext->FindClassData(productEntry->getAssetType());
            AZ_Assert(serializeContext, "Failed to retrieve serialize context.");
            const QString openInAssetEditorStr = QObject::tr("Asset Editor");

            // For slices, we provide the option to toggle the dynamic flag.
            AZ::Entity* prefabEntity = nullptr;
            AZ::PrefabComponent* prefabAsset = nullptr;
            QString sliceOptions[] = { QObject::tr("Set Dynamic Flag"), QObject::tr("Unset Dynamic Flag") };
            if (productEntry->getAssetType() == AZ::AzTypeInfo<AZ::PrefabAsset>::Uuid())
            {
                prefabEntity = AZ::Utils::LoadObjectFromFile<AZ::Entity>(fullGamePath, nullptr, AZ::ObjectStream::FilterDescriptor(AZ::ObjectStream::AssetFilterNoAssetLoading));
                prefabAsset = prefabEntity ? prefabEntity->FindComponent<AZ::PrefabComponent>() : nullptr;
                if (prefabAsset)
                {
                    extraItemsFront << (prefabAsset->IsDynamic() ? sliceOptions[1] : sliceOptions[0]);
                }
            }

            extraItemsBack << openInAssetEditorStr;

            selectedOption = CFileUtil::PopupQMenu(fileName.c_str(), fullGamePath.c_str(), parent, nullptr, extraItemsFront, extraItemsBack);

            bool resaveSlice = false;
            if (selectedOption == sliceOptions[0] /*Set dynamic flag*/)
            {
                prefabAsset->SetIsDynamic(true);
                resaveSlice = true;
            }
            else if (selectedOption == sliceOptions[1] /*Unset dynamic flag*/)
            {
                prefabAsset->SetIsDynamic(false);
                resaveSlice = true;
            }
            else if (selectedOption == openInAssetEditorStr)
            {
                AzToolsFramework::AssetEditorDialog* dialog = new AzToolsFramework::AssetEditorDialog(parent, serializeContext);
                if (productEntry && productEntry->getAssetId().IsValid())
                {
                    if (assetClassData && assetClassData->m_editData)
                    {
                        AZ::Data::Asset<AZ::Data::AssetData> asset =
                            AZ::Data::AssetDatabase::Instance().GetAsset(productEntry->getAssetId(), productEntry->getAssetType(), false);
                        dialog->SetAsset(asset);
                    }
                }
                QObject::connect(dialog, &QDialog::finished, parent, [dialog]() { dialog->deleteLater(); });
                dialog->show();
            }

            if (resaveSlice)
            {
                EBUS_EVENT(AzToolsFramework::SourceControlCommands::Bus, RequestEdit, fullGamePath.c_str(), true,
                    [prefabEntity, fullGamePath, parent](bool success, const AzToolsFramework::SourceControlFileInfo& info)
                {
                    if (!info.IsReadOnly())
                    {
                        AZ::IO::FileIOStream fileStream(fullGamePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
                        if (fileStream.IsOpen())
                        {
                            bool saved = AZ::Utils::SaveObjectToStream<AZ::Entity>(fileStream, AZ::DataStream::ST_XML, prefabEntity);
                            if (saved)
                            {
                                // Bump the slice asset up in the asset processor's queue.
                                EBUS_EVENT(AzFramework::AssetSystemRequestBus, GetAssetStatus, fullGamePath);
                            }
                        }
                    }
                    else
                    {
                        QWidget* mainWindow = nullptr;
                        EBUS_EVENT_RESULT(mainWindow, AzToolsFramework::EditorRequests::Bus, GetMainWindow);
                        QMessageBox::warning(mainWindow, QObject::tr("Unable to Modify Slice"),
                            QObject::tr("File is not writable."), QMessageBox::Ok, QMessageBox::Ok);
                    }

                    delete prefabEntity;
                }
                );
            }
            else
            {
                delete prefabEntity;
            }
        }
        break;
        default:
            break;
        }
    }
} // namespace AssetBrowser