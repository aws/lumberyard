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
#include "AzToolsFramework/StdAfx.h"
#include "EditorLayerComponent.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/std/containers/stack.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

namespace AzToolsFramework
{
    namespace Layers
    {
        void LayerProperties::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<LayerProperties>()
                    ->Version(2)
                    ->Field("m_color", &LayerProperties::m_color)
                    ->Field("m_saveAsBinary", &LayerProperties::m_saveAsBinary)
                    ->Field("m_isLayerVisible", &LayerProperties::m_isLayerVisible)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    // m_isLayerVisible is purposely omitted from the editor here, it is toggled through the outliner.
                    editContext->Class<LayerProperties>("Layer Properties", "Layer properties that are saved to the layer file, and not the level.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &LayerProperties::m_color, "Color", "Color for the layer.")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &LayerProperties::m_saveAsBinary, "Save as binary", "Save the layer as an XML or binary file.");
                }
            }
        }

        void EditorLayer::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLayer>()
                    ->Version(2)
                    ->Field("layerEntities", &EditorLayer::m_layerEntities)
                    ->Field("sliceAssetsToSliceInstances", &EditorLayer::m_sliceAssetsToSliceInstances)
                    ->Field("m_layerProperties", &EditorLayer::m_layerProperties)
                    ;
            }
        }

        void EditorLayerComponent::Reflect(AZ::ReflectContext* context)
        {
            LayerProperties::Reflect(context);
            EditorLayer::Reflect(context);
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLayerComponent, EditorComponentBase>()
                    ->Version(2)
                    ->Field("m_layerFileName", &EditorLayerComponent::m_layerFileName)
                    ->Field("m_editableLayerProperties", &EditorLayerComponent::m_editableLayerProperties);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorLayerComponent>("Layer", "The layer component allows entities to be saved to different files on disk.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Layers.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Layers.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Layer", 0xe4db211a))
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, false)
                        ->Attribute(AZ::Edit::Attributes::RemoveableByUser, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorLayerComponent::m_editableLayerProperties, "Layer Properties", "Layer properties that are saved to the layer file, and not the level.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
                }
            }
        }

        void EditorLayerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorLayerService", 0x3e120934));
        }

        void EditorLayerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorLayerService", 0x3e120934));
        }

        void EditorLayerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            // Even though Layers hide their position and rotation, layers require access to the transform service
            // for hierarchy information.
            services.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        EditorLayerComponent::~EditorLayerComponent()
        {
            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            EditorLayerComponentRequestBus::Handler::BusDisconnect();
            EditorLayerInfoRequestsBus::Handler::BusDisconnect();

            if (m_loadedLayer != nullptr)
            {
                AZ_Error("Layers", "Layer %s did not unload itself correctly.", GetEntity()->GetName().c_str());
                CleanupLoadedLayer();
            }
        }

        void EditorLayerComponent::Init()
        {
            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            EditorLayerComponentRequestBus::Handler::BusConnect(GetEntityId());
            EditorLayerInfoRequestsBus::Handler::BusConnect();
        }

        void EditorLayerComponent::Activate()
        {
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

            // When an entity activates, it needs to know its visibility and lock state.
            // Unfortunately there is no guarantee on entity activation order, so if that entity
            // is in a layer and activates before the layer, then the entity can't retrieve the layer's
            // lock or visibility overrides. To work around this, the layer refreshes the visibility
            // and lock state on its children.

            bool isLocked = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                isLocked,
                GetEntityId(),
                &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);

            if (!m_editableLayerProperties.m_isLayerVisible ||
                isLocked)
            {
                AZStd::vector<AZ::EntityId> descendants;
                AZ::TransformBus::EventResult(
                    descendants,
                    GetEntityId(),
                    &AZ::TransformBus::Events::GetAllDescendants);

                for (AZ::EntityId descendant : descendants)
                {
                    // Don't need to worry about double checking the hierarchy for a layer overwriting another layer,
                    // layers can only turn visibility off or lock descendants, so there is no risk of conflict.

                    // Check if the descendant is a layer because layers aren't directly changed by these settings and can be skipped.
                    bool isLayerEntity = false;
                    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                        isLayerEntity,
                        descendant,
                        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                    if (isLayerEntity)
                    {
                        continue;
                    }

                    ComponentEntityEditorRequestBus::Event(descendant, &ComponentEntityEditorRequestBus::Events::RefreshVisibilityAndLock);
                }
            }
        }

        void EditorLayerComponent::Deactivate()
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
        }

        void EditorLayerComponent::UpdateLayerNameConflictMapping(AZStd::unordered_map<AZStd::string, int>& nameConflictsMapping)
        {
            AZ::Outcome<AZStd::string, AZStd::string> layerNameResult = GetLayerBaseFileName();
            if (layerNameResult.IsSuccess())
            {
                AZStd::string layerName = layerNameResult.GetValue();

                EntityIdSet layerEntities;
                AzToolsFramework::Layers::EditorLayerInfoRequestsBus::Broadcast(
                    &AzToolsFramework::Layers::EditorLayerInfoRequestsBus::Events::GatherLayerEntitiesWithName,
                    layerName,
                    layerEntities);

                if (layerEntities.size() > 1)
                {
                    nameConflictsMapping[layerName] = static_cast<int>(layerEntities.size());
                }
            }
        }

        QColor EditorLayerComponent::GetLayerColor()
        {
            // LY uses AZ classes for serialization, but the color to display needs to be a Qt color.
            // Ignore the alpha channel, it's not represented visually.
            return QColor::fromRgb(m_editableLayerProperties.m_color.GetR8(), m_editableLayerProperties.m_color.GetG8(), m_editableLayerProperties.m_color.GetB8());
        }

        bool EditorLayerComponent::IsLayerNameValid()
        {
            LayerResult fileNameGenerationResult = GenerateLayerFileName();
            // If the name can't be generated, it's definitely not a valid layer name.
            if (!fileNameGenerationResult.IsSuccess())
            {
                return false;
            }
            EntityIdSet layerEntities;
            EditorLayerInfoRequestsBus::Broadcast(
                &EditorLayerInfoRequestsBus::Events::GatherLayerEntitiesWithName,
                m_layerFileName,
                layerEntities);
            return layerEntities.size() == 1;
        }

        void EditorLayerComponent::GatherLayerEntitiesWithName(const AZStd::string& layerName, EntityIdSet& layerEntities)
        {
            LayerResult fileNameGenerationResult = GenerateLayerFileName();
            // Only gather if this layer's name is available.
            if (fileNameGenerationResult.IsSuccess())
            {
                if (azstricmp(m_layerFileName.c_str(), layerName.c_str()) == 0)
                {
                    layerEntities.insert(GetEntityId());
                }
            }
        }

        void EditorLayerComponent::MarkLayerWithUnsavedChanges()
        {
            if (m_hasUnsavedChanges)
            {
                return;
            }
            SetUnsavedChanges(true);
        }

        void EditorLayerComponent::SetUnsavedChanges(bool unsavedChanges)
        {
            m_hasUnsavedChanges = unsavedChanges;
            EditorEntityInfoNotificationBus::Broadcast(
                &EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedUnsavedChanges, GetEntityId());
        }

        void EditorLayerComponent::CanParentChange(bool &parentCanChange, AZ::EntityId /*oldParent*/, AZ::EntityId newParent)
        {
            if (newParent.IsValid())
            {
                bool isLayerEntity = false;
                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                    isLayerEntity,
                    newParent,
                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                if (!isLayerEntity)
                {
                    parentCanChange = false;
                }
            }
        }

        LayerResult EditorLayerComponent::WriteLayerAndGetEntities(
            QString levelAbsoluteFolder,
            EntityList& entityList,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            EditorLayer layer;
            LayerResult layerPrepareResult = PrepareLayerForSaving(layer, entityList, layerInstances);
            if (!layerPrepareResult.IsSuccess())
            {
                return layerPrepareResult;
            }

            AZStd::vector<char> entitySaveBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
            LayerResult streamCreationResult = WriteLayerToStream(layer, entitySaveStream);
            if (!streamCreationResult.IsSuccess())
            {
                return streamCreationResult;
            }
            return WriteLayerStreamToDisk(levelAbsoluteFolder, entitySaveStream);
        }

        void EditorLayerComponent::RestoreEditorData()
        {
            m_editableLayerProperties = m_cachedLayerProperties;
        }

        LayerResult EditorLayerComponent::ReadLayer(
            QString levelPakFile,
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            // If this layer is being loaded, it won't have a level save dependency yet, so clear that flag.
            m_mustSaveLevelWhenLayerSaves = false;
            QString fullPathName = levelPakFile;
            if (fullPathName.contains("@devassets@"))
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                // Resolving the path through resolvepath would normalize and lowcase it, and in this case, we don't want that.
                fullPathName.replace("@devassets@", fileIO->GetAlias("@devassets@"));
            }

            QFileInfo fileNameInfo(fullPathName);
            QDir layerFolder(fileNameInfo.absoluteDir());
            if (!layerFolder.cd(GetLayerDirectory()))
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to access layer folder %1 for layer %2.").arg(layerFolder.absoluteFilePath(GetLayerDirectory())).arg(m_layerFileName.c_str()));
            }

            QString myLayerFileName = QString("%1.%2").arg(m_layerFileName.c_str()).arg(GetLayerExtension());
            if (!layerFolder.exists(myLayerFileName))
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to access layer %1.").arg(layerFolder.absoluteFilePath(myLayerFileName)));
            }

            QString fullLayerPath(layerFolder.filePath(myLayerFileName));

            // Lumberyard will read in the layer in whatever format it's in, so there's no need to check what the save format is set to.
            // The save format is also set in this object that is being loaded, so it wouldn't even be available.
            m_loadedLayer = AZ::Utils::LoadObjectFromFile<EditorLayer>(fullLayerPath.toUtf8().data());

            if (!m_loadedLayer)
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to load from disk layer %1.").arg(layerFolder.absoluteFilePath(myLayerFileName)));
            }

            return PopulateFromLoadedLayerData(*m_loadedLayer, sliceInstances, uniqueEntities);
        }

        LayerResult EditorLayerComponent::PopulateFromLoadedLayerData(
            const EditorLayer& loadedLayer,
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities)
        {
            // The properties for the component are saved in the layer data instead of the component data
            // so users can change these without checking out the level file.
            m_cachedLayerProperties = loadedLayer.m_layerProperties;
            m_editableLayerProperties = m_cachedLayerProperties;

            AddUniqueEntitiesAndInstancesFromEditorLayer(loadedLayer, sliceInstances, uniqueEntities);

            return LayerResult::CreateSuccess();
        }

        void EditorLayerComponent::AddUniqueEntitiesAndInstancesFromEditorLayer(
            const EditorLayer& loadedLayer,
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities)
        {            
            // This tracks entities that are in this current layer, so duplicate entities in this layer can be cleaned up.
            AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::Entity*>::iterator> layerEntityIdsToIterators;

            for (AZStd::vector<AZ::Entity*>::iterator loadedLayerEntity = m_loadedLayer->m_layerEntities.begin();
                loadedLayerEntity != m_loadedLayer->m_layerEntities.end();)
            {
                AZ::EntityId currentEntityId = (*loadedLayerEntity)->GetId();
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>::iterator existingEntity =
                    uniqueEntities.find(currentEntityId);
                if (existingEntity != uniqueEntities.end())
                {
                    AZ_Warning(
                        "Layers",
                        "Multiple entities that exist in your scene with ID %s and name %s were found. Attempting to load the last entity found in layer %s.",
                        currentEntityId.ToString().c_str(),
                        (*loadedLayerEntity)->GetName().c_str(),
                        m_layerFileName.c_str());
                    AZ::Entity* duplicateEntity = existingEntity->second;
                    existingEntity->second = *loadedLayerEntity;
                    delete duplicateEntity;
                }
                else
                {
                    uniqueEntities[currentEntityId] = (*loadedLayerEntity);
                }

                AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::Entity*>::iterator>::iterator inLayerDuplicateSearch =
                    layerEntityIdsToIterators.find(currentEntityId);
                if (inLayerDuplicateSearch != layerEntityIdsToIterators.end())
                {
                    // Always keep the last found entity. If an entity cloning situation previously occurred,
                    // this is the entity most likely to have the latest changes from the content creator.
                    AZ_Warning(
                        "Layers",
                        "Multiple entities that exist in your scene with ID %s and name %s were found in layer %s. The last entity found has been loaded.",
                        currentEntityId.ToString().c_str(),
                        (*loadedLayerEntity)->GetName().c_str(),
                        m_layerFileName.c_str());
                    loadedLayerEntity = m_loadedLayer->m_layerEntities.erase(loadedLayerEntity);
                }
                else
                {
                    layerEntityIdsToIterators[currentEntityId] = loadedLayerEntity;
                    ++loadedLayerEntity;
                }
            }

            for (const auto& instanceIter :
                loadedLayer.m_sliceAssetsToSliceInstances)
            {
                sliceInstances[instanceIter.first].insert(
                    instanceIter.second.begin(),
                    instanceIter.second.end());
            }
        }

        void EditorLayerComponent::CleanupLoadedLayer()
        {
            delete m_loadedLayer;
            m_loadedLayer = nullptr;
        }

        LayerResult EditorLayerComponent::PrepareLayerForSaving(
            EditorLayer& layer,
            EntityList& entityList,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            // Move the editable data into the data serialized to the layer, and not the layer component.
            layer.m_layerProperties = m_editableLayerProperties;
            m_cachedLayerProperties = m_editableLayerProperties;
            // When serialization begins, clear out the editor data so it doesn't serialize to the level.
            m_editableLayerProperties.Clear();

            AzToolsFramework::Components::TransformComponent* transformComponent =
                GetEntity()->FindComponent<AzToolsFramework::Components::TransformComponent>();
            if (!transformComponent)
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to find the Transform component for layer %1 %2. This layer will not be exported.").
                    arg(GetEntity()->GetName().c_str()).
                    arg(GetEntity()->GetId().ToString().c_str()));
            }

            // GenerateLayerFileName populates m_layerFileName.
            LayerResult fileNameGenerationResult = GenerateLayerFileName();
            if (!fileNameGenerationResult.IsSuccess())
            {
                return fileNameGenerationResult;
            }

            // For MVP, don't save the layer if there is a name collision.
            // When this save occurs, the layer entities will be saved in the level so they won't be lost.
            if (!IsLayerNameValid())
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Layers with the name %1 can't be saved. Layer names at the same hierarchy level must be unique.").
                    arg(GetEntity()->GetName().c_str()));
            }

            // Entity pointers are necessary for saving entities to disk.
            // Grabbing the entities this way also guarantees that only entities that are safe to save to disk
            // will be included in the layer. Slice entities cannot yet be included in layers, only loose entities.
            EntityList editorEntities;
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::GetLooseEditorEntities,
                editorEntities);
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*> entityIdsToEntityPtrs;
            for (AZ::Entity* entity : editorEntities)
            {
                entityIdsToEntityPtrs[entity->GetId()] = entity;
            }

            // All descendants of the layer entity are included in the layer.
            // Note that the layer entity itself is not included in the layer.
            // This is intentional, it allows the level to track what layers exist and only
            // load the layers it expects to see.
            GatherAllNonLayerNonSliceDescendants(entityList, layer, entityIdsToEntityPtrs, *transformComponent);


            AZ::SliceComponent* rootSlice = nullptr;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(rootSlice, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorRootSlice);
            if (rootSlice)
            {
                AZ::SliceComponent::SliceList& slices = rootSlice->GetSlices();
                for (AZ::SliceComponent::SliceReference& sliceReference : slices)
                {
                    // Don't save the root slice instance in the layer.
                    if (sliceReference.GetSliceAsset().Get()->GetComponent() == rootSlice)
                    {
                        continue;
                    }
                    AZ::SliceComponent::SliceReference::SliceInstances& sliceInstances = sliceReference.GetInstances();
                    for (AZ::SliceComponent::SliceInstance& sliceInstance : sliceInstances)
                    {
                        const AZ::SliceComponent::InstantiatedContainer* instantiated = sliceInstance.GetInstantiated();
                        if (!instantiated || instantiated->m_entities.empty())
                        {
                            continue;
                        }
                        AZ::Entity* rootEntity = instantiated->m_entities[0];

                        AzToolsFramework::Components::TransformComponent* sliceTransform =
                            rootEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();

                        if (!sliceTransform)
                        {
                            continue;
                        }

                        for (AZ::TransformInterface* transformLayerSearch = sliceTransform;
                            transformLayerSearch != nullptr && transformLayerSearch->GetParent() != nullptr;
                            transformLayerSearch = transformLayerSearch->GetParent())
                        {
                            bool sliceAncestorParentIsLayer = false;
                            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                                sliceAncestorParentIsLayer,
                                transformLayerSearch->GetParentId(),
                                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

                            bool amISliceAncestorParent = transformLayerSearch->GetParentId() == GetEntityId();

                            if (sliceAncestorParentIsLayer && !amISliceAncestorParent)
                            {
                                // This slice instance will be handled by a different layer.
                                break;
                            }
                            else if (amISliceAncestorParent)
                            {
                                sliceReference.ComputeDataPatch(&sliceInstance);
                                layer.m_sliceAssetsToSliceInstances[sliceReference.GetSliceAsset()].insert(&sliceInstance);
                                layerInstances[&sliceReference].insert(&sliceInstance);
                                break;
                            }
                        }
                    }
                }
            }
            return LayerResult::CreateSuccess();
        }

        LayerResult EditorLayerComponent::WriteLayerToStream(
            const EditorLayer& layer,
            AZ::IO::ByteContainerStream<AZStd::vector<char> >& entitySaveStream)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            m_otherLayersToSave.clear();
            m_mustSaveLevelWhenLayerSaves = false;

            bool savedEntitiesResult = false;
            AZ::DataStream::StreamType saveStreamType = layer.m_layerProperties.m_saveAsBinary ? AZ::DataStream::ST_BINARY : AZ::DataStream::ST_XML;
            savedEntitiesResult = AZ::Utils::SaveObjectToStream<EditorLayer>(entitySaveStream, saveStreamType, &layer);

            AZStd::string layerBaseFileName(m_layerFileName);

            if (!savedEntitiesResult)
            {
                return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to serialize layer %1.").arg(layerBaseFileName.c_str()));
            }
            entitySaveStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            return LayerResult::CreateSuccess();
        }

        LayerResult EditorLayerComponent::WriteLayerStreamToDisk(
            QString levelAbsoluteFolder,
            const AZ::IO::ByteContainerStream<AZStd::vector<char> >& entitySaveStream)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            AZStd::string layerBaseFileName(m_layerFileName);

            // Write to a temp file first.
            QDir layerTempFolder(levelAbsoluteFolder);
            QString tempLayerDirectory = GetLayerTempDirectory();

            LayerResult layerErrorResult = CreateDirectoryAtPath(layerTempFolder.absoluteFilePath(tempLayerDirectory).toUtf8().data());
            if (!layerErrorResult.IsSuccess())
            {
                return layerErrorResult;
            }


            if (!layerTempFolder.cd(tempLayerDirectory))
            {
                return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to access directory %1.").arg(tempLayerDirectory));
            }

            QString tempLayerFullPath;
            for (int tempWriteTries = 0; tempWriteTries < GetMaxTempFileWriteAttempts(); ++tempWriteTries)
            {

                QString tempLayerFileName(QString("%1.%2.%3").
                    arg(layerBaseFileName.c_str()).
                    arg(GetLayerTempExtension()).
                    arg(tempWriteTries));
                tempLayerFullPath = layerTempFolder.absoluteFilePath(tempLayerFileName);

                AZ::IO::FileIOStream fileStream(tempLayerFullPath.toUtf8().data(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
                if (fileStream.IsOpen())
                {
                    if (!fileStream.Write(entitySaveStream.GetLength(), entitySaveStream.GetData()->data()))
                    {
                        layerErrorResult = LayerResult(
                            LayerResultStatus::Error,
                            QObject::tr("Unable to write layer %1 to the stream.").arg(layerBaseFileName.c_str()));
                    }
                    fileStream.Close();
                }
                if (layerErrorResult.IsSuccess())
                {
                    break;
                }
                else
                {
                    // Just record a warning for temp file writes that fail, don't both trying to add these to the final outcome.
                    AZ_Warning("Layers", false, "Unable to save temp file %1 for layer.", tempLayerFullPath.toUtf8().data());
                }
            }

            if (!layerErrorResult.IsSuccess())
            {
                return layerErrorResult;
            }

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

            // The real file

            QDir layerFolder(levelAbsoluteFolder);
            QString layerDirectory = GetLayerDirectory();
            layerErrorResult = CreateDirectoryAtPath(layerFolder.absoluteFilePath(layerDirectory).toUtf8().data());
            if (!layerErrorResult.IsSuccess())
            {
                return layerErrorResult;
            }

            if (!layerFolder.cd(layerDirectory))
            {
                return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to access directory %1.").arg(layerDirectory));
            }
            QString layerFileName(QString("%1.%2").arg(layerBaseFileName.c_str()).arg(GetLayerExtension()));
            QString layerFullPath(layerFolder.absoluteFilePath(layerFileName));

            if (layerFolder.exists(layerFileName))
            {
                QCryptographicHash layerHash(QCryptographicHash::Sha1);
                layerErrorResult = GetFileHash(layerFullPath, layerHash);
                if (!layerErrorResult.IsSuccess())
                {
                    return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &layerErrorResult);
                }

                // Generate a hash of what was actually written to disk, don't use the in-memory stream.
                // This makes it easy to account for things like newline differences between operating systems.
                QCryptographicHash tempLayerHash(QCryptographicHash::Sha1);
                layerErrorResult = GetFileHash(tempLayerFullPath, tempLayerHash);
                if (!layerErrorResult.IsSuccess())
                {
                    return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &layerErrorResult);
                }

                // This layer file hasn't actually changed, so return a success.
                if (layerHash.result() == tempLayerHash.result())
                {
                    SetUnsavedChanges(false);
                    return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, nullptr);
                }
            }

            // No need to specifically check for read-only state, if Perforce is not connected, it will
            // automatically overwrite the file.
            bool checkedOutSuccessfully = false;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                checkedOutSuccessfully,
                &ToolsApplicationRequestBus::Events::RequestEditForFileBlocking,
                layerFullPath.toUtf8().data(),
                "Checking out for edit...",
                ToolsApplicationRequestBus::Events::RequestEditProgressCallback());

            if (!checkedOutSuccessfully)
            {
                LayerResult failure(
                    LayerResultStatus::Error,
                    QObject::tr("Layer file %1 is unavailable to edit. Check your source control settings or if the file is locked by another tool.").arg(layerFullPath));
                return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &failure);
            }

            if (fileIO->Exists(layerFullPath.toUtf8().data()))
            {
                AZ::IO::Result removeResult = fileIO->Remove(layerFullPath.toUtf8().data());
                if (!removeResult)
                {
                    LayerResult failure(
                        LayerResultStatus::Error,
                        QObject::tr("Unable to write changes for layer %1. Check whether the file is locked by another tool.").arg(layerFullPath));
                    return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &failure);
                }
            }

            AZ::IO::Result renameResult = fileIO->Rename(tempLayerFullPath.toUtf8().data(), layerFullPath.toUtf8().data());

            if (!renameResult)
            {
                LayerResult failure(LayerResultStatus::Error, QObject::tr("Unable to save layer %1.").arg(layerFullPath));
                return CleanupTempFileAndFolder(tempLayerFullPath, layerTempFolder, &failure);
            }
            SetUnsavedChanges(false);
            return CleanupTempFolder(layerTempFolder, nullptr);
        }

        LayerResult EditorLayerComponent::GenerateLayerFileName()
        {
            // The layer name will be populated in the recursive function.
            m_layerFileName.clear();

            AZ::Outcome<AZStd::string, AZStd::string> layerFileNameResult = GetLayerBaseFileName();
            if (!layerFileNameResult.IsSuccess())
            {
                return LayerResult(
                    LayerResultStatus::Error,
                    QObject::tr("Unable to generate a file name for the layer. The content for the layer will save in the level instead of a layer file. %1").arg(layerFileNameResult.GetError().c_str()));
            }
            m_layerFileName = layerFileNameResult.GetValue();
            return LayerResult::CreateSuccess();
        }

        AZ::Outcome<AZStd::string, AZStd::string> EditorLayerComponent::GetLayerBaseFileName()
        {
            AZ::EntityId entityId(GetEntityId());
            AZ::Entity* myEntity(GetEntity());
            if (!entityId.IsValid() || !myEntity)
            {
                return AZ::Failure(AZStd::string("Entity is unavailable for this layer."));
            }
            AZ::EntityId nextAncestor;
            AZ::TransformBus::EventResult(
                /*result*/ nextAncestor,
                /*address*/ entityId,
                &AZ::TransformBus::Events::GetParentId);

            AZStd::string layerName = myEntity->GetName();
            if (nextAncestor.IsValid())
            {
                AZ::Outcome<AZStd::string, AZStd::string> ancestorLayerNameResult(
                    AZ::Failure(AZStd::string("A layer has a non-layer parent, which is unsupported. This layer will save to the level instead of a layer file.")));
                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                    ancestorLayerNameResult,
                    nextAncestor,
                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerBaseFileName);

                if (!ancestorLayerNameResult.IsSuccess())
                {
                    return ancestorLayerNameResult;
                }
                // Layers use their parents as a namespace to minimize collisions,
                // so a child layer's name is "ParentName.LayerName".
                layerName = ancestorLayerNameResult.GetValue() + "." + layerName;
            }
            return AZ::Success(layerName);
        }

        bool EditorLayerComponent::GatherSaveDependencies(
            AZStd::unordered_set<AZ::EntityId>& allLayersToSave,
            bool& mustSaveLevel)
        {
            bool dependenciesChanged = false;
            if (m_mustSaveLevelWhenLayerSaves)
            {
                mustSaveLevel = true;
                dependenciesChanged = true;
            }
            // There should only be a warning popup to save content if there are additional layers to save.
            for (const AZ::EntityId& dependency : m_otherLayersToSave)
            {
                if (allLayersToSave.find(dependency) == allLayersToSave.end())
                {
                    dependenciesChanged = true;
                    allLayersToSave.insert(dependency);
                }
            }
            return dependenciesChanged;
        }

        void EditorLayerComponent::AddLayerSaveDependency(const AZ::EntityId& layerSaveDependency)
        {
            m_otherLayersToSave.insert(layerSaveDependency);
        }

        void EditorLayerComponent::AddLevelSaveDependency()
        {
            m_mustSaveLevelWhenLayerSaves = true;
        }

        AZ::Outcome<AZStd::string, AZStd::string> EditorLayerComponent::GetLayerFullFileName()
        {
            AZ::Outcome<AZStd::string, AZStd::string> layerBaseNameResult = GetLayerBaseFileName();

            if (!layerBaseNameResult.IsSuccess())
            {
                return layerBaseNameResult;
            }

            AZStd::string layerFullFileName = layerBaseNameResult.GetValue() + +"." + GetLayerExtension().toStdString().c_str();
            return AZ::Success(layerFullFileName);
        }

        bool EditorLayerComponent::HasUnsavedChanges()
        {
            return m_hasUnsavedChanges;
        }

        bool EditorLayerComponent::DoesLayerFileExistOnDisk(const QString& levelAbsoluteFolder)
        {
            AZ::Outcome<AZStd::string, AZStd::string> layerFullNameResult = GetLayerFullFileName();
            if (!layerFullNameResult.IsSuccess())
            {
                return false;
            }

            AZStd::string layerFullFileName = layerFullNameResult.GetValue();
            QDir layerFolder(levelAbsoluteFolder);
            QString layerDirectory = GetLayerDirectory();

            return !layerFolder.cd(layerDirectory) || !layerFolder.exists(layerFullFileName.c_str());
        }

        LayerResult EditorLayerComponent::GetFileHash(QString filePath, QCryptographicHash& hash)
        {
            QFile fileForHash(filePath);
            if (fileForHash.open(QIODevice::ReadOnly))
            {
                hash.addData(fileForHash.readAll());
                fileForHash.close();
                return LayerResult::CreateSuccess();
            }
            return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to compute hash for layer %1. This layer will not be saved.").arg(filePath));
        }

        LayerResult EditorLayerComponent::CreateDirectoryAtPath(const QString& path)
        {
            const QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
            QFileInfo fileInfo(cleanPath);
            if (fileInfo.isDir())
            {
                return LayerResult::CreateSuccess();
            }
            else if (!fileInfo.exists())
            {
                if (QDir().mkpath(cleanPath))
                {
                    return LayerResult::CreateSuccess();
                }
            }
            return LayerResult(LayerResultStatus::Error, QObject::tr("Unable to create directory at %1.").arg(path));
        }

        LayerResult EditorLayerComponent::GenerateCleanupFailureWarningResult(QString message, const LayerResult* currentFailure)
        {
            LayerResult result(LayerResultStatus::Warning, message);
            if (currentFailure)
            {
                // CleanupTempFileAndFolder can only fail with a warning, so if a failure was provided, use that failure's level.
                if (currentFailure->m_result != LayerResultStatus::Success)
                {
                    result.m_result = currentFailure->m_result;
                }
                if (!currentFailure->m_message.isEmpty())
                {
                    // If a message was provided, then treat that as the primary warning, and append the cleanup warning.
                    result.m_message = (QObject::tr("%1, with additional warning: %2").
                        arg(currentFailure->m_message).arg(result.m_message));
                }
            }
            return result;
        }

        LayerResult EditorLayerComponent::CleanupTempFileAndFolder(
            QString tempFile,
            const QDir& layerTempFolder,
            const LayerResult* currentFailure)
        {
            LayerResult cleanupFailureResult(GenerateCleanupFailureWarningResult(
                QObject::tr("Temp layer file %1 was not removed.").arg(tempFile),
                currentFailure));

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (!fileIO)
            {
                return cleanupFailureResult;
            }
            AZ::IO::Result removeResult = fileIO->Remove(tempFile.toUtf8().data());
            if (!removeResult)
            {
                return cleanupFailureResult;
            }

            return CleanupTempFolder(layerTempFolder, &cleanupFailureResult);
        }

        LayerResult EditorLayerComponent::CleanupTempFolder(const QDir& layerTempFolder, const LayerResult* currentFailure)
        {
            LayerResult cleanupFailureResult(GenerateCleanupFailureWarningResult(
                QObject::tr("Temp layer folder %1 was not removed.").arg(layerTempFolder.absolutePath()),
                currentFailure));

            // FileIO doesn't support removing directory, so use Qt.
            // QDir::IsEmpty isn't available until a newer version fo Qt than Lumberyard is using.
            if (layerTempFolder.entryInfoList(
                QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System | QDir::Hidden).count() == 0)
            {
                if (layerTempFolder.rmdir("."))
                {
                    if (currentFailure)
                    {
                        return cleanupFailureResult;
                    }
                    else
                    {
                        return LayerResult::CreateSuccess();
                    }
                }
                return cleanupFailureResult;
            }
            // It's not an error if the folder wasn't empty.
            if (currentFailure)
            {
                return cleanupFailureResult;
            }
            else
            {
                return LayerResult::CreateSuccess();
            }
        }

        void EditorLayerComponent::GatherAllNonLayerNonSliceDescendants(
            EntityList& entityList,
            EditorLayer& layer,
            const AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& entityIdsToEntityPtrs,
            AzToolsFramework::Components::TransformComponent& transformComponent) const
        {
            AZStd::vector<AZ::EntityId> children = transformComponent.GetChildren();
            for (AZ::EntityId child : children)
            {
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>::const_iterator childEntityIdToPtr =
                    entityIdsToEntityPtrs.find(child);
                if (childEntityIdToPtr == entityIdsToEntityPtrs.end())
                {
                    // We only want loose entities, ignore slice entities, they are tracked differently.
                    continue;
                }

                bool isChildLayerEntity = false;
                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                    isChildLayerEntity,
                    child,
                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                if (isChildLayerEntity)
                {
                    // Don't add child layers or their descendants.
                    // All layers are currently tracked by the scene slice,
                    // and the descendants will be tracked by that child layer.
                    continue;
                }

                entityList.push_back(childEntityIdToPtr->second);
                layer.m_layerEntities.push_back(childEntityIdToPtr->second);

                AzToolsFramework::Components::TransformComponent* childTransform =
                    childEntityIdToPtr->second->FindComponent<AzToolsFramework::Components::TransformComponent>();
                if (childTransform)
                {
                    GatherAllNonLayerNonSliceDescendants(entityList, layer, entityIdsToEntityPtrs, *childTransform);
                }
            }
        }
    }
}