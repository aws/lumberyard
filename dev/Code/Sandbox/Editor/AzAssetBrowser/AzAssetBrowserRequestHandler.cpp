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

#include <Editor/AzAssetBrowser/AzAssetBrowserRequestHandler.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/UI/AssetEditor/AssetEditorWidget.hxx>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

#include "Viewport.h"
#include "ViewManager.h"
#include <MathConversion.h>
#include <QMessageBox>

namespace AzAssetBrowserRequestHandlerPrivate
{
    bool IsDragOfFBX(const QMimeData* mimeData);

    void SpawnEntityAtPoint(CViewport* viewport, int ptx, int pty, void* custom)
    {
        // Calculate the drop location.
        Vec3 pos = Vec3(ZERO);
        if (viewport)
        {
            viewport->PreWidgetRendering();

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

            viewport->PostWidgetRendering();
        }
        const AZ::Transform worldTransform = AZ::Transform::CreateTranslation(LYVec3ToAZVec3(pos));

        QMimeData* mimeData = static_cast<QMimeData*>(custom);

        AZStd::vector<AssetBrowserEntry*> entries;
        if (!AssetBrowserEntry::FromMimeData(mimeData, entries))
        {
            return;
        }

        bool isDraggingFBXFile = IsDragOfFBX(mimeData);
        bool entityCreated = false;

        for (auto entry : entries)
        {
            AZStd::vector<const ProductAssetBrowserEntry*> products;
            entry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);
            if (products.size() == 0)
            {
                continue;
            }

            for (auto product : products)
            {
                // Handle instantiation of slices.
                if (product->GetAssetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
                {
                    // Instantiate the slice at the specified location.
                    AZ::Data::Asset<AZ::SliceAsset> asset =
                        AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(product->GetAssetId(), false);
                    if (asset)
                    {
                        AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::DragAndDrop);

                        AZStd::string idString;
                        asset.GetId().ToString(idString);

                        AzToolsFramework::EditorMetricsEventsBus::Broadcast(&AzToolsFramework::EditorMetricsEventsBusTraits::SliceInstantiated, AZ::Crc32(idString.c_str()));

                        EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, InstantiateEditorSlice, asset, worldTransform);
                        entityCreated = true;
                        break;
                    }
                }
                else
                {
                    //  Add the component(s).

                    if (isDraggingFBXFile)
                    {
                        // ignore MTL files when dragging an entire fbx file.
                        // note that the above UUID is being put directly here to avoid having to cross-link LmbrCentral with AzToolsframework
                        static const AZ::Data::AssetType materialAssetType("{F46985B5-F7FF-4FCB-8E8C-DC240D701841}");
                        if (product->GetAssetType() == materialAssetType)
                        {
                            continue;
                        }

                    }

                    AZ::Uuid componentId = AZ::Uuid::CreateNull();
                    AZ::AssetTypeInfoBus::EventResult(componentId, product->GetAssetType(), &AZ::AssetTypeInfo::GetComponentTypeId);
                    if (!componentId.IsNull())
                    {
                        AzToolsFramework::ScopedUndoBatch undo("Create entity from asset");

                        AZStd::string entityName;

                        // If the entity is being created from an asset, name it after said asset.
                        const AZ::Data::AssetId assetId = product->GetAssetId();
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

                        AZ::Component* newComponent = newEntity->CreateComponent(componentId);
                        if (newComponent)
                        {
                            // Add Component metrics event (Drag+Drop from Asset Browser to View port to create a new entity with the component)
                            EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, ComponentAdded, newEntity->GetId(), componentId);
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
                        if (newComponent)
                        {
                            AzToolsFramework::Components::EditorComponentBase* asEditorComponent =
                                azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(newComponent);

                            if (asEditorComponent)
                            {
                                asEditorComponent->SetPrimaryAsset(assetId);
                            }
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

    // Helper utility - determines if the thing being dragged is a FBX from the scene import pipeline
    // This is important to differentiate.
    // when someone drags a MTL file directly into the viewport, even from a FBX, we want to spawn it as a decal
    // but when someone drags a FBX that contains MTL files, we want only to spawn the meshes.
    // so we have to specifically differentiate here between the mimeData type that contains the source as the root
    // (dragging the fbx file itself)
    // and one which contains the actual product at its root.

    bool IsDragOfFBX(const QMimeData* mimeData)
    {
        AZStd::vector<AssetBrowserEntry*> entries;
        if (!AssetBrowserEntry::FromMimeData(mimeData, entries))
        {
            // if mimedata does not even contain entries, no point in proceeding.
            return false;
        }

        for (auto entry : entries)
        {
            if (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Source)
            {
                continue;
            }
            // this is a source file.  Is it the filetype we're looking for?
            if (SourceAssetBrowserEntry* source = azrtti_cast<SourceAssetBrowserEntry*>(entry))
            {
                if (AzFramework::StringFunc::Equal(source->GetExtension().c_str(), ".fbx", false))
                {
                    return true;
                }
            }
        }
        return false;
    }
}

AzAssetBrowserRequestHandler::AzAssetBrowserRequestHandler()
{
    BusConnect();
}

AzAssetBrowserRequestHandler::~AzAssetBrowserRequestHandler()
{
    BusDisconnect();
}

void AzAssetBrowserRequestHandler::StartDrag(QMimeData* data)
{
    for (int i = 0; i < GetIEditor()->GetViewManager()->GetViewCount(); i++)
    {
        GetIEditor()->GetViewManager()->GetView(i)->SetGlobalDropCallback(&AzAssetBrowserRequestHandlerPrivate::SpawnEntityAtPoint, data);
    }
}

void AzAssetBrowserRequestHandler::AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<AssetBrowserEntry*>& entries)
{
    AssetBrowserEntry* entry = entries.empty() ? nullptr : entries.front();
    if (!entry)
    {
        return;
    }

    AZStd::string fullFileDirectory;
    AZStd::string fullFilePath;
    AZStd::string fileName;

    switch (entry->GetEntryType())
    {
    case AssetBrowserEntry::AssetEntryType::Product:
        // if its a product, we actually want to perform these operations on the source
        // which will be the parent of the product.
        entry = entry->GetParent();
        if ((!entry) || (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Source))
        {
            AZ_Assert(false, "Asset Browser entry product has a non-source parent?");
            break;     // no valid parent.
        }
        // the fall through to the next case is intentional here.
    case AssetBrowserEntry::AssetEntryType::Source:
    {
        fullFilePath = entry->GetFullPath();
        fullFileDirectory = fullFilePath.substr(0, fullFilePath.find_last_of(AZ_CORRECT_DATABASE_SEPARATOR));
        fileName = entry->GetName();

        AZStd::vector<const ProductAssetBrowserEntry*> products;
        entry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);
        if (products.empty())
        {
            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
            {
                CFileUtil::PopulateQMenu(caller, menu, fileName.c_str(), fullFileDirectory.c_str(), nullptr, true);
            }
            return;
        }
        auto productEntry = products[0];

        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        const AZ::SerializeContext::ClassData* assetClassData = serializeContext->FindClassData(productEntry->GetAssetType());
        AZ_Assert(serializeContext, "Failed to retrieve serialize context.");
        

        // For slices, we provide the option to toggle the dynamic flag.
        bool includeOpen = true;
        QString sliceOptions[] = { QObject::tr("Set Dynamic Slice"), QObject::tr("Unset Dynamic Slice") };
        if (productEntry->GetAssetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
        {
            includeOpen = false;

            AZ::Entity* sliceEntity = AZ::Utils::LoadObjectFromFile<AZ::Entity>(fullFilePath, nullptr, 
                                                                                 AZ::ObjectStream::FilterDescriptor(AZ::ObjectStream::AssetFilterNoAssetLoading));
            AZ::SliceComponent* sliceAsset = sliceEntity ? sliceEntity->FindComponent<AZ::SliceComponent>() : nullptr;
            if (sliceAsset)
            {
                if (sliceAsset->IsDynamic())
                {
                    menu->addAction(sliceOptions[1], [sliceEntity, fullFilePath]()
                    {
                        /*Unset dynamic slice*/
                        AZ::SliceComponent* sliceAsset = sliceEntity->FindComponent<AZ::SliceComponent>();
                        AZ_Assert(sliceAsset, "SliceComponent no longer present on component.");
                        sliceAsset->SetIsDynamic(false);
                        ResaveSlice(sliceEntity, fullFilePath);
                    });
                }
                else
                {
                    menu->addAction(sliceOptions[0], [sliceEntity, fullFilePath]()
                    {
                        /*Set dynamic slice*/
                        AZ::SliceComponent* sliceAsset = sliceEntity->FindComponent<AZ::SliceComponent>();
                        AZ_Assert(sliceAsset, "SliceComponent no longer present on component.");
                        sliceAsset->SetIsDynamic(true);
                        ResaveSlice(sliceEntity, fullFilePath);
                    });
                }
            }
            else
            {
                delete sliceEntity;
            }
        }
        // We only let users edit reflected assets using the asset editor, slices when we're ready
        else if (assetClassData && assetClassData->m_editData)
        {
            includeOpen = false;

            const QString openInAssetEditorStr = QObject::tr("Open");
            menu->addAction(openInAssetEditorStr, [=]()
            {
                AzToolsFramework::AssetEditorDialog* dialog = new AzToolsFramework::AssetEditorDialog(caller, serializeContext);
                if (productEntry && productEntry->GetAssetId().IsValid())
                {
                    if (assetClassData && assetClassData->m_editData)
                    {
                        AZ::Data::Asset<AZ::Data::AssetData> asset =
                            AZ::Data::AssetManager::Instance().GetAsset(productEntry->GetAssetId(), productEntry->GetAssetType(), false);
                        dialog->SetAsset(asset);
                    }
                }
                QObject::connect(dialog, &QDialog::finished, caller, [dialog]() { dialog->deleteLater(); });
                dialog->show();
            });
        }
        CFileUtil::PopulateQMenu(caller, menu, fileName.c_str(), fullFileDirectory.c_str(), nullptr, includeOpen);
    }
    break;
    case AssetBrowserEntry::AssetEntryType::Folder:
    {
        fullFileDirectory = entry->GetFullPath();
        // we are sending an empty filename to indicate that it is a folder and not a file
        CFileUtil::PopulateQMenu(caller, menu, fileName.c_str(), fullFileDirectory.c_str());
    }
    break;
    default:
        break;
    }
}

void AzAssetBrowserRequestHandler::ResaveSlice(AZ::Entity* sliceEntity, const AZStd::string& fullFilePath)
{
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, fullFilePath.c_str(), true,
        [sliceEntity, fullFilePath](bool success, const AzToolsFramework::SourceControlFileInfo& info)
    {
        if (!info.IsReadOnly())
        {
            AZ::IO::FileIOStream fileStream(fullFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
            if (fileStream.IsOpen())
            {
                bool saved = AZ::Utils::SaveObjectToStream<AZ::Entity>(fileStream, AZ::DataStream::ST_XML, sliceEntity);
                if (saved)
                {
                    // Bump the slice asset up in the asset processor's queue.
                    EBUS_EVENT(AzFramework::AssetSystemRequestBus, GetAssetStatus, fullFilePath);
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

        delete sliceEntity;
    });
}