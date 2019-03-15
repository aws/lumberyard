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

#include "EditorComponentBase.h"
#include "EditorLayerComponentBus.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

class QCryptographicHash;
class QDir;

namespace AzToolsFramework
{
    namespace Components
    {
        class TransformComponent;
    }

    namespace Layers
    {
        /// Properties on this class will save to the layer file. Properties on the component
        /// will save to the level. Tend toward saving properties here to minimize how often
        /// users need to interact with the level file.
        class LayerProperties
        {
        public:
            AZ_CLASS_ALLOCATOR(LayerProperties, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(LayerProperties, "{FA61BD6E-769D-4856-BFB5-B535E0FC57B4}");

            static void Reflect(AZ::ReflectContext* context);

            void Clear()
            {
                m_saveAsBinary = false;
                m_color = AZ::Color::CreateOne();
                m_isLayerVisible = true;
            }

            // The color to display the layer in the outliner.
            AZ::Color m_color = AZ::Color::CreateOne();
            // Default to text files, so the save history is easier to understand in source control.
            // This attribute only effects writing layers, and is safe to store here instead of on the component.
            // When reading files off disk, Lumberyard figures out the correct format automatically.
            bool m_saveAsBinary = false;

            // The layer entity needs to be invisible to all other systems, so they don't show up in the viewport.
            // Visibility for layers can be toggled in the outliner, though. When layers are made invisible in the outliner,
            // it should provide an override to all children, making them invisible.
            bool m_isLayerVisible = true;
        };

        /// Helper class for saving and loading the contents of layers.
        class EditorLayer
        {
        public:
            AZ_CLASS_ALLOCATOR(EditorLayer, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(EditorLayer, "{82C661FE-617C-471D-98D5-289570137714}");

            static void Reflect(AZ::ReflectContext* context);

            EntityList m_layerEntities;
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs m_sliceAssetsToSliceInstances;
            LayerProperties m_layerProperties;
        };

        /// This editor component marks an entity as a layer entity.
        /// Layer entities allow the level to be split into multiple files,
        /// so content creators can work on the same level at the same time, without conflict.
        class EditorLayerComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorLayerComponentRequestBus::Handler
            , public EditorLayerInfoRequestsBus::Handler
            , public AZ::TransformNotificationBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorLayerComponent, "{976E05F0-FAC7-43B6-B621-66108AE73FD4}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            ~EditorLayerComponent();

            ////////////////////////////////////////////////////////////////////
            // EditorLayerComponentRequestBus::Handler
            LayerResult WriteLayerAndGetEntities(
                QString levelAbsoluteFolder,
                EntityList& entityList,
                AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances) override;

            void RestoreEditorData() override;

            bool HasLayer() override { return true; }

            void UpdateLayerNameConflictMapping(AZStd::unordered_map<AZStd::string, int>& nameConflictsMapping) override;

            QColor GetLayerColor() override;

            bool IsLayerNameValid() override;

            AZ::Outcome<AZStd::string, AZStd::string> GetLayerBaseFileName() override;
            AZ::Outcome<AZStd::string, AZStd::string> GetLayerFullFileName() override;

            void SetLayerChildrenVisibility(bool visible) override { m_editableLayerProperties.m_isLayerVisible = visible; }
            bool AreLayerChildrenVisible() override { return m_editableLayerProperties.m_isLayerVisible; }

            bool HasUnsavedChanges() override;
            void MarkLayerWithUnsavedChanges() override;

            bool DoesLayerFileExistOnDisk(const QString& levelAbsoluteFolder) override;

            bool GatherSaveDependencies(
                AZStd::unordered_set<AZ::EntityId>& allLayersToSave,
                bool& mustSaveLevel) override;

            void AddLayerSaveDependency(const AZ::EntityId& layerSaveDependency) override;

            void AddLevelSaveDependency() override;
            ////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////
            // EditorLayerInfoRequestsBus::Handler
            void GatherLayerEntitiesWithName(const AZStd::string& layerName, EntityIdSet& layerEntities) override;
            ////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AZ::TransformNotificationBus::Handler
            // Layers cannot have their parent changed.
            void CanParentChange(bool &parentCanChange, AZ::EntityId oldParent, AZ::EntityId newParent) override;
            //////////////////////////////////////////////////////////////////////////
            
            // This is not an ebus call because the layer entity is not yet fully setup at the point that entities in the layer are loaded.
            LayerResult ReadLayer(
                QString levelPakFile,
                AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities);

            // When reading in layers, GetLayerBaseFileName cannot be called because entities aren't yet responding to ebuses.
            // The system reading in the layers needs to know the file names to look for them, so used the last saved file name.
            AZStd::string GetCachedLayerBaseFileName() { return m_layerFileName; }

            // The Layer entities need to be added to the before they are cleaned up.
            void CleanupLoadedLayer();

            void SetLayerColor(AZ::Color newColor) { m_editableLayerProperties.m_color = newColor; }
        protected:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////
            LayerResult PrepareLayerForSaving(
                EditorLayer& layer,
                EntityList& entityList,
                AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances);
            LayerResult WriteLayerToStream(
                const EditorLayer& layer,
                AZ::IO::ByteContainerStream<AZStd::vector<char> >& entitySaveStream);
            LayerResult WriteLayerStreamToDisk(
                QString levelAbsoluteFolder,
                const AZ::IO::ByteContainerStream<AZStd::vector<char> >& entitySaveStream);
            LayerResult CreateDirectoryAtPath(const QString& path);

            LayerResult PopulateFromLoadedLayerData(
                const EditorLayer& loadedLayer,
                AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities);

            void AddUniqueEntitiesAndInstancesFromEditorLayer(
                const EditorLayer& loadedLayer,
                AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities);

            QString GetLayerDirectory() const { return "Layers"; }
            QString GetLayerTempDirectory() const { return "Layers_Temp"; }
            QString GetLayerExtension() const { return "layer"; }
            QString GetLayerTempExtension() const { return "layer_temp"; }

            // Try writing a temp file a few times, in case the initial temp file isn't writeable for some reason.
            int GetMaxTempFileWriteAttempts() const { return 5; }

            // Layer file names generated on each save.
            LayerResult GenerateLayerFileName();

            LayerResult GetFileHash(QString filePath, QCryptographicHash& hash);

            LayerResult GenerateCleanupFailureWarningResult(QString message, const LayerResult* currentFailure);
            LayerResult CleanupTempFileAndFolder(QString tempFile, const QDir& layerTempFolder, const LayerResult* currentFailure);
            LayerResult CleanupTempFolder(const QDir& layerTempFolder, const LayerResult* currentFailure);

            void GatherAllNonLayerNonSliceDescendants(
                EntityList& entityList,
                EditorLayer& layer,
                const AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& entityIdsToEntityPtrs,
                AzToolsFramework::Components::TransformComponent& transformComponent) const;

            void SetUnsavedChanges(bool unsavedChanges);

            EditorLayer* m_loadedLayer = nullptr;
            AZStd::string m_layerFileName;

            // Lumberyard's serialization system requires everything in the editor to have a serialized to disk counterpart.
            // Layers have their data split into two categories: Stuff that should save to the layer file, and stuff that should
            // save to the layer component in the level. To allow the layer component to edit the data that goes in the layer file,
            // a placeholder value is serialized. This is only used at edit time, and is copied and cleared during serialization.
            LayerProperties m_editableLayerProperties;
            LayerProperties m_cachedLayerProperties;

            bool m_hasUnsavedChanges = false;

            // Users can save individual layers. This can cause problems if an entity is moved between layers
            // and only one of those two layers is saved, it will duplicate the entity on the next load.
            // This tracks other layers that need to be saved when this layer is saved.
            AZStd::unordered_set<AZ::EntityId> m_otherLayersToSave;
            // When a new layer is created, mark it as needing to save the level the next time it saves. Existing layers
            // will set this flag to false when they load.
            bool m_mustSaveLevelWhenLayerSaves = true;
        };
    }
}