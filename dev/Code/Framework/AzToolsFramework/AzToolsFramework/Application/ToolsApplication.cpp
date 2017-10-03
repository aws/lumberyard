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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Debug/Profiler.h>

#include <AzFramework/TargetManagement/TargetManagementComponent.h>

#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/SelectionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/Archive/SevenZipComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzCore/Component/Component.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Entity/EditorEntityModelComponent.h>
#include <AzToolsFramework/UI/LegacyFramework/MainWindowSavedState.h>

#include <QtWidgets/QMessageBox>

// Not possible to use AZCore's operator new overrides until we address the overall problems
// with allocators. CryMemoryManager overrides new, so doing so here requires disabling theirs.
// Fortunately that's simple, but CryEngine makes heavy use of static-init time allocation, which
// is a no-no for AZ allocators. For now we'll stick with CryEngine's allocator.
//#include <AzToolsFramework/newoverride.inl>

namespace AzToolsFramework
{
    namespace Internal
    {
        template<typename IdContainerType>
        void DeleteEntities(const IdContainerType& entityIds)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            if (entityIds.empty())
            {
                return;
            }

            UndoSystem::URSequencePoint* currentUndoBatch = nullptr;
            EBUS_EVENT_RESULT(currentUndoBatch, ToolsApplicationRequests::Bus, GetCurrentUndoBatch);

            bool createdUndo = false;
            if (!currentUndoBatch)
            {
                createdUndo = true;
                EBUS_EVENT(ToolsApplicationRequests::Bus, BeginUndoBatch, "Delete Selected");
                EBUS_EVENT_RESULT(currentUndoBatch, ToolsApplicationRequests::Bus, GetCurrentUndoBatch);
                AZ_Assert(currentUndoBatch, "Failed to create new undo batch.");
            }

            UndoSystem::UndoStack* undoStack = nullptr;
            PreemptiveUndoCache* preemptiveUndoCache = nullptr;
            EBUS_EVENT_RESULT(undoStack, ToolsApplicationRequests::Bus, GetUndoStack);
            EBUS_EVENT_RESULT(preemptiveUndoCache, ToolsApplicationRequests::Bus, GetUndoCache);
            AZ_Assert(undoStack, "Failed to retrieve undo stack.");
            AZ_Assert(preemptiveUndoCache, "Failed to retrieve preemptive undo cache.");
            if (undoStack && preemptiveUndoCache)
            {
                // In order to undo DeleteSelected, we have to create a selection command which selects the current selection
                // and then add the deletion as children.
                // Commands always execute themselves first and then their children (when going forwards)
                // and do the opposite when going backwards.
                AzToolsFramework::EntityIdList selection;
                EBUS_EVENT_RESULT(selection, ToolsApplicationRequests::Bus, GetSelectedEntities);
                AzToolsFramework::SelectionCommand* selCommand = aznew AzToolsFramework::SelectionCommand(selection, "Delete Entities");

                // We insert a "deselect all" command before we delete the entities. This ensures the delete operations aren't changing
                // selection state, which triggers expensive UI updates. By deselecting up front, we are able to do those expensive
                // UI updates once at the start instead of once for each entity.
                {
                    AzToolsFramework::EntityIdList deselection;
                    AzToolsFramework::SelectionCommand* deselectAllCommand = aznew AzToolsFramework::SelectionCommand(deselection, "Deselect Entities");
                    deselectAllCommand->SetParent(selCommand);
                }

                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "Internal::DeleteEntities:UndoCaptureAndPurgeEntities");
                    for (const auto& entityId : entityIds)
                    {
                        AZ::Entity* entity = NULL;
                        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

                        if (entity)
                        {
                            EntityDeleteCommand* command = aznew EntityDeleteCommand(static_cast<AZ::u64>(entityId));
                            command->Capture(entity);
                            command->SetParent(selCommand);
                        }

                        preemptiveUndoCache->PurgeCache(entityId);

                        // send metrics event here for delete
                        EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, EntityDeleted, entityId);
                    }
                }

                selCommand->SetParent(currentUndoBatch);
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "Internal::DeleteEntities:RunRedo");
                    selCommand->RunRedo();
                }
            }

            if (createdUndo)
            {
                EBUS_EVENT(ToolsApplicationRequests::Bus, EndUndoBatch);
            }
        }

    } // Internal

    ToolsApplication::ToolsApplication()
        : m_selectionBounds(AZ::Aabb())
        , m_undoStack(nullptr)
        , m_currentBatchUndo(nullptr)
        , m_isDuringUndoRedo(false)
        , m_isInIsolationMode(false)
    {
        ToolsApplicationRequests::Bus::Handler::BusConnect();
    }

    ToolsApplication::~ToolsApplication()
    {
        ToolsApplicationRequests::Bus::Handler::BusDisconnect();
        Stop();
    }

    void ToolsApplication::RegisterCoreComponents()
    {
        AzFramework::Application::RegisterCoreComponents();

        RegisterComponentDescriptor(Components::TransformComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::SelectionComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::GenericComponentWrapper::CreateDescriptor());
        RegisterComponentDescriptor(Components::GenericComponentUnwrapper::CreateDescriptor());
        RegisterComponentDescriptor(Components::PropertyManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::ScriptEditorComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorSelectionAccentSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(EditorEntityContextComponent::CreateDescriptor());
        RegisterComponentDescriptor(SliceMetadataEntityContextComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorEntityActionComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorEntityIconComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorInspectorComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorLockComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorPendingCompositionComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorVisibilityComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorDisabledCompositionComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorEntitySortComponent::CreateDescriptor());
        RegisterComponentDescriptor(Components::EditorEntityModelComponent::CreateDescriptor());
        RegisterComponentDescriptor(AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(PerforceComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::SevenZipComponent::CreateDescriptor());
    }

    AZ::ComponentTypeList ToolsApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

        components.insert(components.end(), {
                azrtti_typeid<EditorEntityContextComponent>(),
                azrtti_typeid<SliceMetadataEntityContextComponent>(),
                azrtti_typeid<Components::EditorEntityActionComponent>(),
                azrtti_typeid<Components::GenericComponentUnwrapper>(),
                azrtti_typeid<Components::PropertyManagerComponent>(),
                azrtti_typeid<AzFramework::TargetManagementComponent>(),
                azrtti_typeid<AssetSystem::AssetSystemComponent>(),
                azrtti_typeid<PerforceComponent>(),
                azrtti_typeid<AzToolsFramework::SevenZipComponent>(),
                azrtti_typeid<Components::EditorEntityModelComponent>()
            });

        return components;
    }

    void ToolsApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Application::StartCommon(systemEntity);

        m_undoStack = new UndoSystem::UndoStack(10, nullptr);
    }

    void ToolsApplication::Stop()
    {
        if (m_isStarted)
        {
            m_undoCache.Clear();

            delete m_undoStack;
            m_undoStack = nullptr;

            // Release any memory used by ToolsApplication before Application::Stop() destroys the allocators.
            m_selectedEntities.set_capacity(0);
            m_highlightedEntities.set_capacity(0);

            m_serializeContext->DestroyEditContext();

            Application::Stop();
        }
    }

    void ToolsApplication::CreateSerializeContext()
    {
        if (m_serializeContext == nullptr)
        {
            m_serializeContext = aznew AZ::SerializeContext(true, true); // create EditContext
        }
    }

    void ToolsApplication::Reflect(AZ::ReflectContext* context)
    {
        Application::Reflect(context);

        Components::EditorComponentBase::Reflect(context);
        EditorAssetMimeDataContainer::Reflect(context);
        ComponentAssetMimeDataContainer::Reflect(context);

        AssetBrowser::AssetBrowserEntry::Reflect(context);
        AssetBrowser::RootAssetBrowserEntry::Reflect(context);
        AssetBrowser::FolderAssetBrowserEntry::Reflect(context);
        AssetBrowser::SourceAssetBrowserEntry::Reflect(context);
        AssetBrowser::ProductAssetBrowserEntry::Reflect(context);

        QTreeViewStateSaver::Reflect(context);
        QWidgetSavedState::Reflect(context);
        SliceUtilities::Reflect(context);
    }

    bool ToolsApplication::AddEntity(AZ::Entity* entity)
    {
        const bool result = AzFramework::Application::AddEntity(entity);

        if (result)
        {
            EBUS_EVENT(ToolsApplicationEvents::Bus, EntityRegistered, entity->GetId());
        }

        return result;
    }

    bool ToolsApplication::RemoveEntity(AZ::Entity* entity)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_undoCache.PurgeCache(entity->GetId());

        MarkEntityDeselected(entity->GetId());
        SetEntityHighlighted(entity->GetId(), false);

        EBUS_EVENT(ToolsApplicationEvents::Bus, EntityDeregistered, entity->GetId());

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "ToolsApplication::RemoveEntity:CallApplicationRemoveEntity");
            if (AzFramework::Application::RemoveEntity(entity))
            {
                return true;
            }
        }

        return false;
    }

    void ToolsApplication::PreExportEntity(AZ::Entity& source, AZ::Entity& target)
    {
        AZ_Assert(source.GetState() == AZ::Entity::ES_ACTIVE, "Source entity must be active.");
        AZ_Assert(target.GetState() != AZ::Entity::ES_ACTIVE, "Target entity must not be active.");

        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "No serialization context found");

        const AZ::Entity::ComponentArrayType& editorComponents = source.GetComponents();

        // Propagate components from source entity to target, in preparation for exporting target.
        for (AZ::Component* component : editorComponents)
        {
            Components::EditorComponentBase* asEditorComponent =
                azrtti_cast<Components::EditorComponentBase*>(component);

            if (asEditorComponent)
            {
                const size_t oldComponentCount = target.GetComponents().size();

                asEditorComponent->BuildGameEntity(&target);

                // Applying same Id persistence trick as we do in the slice compiler. Once we're off levels,
                // this code all goes away and everything runs through the slice copiler.
                if (target.GetComponents().size() > oldComponentCount)
                {
                    AZ::Component* newComponent = target.GetComponents().back();
                    AZ_Error("Export", asEditorComponent->GetId() != AZ::InvalidComponentId, "For entity \"%s\", component \"%s\" doesn't have a valid component id", 
                             source.GetName().c_str(), asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str());
                    newComponent->SetId(asEditorComponent->GetId());
                }
            }
            else
            {
                // The component is already runtime-ready. I.e. it is not an editor component.
                // Clone the component and add it to the export entity
                AZ::Component* clonedComponent = serializeContext->CloneObject(component);
                target.AddComponent(clonedComponent);
            }
        }
    }

    void ToolsApplication::PostExportEntity(AZ::Entity& source, AZ::Entity& target)
    {
        AZ_Assert(source.GetState() == AZ::Entity::ES_ACTIVE, "Source entity must be active.");
        AZ_Assert(target.GetState() != AZ::Entity::ES_ACTIVE, "Target entity must not be active.");

        const AZ::Entity::ComponentArrayType& editorComponents = source.GetComponents();

        for (AZ::Component* component : editorComponents)
        {
            Components::EditorComponentBase* asEditorComponent =
                azrtti_cast<Components::EditorComponentBase*>(component);

            if (asEditorComponent)
            {
                asEditorComponent->FinishedBuildingGameEntity(&target);
            }
        }
    }

    const char* ToolsApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseEditor";
#elif defined(_DEBUG)
        return "DebugEditor";
#else
        return "ProfileEditor";
#endif
    }

    void ToolsApplication::MarkEntitySelected(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        AZ_Assert(entityId.IsValid(), "Invalid entity Id being marked as selected.");

        auto foundIter = AZStd::find(m_selectedEntities.begin(), m_selectedEntities.end(), entityId);
        if (foundIter == m_selectedEntities.end())
        {
            EBUS_EVENT(ToolsApplicationEvents::Bus, BeforeEntitySelectionChanged);

            m_selectedEntities.push_back(entityId);

            EBUS_EVENT_ID(entityId, EntitySelectionEvents::Bus, OnSelected);

            EBUS_EVENT(ToolsApplicationEvents::Bus, AfterEntitySelectionChanged);
        }
    }

    void ToolsApplication::MarkEntityDeselected(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        auto foundIter = AZStd::find(m_selectedEntities.begin(), m_selectedEntities.end(), entityId);
        if (foundIter != m_selectedEntities.end())
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "ToolsApplication::MarkEntityDeselected:Deselect");
            EBUS_EVENT(ToolsApplicationEvents::Bus, BeforeEntitySelectionChanged);
            m_selectedEntities.erase(foundIter);

            EBUS_EVENT_ID(entityId, EntitySelectionEvents::Bus, OnDeselected);
            EBUS_EVENT(ToolsApplicationEvents::Bus, AfterEntitySelectionChanged);
        }
    }

    void ToolsApplication::SetEntityHighlighted(AZ::EntityId entityId, bool highlighted)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        AZ_Assert(entityId.IsValid(), "Attempting to mark an invalid entity as highlighted.");

        auto foundIter = AZStd::find(m_highlightedEntities.begin(), m_highlightedEntities.end(), entityId);
        if (foundIter != m_highlightedEntities.end())
        {
            if (!highlighted)
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "ToolsApplication::SetEntityHighlighted:RemoveHighlight");
                ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::BeforeEntityHighlightingChanged);
                m_highlightedEntities.erase(foundIter);
                ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::AfterEntityHighlightingChanged);
            }
        }
        else if (highlighted)
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "ToolsApplication::SetEntityHighlighted:AddHighlight");
            ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::BeforeEntityHighlightingChanged);
            m_highlightedEntities.push_back(entityId);
            ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::AfterEntityHighlightingChanged);
        }
    }

    void ToolsApplication::SetSelectedEntities(const EntityIdList& selectedEntities)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // We're setting the selection set as a batch from an external caller.
        // * Filter out any unselectable entities
        // * Calculate selection/deselection delta so we can notify specific entities only on change.
        // * Filter any duplicates.

        EntityIdList selectedEntitiesFiltered;
        selectedEntitiesFiltered.reserve(selectedEntities.size());
        for (AZ::EntityId nowSelectedId : selectedEntities)
        {
            AZ_Assert(nowSelectedId.IsValid(), "Invalid entity Id being marked as selected.");

            if (IsSelectable(nowSelectedId))
            {
                selectedEntitiesFiltered.push_back(nowSelectedId);
            }
        }

        EntityIdList newlySelectedIds;
        EntityIdList newlyDeselectedIds;

        for (AZ::EntityId nowSelectedId : selectedEntitiesFiltered)
        {
            auto alreadySelectedIter = AZStd::find(m_selectedEntities.begin(), m_selectedEntities.end(), nowSelectedId);

            if (alreadySelectedIter == m_selectedEntities.end())
            {
                newlySelectedIds.push_back(nowSelectedId);
            }
        }

        for (AZ::EntityId currentlySelectedId : m_selectedEntities)
        {
            auto stillSelectedIter = AZStd::find(selectedEntitiesFiltered.begin(), selectedEntitiesFiltered.end(), currentlySelectedId);

            if (stillSelectedIter == selectedEntitiesFiltered.end())
            {
                newlyDeselectedIds.push_back(currentlySelectedId);
            }
        }

        if (!newlySelectedIds.empty() || !newlyDeselectedIds.empty())
        {
            EBUS_EVENT(ToolsApplicationEvents::Bus, BeforeEntitySelectionChanged);

            m_selectedEntities.clear();

            // Guarantee no dupes, since external caller is unknown.
            for (AZ::EntityId id : selectedEntitiesFiltered)
            {
                auto foundIter = AZStd::find(m_selectedEntities.begin(), m_selectedEntities.end(), id);
                if (foundIter == m_selectedEntities.end())
                {
                    m_selectedEntities.push_back(id);
                }
            }

            for (AZ::EntityId id : newlySelectedIds)
            {
                EBUS_EVENT_ID(id, EntitySelectionEvents::Bus, OnSelected);
            }

            for (AZ::EntityId id : newlyDeselectedIds)
            {
                EBUS_EVENT_ID(id, EntitySelectionEvents::Bus, OnDeselected);
            }

            EBUS_EVENT(ToolsApplicationEvents::Bus, AfterEntitySelectionChanged);
        }
    }

    EntityIdSet ToolsApplication::GatherEntitiesAndAllDescendents(const EntityIdList& inputEntities)
    {
        EntityIdSet output;
        EntityIdList tempList;
        for (const AZ::EntityId& id : inputEntities)
        {
            output.insert(id);

            tempList.clear();
            EBUS_EVENT_ID_RESULT(tempList, id, AZ::TransformBus, GetAllDescendants);
            output.insert(tempList.begin(), tempList.end());
        }

        return output;
    }

    bool ToolsApplication::IsSelectable(const AZ::EntityId& entityId)
    {
        bool locked = false;
        AzToolsFramework::EditorLockComponentRequestBus::EventResult(locked, entityId, &AzToolsFramework::EditorLockComponentRequests::GetLocked);
        return !locked;
    }

    bool ToolsApplication::IsSelected(const AZ::EntityId& entityId)
    {
        return m_selectedEntities.end() != AZStd::find(m_selectedEntities.begin(), m_selectedEntities.end(), entityId);
    }

    void ToolsApplication::DeleteSelected()
    {
        Internal::DeleteEntities(m_selectedEntities);
    }

    void ToolsApplication::DeleteEntitiesAndAllDescendants(const EntityIdList& entities)
    {
        const EntityIdSet entitiesAndDescendants = GatherEntitiesAndAllDescendents(entities);
        Internal::DeleteEntities(entitiesAndDescendants);
    }

    bool ToolsApplication::FindCommonRoot(const AzToolsFramework::EntityIdSet& entitiesToBeChecked, AZ::EntityId& commonRootEntityId
        , AzToolsFramework::EntityIdList* topLevelEntities)
    {
        // Return value
        bool entitiesHaveCommonRoot = true;

        bool rootFound = false;
        commonRootEntityId.SetInvalid();

        // Finding the common root and top level entities
        for (const AZ::EntityId& id : entitiesToBeChecked)
        {
            AZ_Warning("Slices", AZ::TransformBus::FindFirstHandler(id) != nullptr,
                       "Entity with id %llu is not active, or does not have a transform component. This method is only valid for active entities. with a Transform component",
                       id);

            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, id, &AZ::TransformBus::Events::GetParentId);

            // If this entity is not a child of entities to be checked
            if (entitiesToBeChecked.find(parentId) == entitiesToBeChecked.end())
            {
                if (topLevelEntities)
                {
                    // Add it to the top level entities
                    topLevelEntities->push_back(id);
                }

                if (rootFound)
                {
                    // If the entities checked until now have a common root and the parent id of this entity is not the same as the common root
                    if (entitiesHaveCommonRoot && (parentId != commonRootEntityId))
                    {
                        // Entities do not have a common root
                        commonRootEntityId.SetInvalid();
                        entitiesHaveCommonRoot = false;
                    }
                }
                else
                {
                    commonRootEntityId = parentId;
                    rootFound = true;
                }
            }
        }

        return entitiesHaveCommonRoot;
    }

    bool ToolsApplication::FindCommonRootInactive(const AzToolsFramework::EntityList& entitiesToBeChecked, AZ::EntityId& commonRootEntityId, AzToolsFramework::EntityList* topLevelEntities)
    {
        // Return value
        bool entitiesHaveCommonRoot = true;

        bool rootFound = false;
        commonRootEntityId.SetInvalid();

        AzToolsFramework::EntityIdSet entityIds;
        for (const auto& entity : entitiesToBeChecked)
        {
            entityIds.insert(entity->GetId());
        }

        // Finding the common root and top level entities
        for (const AZ::Entity* entity : entitiesToBeChecked)
        {
            Components::TransformComponent* transformComponent =
                entity->FindComponent<Components::TransformComponent>();

            if (transformComponent)
            {
                const AZ::EntityId& parentId = transformComponent->GetParentId();
                // If this entity is not a child of entities to be checked
                if (entityIds.find(parentId) == entityIds.end())
                {
                    if (topLevelEntities)
                    {
                        topLevelEntities->push_back(const_cast<AZ::Entity*>(entity));
                    }

                    if (rootFound)
                    {
                        if (entitiesHaveCommonRoot && (parentId != commonRootEntityId))
                        {
                            commonRootEntityId.SetInvalid();
                            entitiesHaveCommonRoot = false;
                        }
                    }
                    else
                    {
                        commonRootEntityId = parentId;
                        rootFound = true;
                    }
                }
            }
        }

        return entitiesHaveCommonRoot;
    }

    bool ToolsApplication::RequestEditForFileBlocking(const char* assetPath, const char* progressMessage, const ToolsApplicationRequests::RequestEditProgressCallback& progressCallback)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO && !fileIO->IsReadOnly(assetPath))
        {
            SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, assetPath, true, [](bool, const SourceControlFileInfo&) {});
            return true;
        }

        bool editSuccess = false;
        bool editComplete = false;
        SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, assetPath, true,
            [&editSuccess, &editComplete](bool success, const AzToolsFramework::SourceControlFileInfo& /*info*/)
            {
                editSuccess = success;
                editComplete = true;
            }
        );

        QWidget* mainWindow = nullptr;
        EditorRequests::Bus::BroadcastResult(mainWindow, &EditorRequests::Bus::Events::GetMainWindow);
        AZ_Warning("RequestEdit", mainWindow, "Failed to retrieve main application window.");
        ProgressShield::LegacyShowAndWait(mainWindow, mainWindow ? mainWindow->tr(progressMessage) : QString(progressMessage),
            [&editComplete, &progressCallback](int& current, int& max)
            {
                if (progressCallback)
                {
                    progressCallback(current, max);
                }
                else
                {
                    current = 0;
                    max = 0;
                }
                return editComplete;
            }
        );

        return (fileIO && !fileIO->IsReadOnly(assetPath));
    }

    void ToolsApplication::RequestEditForFile(const char* assetPath, RequestEditResultCallback resultCallback)
    {
        AZ_Error("RequestEdit", resultCallback != 0, "User result callback is required.");

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO && !fileIO->IsReadOnly(assetPath))
        {
            if (resultCallback)
            {
                resultCallback(true);
            }
            return;
        }

        SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, assetPath, true,
            [resultCallback](bool success, const AzToolsFramework::SourceControlFileInfo& info)
            {
                if (resultCallback)
                {
                    success = !info.IsReadOnly();
                    resultCallback(success);
                }
            }
        );
    }

    void ToolsApplication::DeleteEntities(const EntityIdList& entities)
    {
        Internal::DeleteEntities(entities);
    }

    void ToolsApplication::AddDirtyEntity(AZ::EntityId entityId)
    {
        // If we're already in undo redo, we don't want the user to have to check for this each time.
        if (m_isDuringUndoRedo)
        {
            return;
        }

        m_dirtyEntities.insert(entityId);
    }

    int ToolsApplication::RemoveDirtyEntity(AZ::EntityId entityId)
    {
        return (int)m_dirtyEntities.erase(entityId);
    }

    void ToolsApplication::UndoPressed()
    {
        if (m_undoStack)
        {
            if (m_undoStack->CanUndo())
            {
                m_isDuringUndoRedo = true;
                EBUS_EVENT(ToolsApplicationEvents::Bus, BeforeUndoRedo);
                m_undoStack->Undo();
                EBUS_EVENT(ToolsApplicationEvents::Bus, AfterUndoRedo);
                m_isDuringUndoRedo = false;

#if defined(ENABLE_UNDOCACHE_CONSISTENCY_CHECKS)
                ConsistencyCheckUndoCache();
#endif
            }
        }
    }

    void ToolsApplication::RedoPressed()
    {
        if (m_undoStack)
        {
            if (m_undoStack->CanRedo())
            {
                m_isDuringUndoRedo = true;
                EBUS_EVENT(ToolsApplicationEvents::Bus, BeforeUndoRedo);
                m_undoStack->Redo();
                EBUS_EVENT(ToolsApplicationEvents::Bus, AfterUndoRedo);
                m_isDuringUndoRedo = false;

#if defined(ENABLE_UNDOCACHE_CONSISTENCY_CHECKS)
                ConsistencyCheckUndoCache();
#endif
            }
        }
    }

    void ToolsApplication::FlushUndo()
    {
        if (m_undoStack)
        {
            m_undoStack->Reset();
        }

        if (m_currentBatchUndo)
        {
            delete m_currentBatchUndo;
            m_currentBatchUndo = nullptr;
        }

        m_dirtyEntities.clear();
        m_isDuringUndoRedo = false;
    }

    UndoSystem::URSequencePoint* ToolsApplication::BeginUndoBatch(const char* label)
    {
        if (!m_currentBatchUndo)
        {
            m_currentBatchUndo = aznew UndoSystem::URSequencePoint(label, 0);
        }
        else
        {
            UndoSystem::URSequencePoint* pCurrent = m_currentBatchUndo;

            m_currentBatchUndo = aznew UndoSystem::URSequencePoint(label, 0);
            m_currentBatchUndo->SetParent(pCurrent);
        }

        // notify Cry undo has started (SandboxIntegrationManager)
        EBUS_EVENT(ToolsApplicationEvents::Bus, OnBeginUndo, label);

        return m_currentBatchUndo;
    }

    UndoSystem::URSequencePoint* ToolsApplication::ResumeUndoBatch(UndoSystem::URSequencePoint* expected, const char* label)
    {
        if (m_currentBatchUndo)
        {
            if (m_undoStack->GetTop() == m_currentBatchUndo)
            {
                m_undoStack->PopTop();
            }

            return m_currentBatchUndo;
        }

        if (m_undoStack)
        {
            auto ptr = m_undoStack->GetTop();
            if (ptr && ptr == expected)
            {
                m_currentBatchUndo = ptr;
                m_undoStack->PopTop();

                return m_currentBatchUndo;
            }
        }

        return BeginUndoBatch(label);
    }

    void ToolsApplication::EndUndoBatch()
    {
        AZ_Assert(m_currentBatchUndo, "Cannot end batch - no batch current");

        // notify Cry undo has ended (SandboxIntegrationManager)
        EBUS_EVENT(ToolsApplicationEvents::Bus, OnEndUndo, m_currentBatchUndo->GetName().c_str());

        if (m_currentBatchUndo->GetParent())
        {
            // pop one up
            m_currentBatchUndo = m_currentBatchUndo->GetParent();
        }
        else
        {
            // we're at root
            
            // only undo at bottom of scope (first invoked ScopedUndoBatch in 
            // chain/hierarchy must go out of scope)
            CreateUndosForDirtyEntities();
            m_dirtyEntities.clear();
            
            // record each undo batch
            if (m_undoStack)
            {
                m_undoStack->Post(m_currentBatchUndo);
            }
            else
            {
                delete m_currentBatchUndo;
            }

#if defined(ENABLE_UNDOCACHE_CONSISTENCY_CHECKS)
            ConsistencyCheckUndoCache();
#endif
            m_currentBatchUndo = nullptr;
        }
    }

    void ToolsApplication::CreateUndosForDirtyEntities()
    {
        AZ_Assert(!m_isDuringUndoRedo, "Cannot add dirty entities during undo/redo.");
        if (m_dirtyEntities.empty())
        {
            return;
        }

        for (auto it = m_dirtyEntities.begin(); it != m_dirtyEntities.end(); ++it)
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, *it);

            if (entity)
            {
                // see if it needs updating in the list:
                EntityStateCommand* pState = azdynamic_cast<EntityStateCommand*>(m_currentBatchUndo->Find(static_cast<AZ::u64>(*it), AZ::AzTypeInfo<EntityStateCommand>::Uuid()));
                if (!pState)
                {
                    pState = aznew EntityStateCommand(static_cast<AZ::u64>(*it));
                    pState->SetParent(m_currentBatchUndo);
                    // capture initial state of entity (before undo)
                    pState->Capture(entity, true);
                }

                // capture last state of entity (after undo) - for redo
                pState->Capture(entity, false);
            }

            m_undoCache.UpdateCache(*it);
        }
    }

    void ToolsApplication::ConsistencyCheckUndoCache()
    {
        for (auto && entityEntry : m_entities)
        {
            m_undoCache.Validate((entityEntry.second)->GetId());
        }
    }

    bool ToolsApplication::IsEntityEditable(AZ::EntityId entityId)
    {
        (void)entityId;

        return true;
    }

    bool ToolsApplication::AreEntitiesEditable(const EntityIdList& entityIds)
    {
        for (AZ::EntityId entityId : entityIds)
        {
            if (!IsEntityEditable(entityId))
            {
                return false;
            }
        }

        return true;
    }

    void ToolsApplication::CheckoutPressed()
    {
    }

    SourceControlFileInfo ToolsApplication::GetSceneSourceControlInfo()
    {
        return SourceControlFileInfo();
    }

    void ToolsApplication::EnterEditorIsolationMode()
    {
        if (!m_isInIsolationMode && m_selectedEntities.size() > 0)
        {
            m_isInIsolationMode = true;

            m_isolatedEntityIdSet.insert(m_selectedEntities.begin(), m_selectedEntities.end());

            // Add also entities in between selected entities along the transform hierarchy chain.
            // For example A has child B and B has child C, and if A and C are isolated, B is too.
            AZStd::vector<AZ::EntityId> inbetweenEntityIds;

            for (AZ::EntityId entityId : m_selectedEntities)
            {
                bool addInbetweenEntityIds = false;
                inbetweenEntityIds.clear();
                
                AZ::EntityId parentEntityId;
                AZ::TransformBus::EventResult(parentEntityId, entityId, &AZ::TransformBus::Events::GetParentId);

                while (parentEntityId.IsValid())
                {
                    EntityIdSet::iterator found = m_isolatedEntityIdSet.find(parentEntityId);
                    if (found != m_isolatedEntityIdSet.end())
                    {
                        addInbetweenEntityIds = true;
                        break;
                    }
                    inbetweenEntityIds.push_back(parentEntityId);

                    AZ::EntityId currentParentId = parentEntityId;
                    parentEntityId.SetInvalid();
                    AZ::TransformBus::EventResult(parentEntityId, currentParentId, &AZ::TransformBus::Events::GetParentId);
                }

                if (addInbetweenEntityIds)
                {
                    m_isolatedEntityIdSet.insert(inbetweenEntityIds.begin(), inbetweenEntityIds.end());
                }
            }


            for (const AZ::EntityId entityId : m_isolatedEntityIdSet)
            {
                ComponentEntityEditorRequestBus::Event(entityId, &ComponentEntityEditorRequestBus::Events::SetSandBoxObjectIsolated, true);
            }

            ToolsApplicationNotificationBus::Broadcast(&ToolsApplicationNotificationBus::Events::OnEnterEditorIsolationMode);
        }
    }

    void ToolsApplication::ExitEditorIsolationMode()
    {
        if (m_isInIsolationMode)
        {
            m_isInIsolationMode = false;

            for (const AZ::EntityId entityId : m_isolatedEntityIdSet)
            {
                ComponentEntityEditorRequestBus::Event(entityId, &ComponentEntityEditorRequestBus::Events::SetSandBoxObjectIsolated, false);
            }

            m_isolatedEntityIdSet.clear();

            ToolsApplicationNotificationBus::Broadcast(&ToolsApplicationNotificationBus::Events::OnExitEditorIsolationMode);
        }
    }

    bool ToolsApplication::IsEditorInIsolationMode()
    {
        return m_isInIsolationMode;
    }
} // namespace AzToolsFramework
