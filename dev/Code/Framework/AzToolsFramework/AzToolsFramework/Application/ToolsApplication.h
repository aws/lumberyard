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

#ifndef AZTOOLSFRAMEWORK_TOOLSAPPLICATION_H
#define AZTOOLSFRAMEWORK_TOOLSAPPLICATION_H

#include <AzCore/base.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Commands/PreemptiveUndoCache.h>

#pragma once

namespace AzToolsFramework
{
    class ToolsApplication
        : public AzFramework::Application
        , public ToolsApplicationRequests::Bus::Handler
    {
    public:
        AZ_RTTI(ToolsApplication, "{2895561E-BE90-4CC3-8370-DD46FCF74C01}", AzFramework::Application);
        AZ_CLASS_ALLOCATOR(ToolsApplication, AZ::SystemAllocator, 0);

        ToolsApplication(int* argc = nullptr, char*** argv = nullptr);
        ~ToolsApplication();

        void Stop();
        void CreateReflectionManager() override;
        void Reflect(AZ::ReflectContext* context) override;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

        AZ::Outcome<AZStd::string, AZStd::string> ResolveConfigToolsPath(const char* toolApplicationName) const override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void Start(const Descriptor& descriptor, const StartupParameters& startupParameters = StartupParameters()) override;
        void Start(const char* applicationDescriptorFile, const StartupParameters& startupParameters = StartupParameters())  override;

    protected:

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void StartCommon(AZ::Entity* systemEntity) override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        bool AddEntity(AZ::Entity* entity) override;
        bool RemoveEntity(AZ::Entity* entity) override;
        const char* GetCurrentConfigurationName() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ToolsApplicationRequests::Bus::Handler
        void PreExportEntity(AZ::Entity& source, AZ::Entity& target) override;
        void PostExportEntity(AZ::Entity& source, AZ::Entity& target) override;

        void MarkEntitySelected(AZ::EntityId entityId) override;
        void MarkEntityDeselected(AZ::EntityId entityId) override;
        void SetEntityHighlighted(AZ::EntityId entityId, bool highlighted) override;

        void AddDirtyEntity(AZ::EntityId entityId) override;
        int RemoveDirtyEntity(AZ::EntityId entityId) override;
        bool IsDuringUndoRedo() override { return m_isDuringUndoRedo; }
        void UndoPressed() override;
        void RedoPressed() override;
        void FlushUndo() override;
        void FlushRedo() override;
        UndoSystem::URSequencePoint* BeginUndoBatch(const char* label) override;
        UndoSystem::URSequencePoint* ResumeUndoBatch(UndoSystem::URSequencePoint* token, const char* label) override;
        void EndUndoBatch() override;

        bool IsEntityEditable(AZ::EntityId entityId) override;
        bool AreEntitiesEditable(const EntityIdList& entityIds) override;

        void CheckoutPressed() override;
        SourceControlFileInfo GetSceneSourceControlInfo() override;

        bool AreAnyEntitiesSelected() override { return !m_selectedEntities.empty(); }
        const EntityIdList& GetSelectedEntities() override { return m_selectedEntities; }
        const EntityIdList& GetHighlightedEntities() override { return m_highlightedEntities; }
        void SetSelectedEntities(const EntityIdList& selectedEntities) override;
        bool IsSelectable(const AZ::EntityId& entityId) override;
        bool IsSelected(const AZ::EntityId& entityId) override;
        UndoSystem::UndoStack* GetUndoStack() override { return m_undoStack; }
        UndoSystem::URSequencePoint* GetCurrentUndoBatch() override { return m_currentBatchUndo; }
        PreemptiveUndoCache* GetUndoCache() override { return &m_undoCache; }

        EntityIdSet GatherEntitiesAndAllDescendents(const EntityIdList& inputEntities) override;

        void DeleteSelected() override;
        void DeleteEntities(const EntityIdList& entities) override;
        void DeleteEntitiesAndAllDescendants(const EntityIdList& entities) override;
        bool FindCommonRoot(const EntityIdSet& entitiesToBeChecked, AZ::EntityId& commonRootEntityId, EntityIdList* topLevelEntities = nullptr) override;
        bool FindCommonRootInactive(const EntityList& entitiesToBeChecked, AZ::EntityId& commonRootEntityId, EntityList* topLevelEntities = nullptr) override;
        void FindTopLevelEntityIdsInactive(const EntityIdList& entityIdsToCheck, EntityIdList& topLevelEntityIds) override;
        AZ::SliceComponent::SliceInstanceAddress FindCommonSliceInstanceAddress(const EntityIdList& entityIds) override;
        AZ::EntityId GetRootEntityIdOfSliceInstance(AZ::SliceComponent::SliceInstanceAddress sliceAddress) override;

        bool RequestEditForFileBlocking(const char* assetPath, const char* progressMessage, const RequestEditProgressCallback& progressCallback) override;
        void RequestEditForFile(const char* assetPath, RequestEditResultCallback resultCallback) override;

        void EnterEditorIsolationMode() override;
        void ExitEditorIsolationMode() override;
        bool IsEditorInIsolationMode() override;
        const char* GetEngineRootPath() const override;
        const char* GetEngineVersion() const override;
        bool IsEngineRootExternal() const override;

        void CreateAndAddEntityFromComponentTags(const AZStd::vector<AZ::Crc32>& requiredTags, const char* entityName) override;

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::SimpleAssetRequests::Bus::Handler
        struct PathAssetEntry
        {
            explicit PathAssetEntry(const char* path)
                : m_path(path) {}
            explicit PathAssetEntry(AZStd::string&& path)
                : m_path(AZStd::move(path)) {}
            AZStd::string m_path;
        };
        //////////////////////////////////////////////////////////////////////////

        void CreateUndosForDirtyEntities();
        void ConsistencyCheckUndoCache();
        void InitializeEngineConfig();
        AZ::Aabb                            m_selectionBounds;
        EntityIdList                        m_selectedEntities;
        EntityIdList                        m_highlightedEntities;
        UndoSystem::UndoStack*              m_undoStack;
        UndoSystem::URSequencePoint*        m_currentBatchUndo;
        AZStd::unordered_set<AZ::EntityId>  m_dirtyEntities;
        PreemptiveUndoCache                 m_undoCache;
        bool                                m_isDuringUndoRedo;
        bool                                m_isInIsolationMode;
        EntityIdSet                         m_isolatedEntityIdSet;

        class EngineConfigImpl;
        AZStd::unique_ptr<EngineConfigImpl> m_engineConfigImpl;
    };
} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_TOOLSAPPLICATION_H
