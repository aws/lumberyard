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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include "EditorCommon.h"
#include "UiEditorEntityContextBus.h"

class UiSliceManager
    : public UiEditorEntityContextNotificationBus::Handler
{
public:     // member functions

    UiSliceManager(AzFramework::EntityContextId entityContextId);
    ~UiSliceManager() override;

    //////////////////////////////////////////////////////////////////////////
    // UiEditorEntityContextNotificationBus implementation
    void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& ticket) override;
    //~UiEditorEntityContextNotificationBus implementation

    //! Instantiate an existing slice asset into the UI canvas
    void InstantiateSlice(const AZ::Data::AssetId& assetId, AZ::Vector2 viewportPosition);

    //! Instantiate an existing slice asset into the UI canvas using a file browser
    void InstantiateSliceUsingBrowser(HierarchyWidget* hierarchy, AZ::Vector2 viewportPosition);

    //! Create a new slice from the selected items and replace the selected items
    //! with an instance of the slice
    void MakeSliceFromSelectedItems(HierarchyWidget* hierarchy, bool inheritSlices);

    // Get the root slice for the canvas
    AZ::SliceComponent* GetRootSlice() const;

    AzToolsFramework::EntityIdSet UiSliceManager::GatherEntitiesAndAllDescendents(const AzToolsFramework::EntityIdList& inputEntities);
    
    void GatherAllReferencedEntities(AZStd::unordered_set<AZ::EntityId>& entitiesWithReferences,
                                     AZ::SerializeContext& serializeContext);

    void PushEntitiesModal(const AzToolsFramework::EntityIdList& entities,
                           AZ::SerializeContext* serializeContext = nullptr);

    void DetachSliceEntities(const AzToolsFramework::EntityIdList& entities);

    void DetachSliceInstances(const AzToolsFramework::EntityIdList& entities);

    bool IsRootEntity(const AZ::Entity& entity) const;

    bool RootEntityTransforms(AZ::Data::Asset<AZ::SliceAsset>& targetSlice, const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& liveToAssetIdMap);

    AzFramework::EntityContextId GetEntityContextId() const { return m_entityContextId; }
    
    static AZ::Entity* FindAncestorInTargetSlice(
        const AZ::Data::Asset<AZ::SliceAsset>& targetSlice,
        const AZ::SliceComponent::EntityAncestorList& ancestors);

    static bool SaveSlice(
        const AZ::Data::Asset<AZ::SliceAsset>& slice,
        const char* fullPath,
        AZ::SerializeContext* serializeContext);


private:    // member functions

    static AZStd::string MakeTemporaryFilePathForSave(const char* targetFilename);

    void MakeSliceFromEntities(AzToolsFramework::EntityIdList& entities, bool inheritSlices);

    bool MakeNewSlice(const AzToolsFramework::EntityIdList& entities, 
                        const char* targetDirectory, 
                        bool inheritSlices, 
                        AZ::SerializeContext* serializeContext = nullptr);

    void GetTopLevelEntities(const AZ::SliceComponent::EntityList& entities, AZ::SliceComponent::EntityList& topLevelEntities);

    AZ::Entity* ValidateSingleRoot(const AzToolsFramework::EntityIdList& liveEntities,
        AzToolsFramework::EntityIdList& topLevelEntities, AZ::Entity*& insertBefore);

    bool ValidatePushSelection(AzToolsFramework::EntityIdList& entities, AZ::SerializeContext* serializeContext);

    //! \return whether user confirmed detach, false if cancelled
    bool ConfirmDialog_Detach(const QString& title, const QString& text);

private:    // data

    AzFramework::EntityContextId m_entityContextId;
};

