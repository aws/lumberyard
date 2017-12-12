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

#include <AzCore/base.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#pragma once

namespace AzToolsFramework
{
    class InstanceDataNode;

    namespace SliceUtilities
    {
        /**
         * Displays slice push UI for the specified set of entities.
         * \param entities - the list of entity Ids to include in the push request.
         * \param serializeContext - desired serialize context. Specify nullptr to use application's serialize context.
         */
        void PushEntitiesModal(const EntityIdList& entities, 
                               AZ::SerializeContext* serializeContext = nullptr);
        
        /**
         * Creates a new slice asset containing the specified entities.
         * If the asset already exists, entities will instead be pushed to the asset to preserve relationships.
         * \param entities - the list of entities to include in the new slice.
         * \param targetDirectory - the preferred directory path.
         * \param inheritSlices - if true, entities already part of slice instances will be added by cascading from their corresponding slices.
         * \return true if slice was created successfully.
         */
        bool MakeNewSlice(const AzToolsFramework::EntityIdList& entities, 
                          const char* targetDirectory, 
                          bool inheritSlices, 
                          AZ::SerializeContext* serializeContext = nullptr);

        /**
         * Utility function to gather all referenced entities given a set of entities.
         * This will expand parent/child relationships and any entity Id reference.
         * \param entitiesWithREferences - input/output set containing all referenced entities.
         * \param serializeContext - serialize context to use to traverse reflected data.
         */
        void GatherAllReferencedEntities(AZStd::unordered_set<AZ::EntityId>& entitiesWithReferences,
                                         AZ::SerializeContext& serializeContext);


        /**
         * Clones a slice-owned entity for the purpose of comparison against a live entity.
         * The key aspect of this utility is the resulting clone has Ids remapped to that of the instance,
         * so entity references don't appear as changes/deltas due to Id remapping during slice instantiation.
         */
        AZStd::unique_ptr<AZ::Entity> CloneSliceEntityForComparison(const AZ::Entity& sourceEntity,
                                                                    const AZ::SliceComponent::SliceInstance& instance,
                                                                    AZ::SerializeContext& serializeContext);

        /**
         * Determines if adding instanceToAdd slice to targetInstanceToAddTo slice is safe to do with regard
         * to whether it would create a cyclic asset dependency. Slices cannot contain themselves.
         * \param instanceToAdd - the slice instance wanting to be added
         * \param targetInstanceToAddTo - the slice instance wanting to have instanceToAdd added to
         * \return true if safe to add. false if the slice addition would create a cyclic asset dependency (invalid).
         */
        bool CheckSliceAdditionCyclicDependencySafe(const AZ::SliceComponent::SliceInstanceAddress& instanceToAdd,
                                                    const AZ::SliceComponent::SliceInstanceAddress& targetInstanceToAddTo);


        /**
         * Push a set of entities to a given slice asset.
         * It is assumed that all provided entities belong to an instance of the provided slice, otherwise AZ::Failure will be returned.
         * \param entityIdList list of live entity Ids whose overrides will be pushed to the slice. Live entities must belong to instances of the specified slice.
         * \param sliceAsset the target slice asset.
         * \return AZ::Success if push is completed successfully, otherwise AZ::Failure with an AZStd::string payload.
         */
        AZ::Outcome<void, AZStd::string> PushEntitiesBackToSlice(const AzToolsFramework::EntityIdList& entityIdList, const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset);

        /**
         * Returns true if the entity has no transform parent.
         */
        bool IsRootEntity(const AZ::Entity& entity);

        /**
        * Retrieves the \ref AZ::Edit::Attributes::SliceFlags assigned to a given data node.
        * \param editData - The specific element data to check for slice flags (can be nullptr)
        * \param classData - The class data to check for slice flags (some flags can cascade from class to all elements, can also be nullptr)
        * return ref AZ::Edit::SliceFlags
        */
        AZ::u32 GetSliceFlags(const AZ::Edit::ElementData* editData, const AZ::Edit::ClassData* classData);

        /**
        * Retrieves the \ref AZ::Edit::Attributes::SliceFlags assigned to a given data node.
        * \param node - instance data hierarchy node
        * return ref AZ::Edit::SliceFlags
        */
        AZ::u32 GetNodeSliceFlags(const InstanceDataNode& node);

        /**
         * Returns true if the specified node is slice-pushable.
         * \param node instance data node to evaluate.
         * \param isRootEntity (optional) specifies whether the parent entity is a transform root.
         */
        bool IsNodePushable(const InstanceDataNode& node, bool isRootEntity = false);


        bool GenerateSuggestedSlicePath(const AzToolsFramework::EntityIdSet& entitiesInSlice, const AZStd::string& targetDirectory, AZStd::string& suggestedFullPath);

        void SetSliceSaveLocation(const AZStd::string& path);
        bool GetSliceSaveLocation(AZStd::string& path);

        void Reflect(AZ::ReflectContext* context);

        class SliceUserSettings
            : public AZ::UserSettings
        {
        public:
            AZ_CLASS_ALLOCATOR(SliceUserSettings, AZ::SystemAllocator, 0);
            AZ_RTTI(SliceUserSettings, "{56EC1A8F-1ADB-4CC7-A3C3-3F462750C31F}", AZ::UserSettings);

            AZStd::string m_saveLocation;

            SliceUserSettings() = default;

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace SliceUtilities

} // AzToolsFramework
