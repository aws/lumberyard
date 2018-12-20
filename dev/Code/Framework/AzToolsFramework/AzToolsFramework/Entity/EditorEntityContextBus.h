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
#ifndef AZTOOLSFRAMEWORK_EDITORENTITYCONTEXTBUS_H
#define AZTOOLSFRAMEWORK_EDITORENTITYCONTEXTBUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AZ
{
    class Entity;
}

namespace AzToolsFramework
{
    /**
     * Bus for making requests to the edit-time entity context component.
     */
    class EditorEntityContextRequests
        : public AZ::EBusTraits
    {
    public:

        virtual ~EditorEntityContextRequests() {}

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /// Retrieve the Id of the editor entity context.
        virtual AzFramework::EntityContextId GetEditorEntityContextId() = 0;

        /// Retrieves the root slice for the editor entity context.
        virtual AZ::SliceComponent* GetEditorRootSlice() = 0;

        /// Creates an entity in the editor context.
        /// \return a new entity
        virtual AZ::Entity* CreateEditorEntity(const char* name) = 0;

        /// Registers an existing entity with the editor context.
        virtual void AddEditorEntity(AZ::Entity* entity) = 0;

        /// Registers an existing set of entities with the editor context.
        virtual void AddEditorEntities(const AzToolsFramework::EntityList& entities) = 0;

        /// Registers an existing set of entities of a slice instance with the editor context.
        virtual void AddEditorSliceEntities(const AzToolsFramework::EntityList& entities) = 0;

        /// Destroys an entity in the editor context.
        /// \return whether or not the entity was destroyed. A false return value signifies the entity did not belong to the game context.
        virtual bool DestroyEditorEntity(AZ::EntityId entityId) = 0;

        /// Detaches entities from their current slice instance and adds them to root slice as loose entities.
        virtual void DetachSliceEntities(const AzToolsFramework::EntityIdList& entities) = 0;

        /// Resets any slice data overrides for the specified entity
        virtual void ResetEntitiesToSliceDefaults(AzToolsFramework::EntityIdList entities) = 0;

        /**
         * Clone an slice-instance that comes from a sub-slice, and add the clone to the root slice.
         * @param sourceSliceInstanceAddress The address of the slice instance that contains the sub-slice instance.
         * @param sourceSubSliceInstanceAncestry The ancestry in order from sourceSubSlice to sourceSlice
         * @param sourceSubSliceInstanceAddress The address of the sub-slice instance to be cloned.
         * @param out_sourceToCloneEntityIdMap If valid address provided, the internal source to clone entity ID map will be returned 
         */
        virtual AZ::SliceComponent::SliceInstanceAddress CloneSubSliceInstance(const AZ::SliceComponent::SliceInstanceAddress& sourceSliceInstanceAddress,
                                        const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
                                        const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
                                        AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap) = 0;

        /// Clones a set of entities and optionally creates the Sandbox objects to wrap them.
        /// This function doesn't automatically add new entities to any entity context, callers are responsible for that.
        /// \param sourceEntities - the source set of entities to clone
        /// \param resultEntities - the set of entities cloned from the source
        /// \param sourceToCloneEntityIdMap[out] The map between source entity ids and clone entity ids
        /// \return true means cloning succeeded, false otherwise
        virtual bool CloneEditorEntities(const AzToolsFramework::EntityIdList& sourceEntities, 
                                         AzToolsFramework::EntityList& resultEntities, 
                                         AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) = 0;

        /// Clones an existing slice instance in the editor context. New instance is immediately returned.
        /// This function doesn't automatically add the new slice instance to any entity context, callers are responsible for that.
        /// \param sourceInstance The source instance to be cloned
        /// \param sourceToCloneEntityIdMap[out] The map between source entity ids and clone entity ids
        /// \return address of new slice instance. A null address will be returned if the source instance address is invalid.
        virtual AZ::SliceComponent::SliceInstanceAddress CloneEditorSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance, 
                                                                                  AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) = 0;

        /// Instantiates a editor slice.
        virtual AzFramework::SliceInstantiationTicket InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, const AZ::Transform& worldTransform) = 0;

        /// Saves the context's slice root to the specified buffer. Entities are saved as-is (with editor components).
        /// \return true if successfully saved. Failure is only possible if serialization data is corrupt.
        virtual bool SaveToStreamForEditor(AZ::IO::GenericStream& stream) = 0;

        /// Saves the context's slice root to the specified buffer. Entities undergo conversion for game: editor -> game components.
        /// \return true if successfully saved. Failure is only possible if serialization data is corrupt.
        virtual bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) = 0;

        /// Loads the context's slice root from the specified buffer.
        /// \return true if successfully loaded. Failure is possible if the source file is corrupt or data could not be up-converted.
        virtual bool LoadFromStream(AZ::IO::GenericStream& stream) = 0;

        /// Completely resets the context (slices and entities deleted).
        virtual void ResetEditorContext() = 0;

        /// Play-in-editor transitions.
        /// When starting, we deactivate the editor context and startup the game context.
        virtual void StartPlayInEditor() = 0;

        /// When stopping, we shut down the game context and re-activate the editor context.
        virtual void StopPlayInEditor() = 0;

        /// Is used to check whether the Editor is running the game simulation or in normal edit mode.
        virtual bool IsEditorRunningGame() = 0;

        /// \return true if the entity is owned by the editor entity context.
        virtual bool IsEditorEntity(AZ::EntityId id) = 0;

        /// Restores an entity back to a slice instance for undo/redo *only*. A valid \ref EntityRestoreInfo must be provided,
        /// and is only extracted directly via \ref SliceReference::GetEntityRestoreInfo().
        virtual void RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info) = 0;

        /// Adds the required editor components to the entity.
        virtual void AddRequiredComponents(AZ::Entity& entity) = 0;

        /// Returns an array of the required editor component types added by AddRequiredComponents()
        virtual const AZ::ComponentTypeList& GetRequiredComponentTypes() = 0;

        /// Editor functionality to replace a set of entities with a new instance of a new slice asset.
        /// This is a deferred operation since the asset may not yet have been processed (i.e. new asset).
        /// Once the asset has been created, it will be loaded and instantiated.
        /// \param targetPath path to the slice asset to be instanced in-place over the specified entities.
        /// \param selectedToAssetMap relates selected (live) entity Ids to Ids in the slice asset for post-replace Id reference patching.
        /// \param entitiesToReplace contains entity Ids to be replaced.
        /// \param parentAfterReplacement Entity that the slice should be a child of upon replacement (If this is invalid, then the slice is free standing)
        /// \param offsetAfterReplacement offset from the parentAfterReplacement that the slice root must be moved to post replacement
        /// \param rotationAfterReplacement rotation from the parentAfterReplacement that the slice root must be rotated to post replacement
        /// \param rootAutoCreated true if the root was auto-created for the user
        virtual void QueueSliceReplacement(const char* targetPath, 
            const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
            const AZStd::unordered_set<AZ::EntityId>& entitiesToReplace,
            const AZ::EntityId& parentAfterReplacement,
            const AZ::Vector3& offsetAfterReplacement,
            const AZ::Quaternion& rotationAfterReplacement,
            bool rootAutoCreated) = 0;

        /// Maps an editor Id to a runtime entity Id. Relevant only during in-editor simulation.
        /// \param Id of editor entity
        /// \param destination parameter for runtime entity Id
        /// \return true if editor Id was found in the editor Id map
        virtual bool MapEditorIdToRuntimeId(const AZ::EntityId& editorId, AZ::EntityId& runtimeId) = 0;

        /// Maps an runtime Id to a editor entity Id. Relevant only during in-editor simulation.
        /// \param Id of runtime entity
        /// \param destination parameter for editor entity Id
        /// \return true if runtime Id was found in the Id map
        virtual bool MapRuntimeIdToEditorId(const AZ::EntityId& runtimeId, AZ::EntityId& editorId) = 0;
    };

    using EditorEntityContextRequestBus = AZ::EBus<EditorEntityContextRequests>;

    /**
     * Bus for receiving events/notifications from the editor entity context component.
     */
    class EditorEntityContextNotification
        : public AZ::EBusTraits
    {
    public:

        virtual ~EditorEntityContextNotification() {};

        /// Called before the context is reset.
        virtual void PrepareForContextReset() {}

        /// Fired when the context is being reset.
        virtual void OnContextReset() {}

        /// Fired when a slice has been successfully instantiated.
        virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/, const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

        /// Fired when a slice has failed to instantiate.
        virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/, const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

        // When a slice is created , the editor entities are replaced by an instance of the slice, this notification
        // sends a map from the old Entity id's to the new Entity id's
        virtual void OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& /*replacedEntitiesMap*/) {}

        /// Fired when an group of entities has slice ownership change.
        /// This should only be fired if all of the entities now belong to the same slice or all now belong to no slice
        virtual void OnEditorEntitiesSliceOwnershipChanged(const AzToolsFramework::EntityIdList& /*entityIdList*/) {}

        //! Fired when the editor begins going into 'Simulation' mode.
        virtual void OnStartPlayInEditorBegin() {}

        //! Fired when the editor finishes going into 'Simulation' mode.
        virtual void OnStartPlayInEditor() {}

        //! Fired when the editor comes out of 'Simulation' mode
        virtual void OnStopPlayInEditor() {}

        //! Fired when the entity stream begins loading.
        virtual void OnEntityStreamLoadBegin() {}

        //! Fired when the entity stream has been successfully loaded.
        virtual void OnEntityStreamLoadSuccess() {}

        //! Fired when the entity stream load has failed
        virtual void OnEntityStreamLoadFailed() {}

        //! Fired when the entities needs to be focused in Entity Outliner
        virtual void OnFocusInEntityOutliner(const AzToolsFramework::EntityIdList& /*entityIdList*/) {}
    };

    using EditorEntityContextNotificationBus = AZ::EBus<EditorEntityContextNotification>;
} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_EDITORENTITYCONTEXTBUS_H
