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
#ifndef AZTOOLSFRAMEWORK_EDITORENTITYCONTEXTCOMPONENT_H
#define AZTOOLSFRAMEWORK_EDITORENTITYCONTEXTCOMPONENT_H

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/Entity/EditorEntityContextPickingBus.h>

#include "EditorEntityContextBus.h"

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    class EntityContext;
}

namespace AzToolsFramework
{
    /**
     * System component responsible for owning the edit-time entity context.
     *
     * The editor entity context owns entities in the world, within the editor.
     * These entities typically own components inheriting from EditorComponentBase.
     */
    class EditorEntityContextComponent
        : public AZ::Component
        , public AzFramework::EntityContext
        , private EditorEntityContextRequestBus::Handler
        , private EditorEntityContextPickingRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private AzFramework::SliceInstantiationResultBus::MultiHandler
    {
    public:

        AZ_COMPONENT(EditorEntityContextComponent, "{92E3029B-6F4E-4628-8306-45D51EE59B8C}");

        EditorEntityContextComponent();
        ~EditorEntityContextComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // Component overrides
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorEntityContextRequestBus
        AzFramework::EntityContextId GetEditorEntityContextId() override { return GetContextId(); }
        AZ::SliceComponent* GetEditorRootSlice() override { return GetRootSlice(); }
        void ResetEditorContext() override;

        AZ::Entity* CreateEditorEntity(const char* name) override;
        AZ::Entity* CreateEditorEntityWithId(const char* name, const AZ::EntityId& entityId) override;
        void AddEditorEntity(AZ::Entity* entity) override;
        void AddEditorEntities(const EntityList& entities) override;
        void AddEditorSliceEntities(const EntityList& entities) override;
        bool CloneEditorEntities(const EntityIdList& sourceEntities, 
                                 EntityList& resultEntities,
                                 AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) override;
        bool DestroyEditorEntity(AZ::EntityId entityId) override;
        void DetachSliceEntities(const EntityIdList& entities) override;
        void DetachSliceInstances(const AZ::SliceComponent::SliceInstanceAddressSet& instances) override;
        void DetachFromSlice(const EntityIdList& entities, const char* undoMessage);
        void ResetEntitiesToSliceDefaults(EntityIdList entities) override;

        AZ::SliceComponent::SliceInstanceAddress CloneSubSliceInstance(
            const AZ::SliceComponent::SliceInstanceAddress& sourceSliceInstanceAddress,
            const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
            const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
            AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap) override;

        AzFramework::SliceInstantiationTicket InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, const AZ::Transform& worldTransform) override;
        AZ::SliceComponent::SliceInstanceAddress CloneEditorSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance, 
                                                                          AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) override;

        bool SaveToStreamForEditor(
            AZ::IO::GenericStream& stream,
            const EntityList& entitiesInLayers,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& instancesInLayers) override;
        void GetLooseEditorEntities(EntityList& entityList) override;
        
        bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) override;
        using EntityContext::LoadFromStream;
        bool LoadFromStream(AZ::IO::GenericStream& stream) override;
        bool LoadFromStreamWithLayers(AZ::IO::GenericStream& stream, QString levelPakFile) override;

        void StartPlayInEditor() override;
        void StopPlayInEditor() override;

        bool IsEditorRunningGame() override;
        bool IsEditorEntity(AZ::EntityId id) override;
        
        void AddRequiredComponents(AZ::Entity& entity) override;
        const AZ::ComponentTypeList& GetRequiredComponentTypes() override;

        void RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info, SliceEntityRestoreType restoreType) override;

        void QueueSliceReplacement(const char* targetPath, 
            const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
            const AZStd::unordered_set<AZ::EntityId>& entitiesInSelection,
            const AZ::EntityId& parentAfterReplacement,
            const AZ::Vector3& offsetAfterReplacement,
            const AZ::Quaternion& rotationAfterReplacement,
            bool rootAutoCreated) override;

        bool MapEditorIdToRuntimeId(const AZ::EntityId& editorId, AZ::EntityId& runtimeId) override;
        bool MapRuntimeIdToEditorId(const AZ::EntityId& runtimeId, AZ::EntityId& editorId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorEntityContextPickingRequestBus
        bool SupportsViewportEntityIdPicking() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::SliceInstantiationResultBus
        void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityContext
        void PrepareForContextReset() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogEventBus::Handler
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EditorEntityContextService", 0x28d93a43));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EditorEntityContextService", 0x28d93a43));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        }

    protected:
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        void OnContextEntitiesAdded(const EntityList& entities) override;
        void OnContextEntityRemoved(const AZ::EntityId& id) override;

        void HandleNewMetadataEntitiesCreated(AZ::SliceComponent& slice) override;

        // Helper function for creating editor ready entities.
        void FinalizeEditorEntity(AZ::Entity* entity);

        void SetupEditorEntity(AZ::Entity* entity);
        void SetupEditorEntities(const EntityList& entities);

        void LoadFromStreamComplete(bool loadedSuccessfully);

        using InstantiatingSlicePair = AZStd::pair<AZ::Data::Asset<AZ::Data::AssetData>, AZ::Transform>;
        AZStd::vector<InstantiatingSlicePair> m_instantiatingSlices;

    private:
        EditorEntityContextComponent(const EditorEntityContextComponent&) = delete;
        //! Indicates whether or not the editor is simulating the game.
        bool m_isRunningGame;

        //! List of selected entities prior to entering game.
        EntityIdList m_selectedBeforeStartingGame;

        //! Bidirectional mapping of runtime entity Ids to their editor counterparts (relevant during in-editor simulation).
        AZ::SliceComponent::EntityIdToEntityIdMap m_editorToRuntimeIdMap;
        AZ::SliceComponent::EntityIdToEntityIdMap m_runtimeToEditorIdMap;

        /**
         * Tracks a queued slice replacement, which is a deferred operation.
         * If the asset has not yet been processed (a new asset), we need
         * to defer before attempting a load.
         */
        struct QueuedSliceReplacement
        {
            ~QueuedSliceReplacement() = default;
            QueuedSliceReplacement() = default;

        private:
            //! Workaround for VS2013 is_copy_constructible returning true for deleted copy constructors
            //! https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
            QueuedSliceReplacement(const QueuedSliceReplacement&) = delete;
            QueuedSliceReplacement& operator=(const QueuedSliceReplacement&) = delete;

        public:
            void Setup(const char* path,
                const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& selectedToAssetMap,
                const AZStd::unordered_set<AZ::EntityId>& entitiesInSelection,
                const AZ::EntityId& parentAfterReplacement,
                const AZ::Vector3& offsetAfterReplacement,
                const AZ::Quaternion& rotationAfterReplacement,
                bool rootAutoCreated)
            {
                m_path = path;
                m_selectedToAssetMap = selectedToAssetMap;
                m_entitiesInSelection.clear();
                m_entitiesInSelection.insert(entitiesInSelection.begin(), entitiesInSelection.end());
                m_parentAfterReplacement = parentAfterReplacement;
                m_offsetAfterReplacement = offsetAfterReplacement;
                m_rotationAfterReplacement = rotationAfterReplacement,
                m_rootAutoCreated = rootAutoCreated;
            }

            bool IsValid() const;
            void Reset();

            bool OnCatalogAssetAdded(const AZ::Data::AssetId& assetId);

            void OnSlicePreInstantiate();

            void Finalize(const AZ::SliceComponent::SliceInstanceAddress& instanceAddress);

            AZStd::string                                       m_path;
            AZStd::unordered_map<AZ::EntityId, AZ::EntityId>    m_selectedToAssetMap;
            AZStd::unordered_set<AZ::EntityId>                  m_entitiesInSelection;
            AZ::EntityId                                        m_parentAfterReplacement;
            AZ::Vector3                                         m_offsetAfterReplacement;
            AZ::Quaternion                                      m_rotationAfterReplacement;
            bool                                                m_rootAutoCreated;
            AzFramework::SliceInstantiationTicket               m_ticket;
        };

        QueuedSliceReplacement m_queuedSliceReplacement;

        /**
         * Slice entity restore requests, which can be deferred if asset wasn't loaded at request time.
         */
        struct SliceEntityRestoreRequest
        {
            AZ::Entity* m_entity;
            AZ::SliceComponent::EntityRestoreInfo m_restoreInfo;
            AZ::Data::Asset<AZ::Data::AssetData> m_asset;
            SliceEntityRestoreType m_restoreType;
        };

        AZStd::vector<SliceEntityRestoreRequest> m_queuedSliceEntityRestores;

        // array of types of required components added to all Editor entities with EditorEntityContextRequestBus::Events::AddRequiredComponents()
        AZ::ComponentTypeList m_requiredEditorComponentTypes;
    };
} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_EDITORENTITYCONTEXTCOMPONENT_H
