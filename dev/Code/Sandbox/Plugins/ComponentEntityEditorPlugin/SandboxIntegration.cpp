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
#include "stdafx.h"

#include "SandboxIntegration.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/Commands/SliceDetachEntityCommand.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/UI/Layer/AddToLayerMenu.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/UI/Layer/NameConflictWarning.hxx>
#include <MathConversion.h>

#include "Objects/ComponentEntityObject.h"
#include "ComponentEntityDebugPrinter.h"
#include "ISourceControl.h"
#include "UI/QComponentEntityEditorMainWindow.h"

#include <LmbrCentral/Rendering/EditorLightComponentBus.h>
#include <LmbrCentral/Scripting/FlowGraphSerialization.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <LmbrCentral/Scripting/EditorTagComponentBus.h>

// Sandbox imports.
#include <Editor/ActionManager.h>
#include <Editor/CryEditDoc.h>
#include <Editor/GameEngine.h>
#include <Editor/DisplaySettings.h>
#include <Editor/Util/CubemapUtils.h>
#include <Editor/IconManager.h>
#include <Editor/Objects/ObjectLayer.h>
#include <Editor/Objects/ObjectLayerManager.h>
#include <Editor/Objects/ShapeObject.h>
#include <Editor/Settings.h>
#include <Editor/StringDlg.h>
#include <Editor/QtViewPaneManager.h>
#include <Editor/HyperGraph/FlowGraphModuleDlgs.h>
#include <Editor/HyperGraph/FlowGraphManager.h>
#include <Editor/HyperGraph/FlowGraph.h>
#include <Editor/HyperGraph/FlowGraphNode.h>
#include <Editor/AzAssetBrowser/AzAssetBrowserDialog.h>
#include <IResourceSelectorHost.h>
#include <Editor/AI/AIManager.h>
#include "CryEdit.h"

#include <CryCommon/FlowGraphInformation.h>

#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QHBoxLayout>
#include "MainWindow.h"

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzCore/std/algorithm.h>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

//////////////////////////////////////////////////////////////////////////
// Gathers all selected entities, culling any that have an ancestor in the selection.
void GetSelectedEntitiesSetWithFlattenedHierarchy(AzToolsFramework::EntityIdSet& out)
{
    AzToolsFramework::EntityIdList entities;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        entities,
        &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    for (const AZ::EntityId& entityId : entities)
    {
        bool selectionIncludesTransformHeritage = false;
        AZ::EntityId parent = entityId;
        do
        {
            AZ::EntityId nextParentId;
            AZ::TransformBus::EventResult(
                /*result*/ nextParentId,
                /*address*/ parent,
                &AZ::TransformBus::Events::GetParentId);
            parent = nextParentId;
            if (!parent.IsValid())
            {
                break;
            }
            for (const AZ::EntityId& parentCheck : entities)
            {
                if (parentCheck == parent)
                {
                    selectionIncludesTransformHeritage = true;
                    break;
                }
            }
        } while (parent.IsValid() && !selectionIncludesTransformHeritage);

        if (!selectionIncludesTransformHeritage)
        {
            out.insert(entityId);
        }
    }
}

SandboxIntegrationManager::SandboxIntegrationManager()
    : m_inObjectPickMode(false)
    , m_startedUndoRecordingNestingLevel(0)
    , m_dc(nullptr)
    , m_notificationWindowManager(new AzToolsFramework::SliceOverridesNotificationWindowManager())
    , m_entityDebugPrinter(aznew ComponentEntityDebugPrinter())
{
    // Required to receive events from the Cry Engine undo system
    GetIEditor()->GetUndoManager()->AddListener(this);
}

SandboxIntegrationManager::~SandboxIntegrationManager()
{
    GetIEditor()->GetUndoManager()->RemoveListener(this);
}

void SandboxIntegrationManager::Setup()
{
    AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
    AzToolsFramework::EditorRequests::Bus::Handler::BusConnect();

    // if the new viewport interaction model is enabled, then object picking is handled via
    // EditorPickEntitySelection and SandboxIntegrationManager is not required
    if (!IsNewViewportInteractionModelEnabled())
    {
        AzFramework::EntityContextId pickModeEntityContextId = GetEntityContextId();
        if (!pickModeEntityContextId.IsNull())
        {
            AzToolsFramework::EditorPickModeNotificationBus::Handler::BusConnect(pickModeEntityContextId);
        }
    }

    AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    AzToolsFramework::HyperGraphRequestBus::Handler::BusConnect();
    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
    AzFramework::EntityDebugDisplayRequestBus::Handler::BusConnect();
    AZ_POP_DISABLE_WARNING
    AzFramework::DebugDisplayRequestBus::Handler::BusConnect(
        AzToolsFramework::ViewportInteraction::g_mainViewportEntityDebugDisplayId);
    AzFramework::DisplayContextRequestBus::Handler::BusConnect();
    SetupFileExtensionMap();
    AZ::LegacyConversion::LegacyConversionRequestBus::Handler::BusConnect();
    AzToolsFramework::NewViewportInteractionModelEnabledRequestBus::Handler::BusConnect();

    MainWindow::instance()->GetActionManager()->RegisterActionHandler(ID_FILE_SAVE_SLICE_TO_ROOT, [this]() {
        SaveSlice(false);
    });
    MainWindow::instance()->GetActionManager()->RegisterActionHandler(ID_FILE_SAVE_SELECTED_SLICE, [this]() {
        SaveSlice(true);
    });
}

void SandboxIntegrationManager::SaveSlice(const bool& QuickPushToFirstLevel)
{
    AzToolsFramework::EntityIdList selectedEntities;
    GetSelectedEntities(selectedEntities);
    if (selectedEntities.size() == 0)
    {
        m_notificationWindowManager->CreateNotificationWindow(AzToolsFramework::SliceOverridesNotificationWindow::EType::TypeError, "Nothing selected - Select a slice entity with overrides and try again");
        return;
    }

    AzToolsFramework::EntityIdList relevantEntities;
    AZ::u32 entitiesInSlices;
    AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;
    GetEntitiesInSlices(selectedEntities, entitiesInSlices, sliceInstances);
    if (entitiesInSlices > 0)
    {
        // Gather the set of relevant entities from the selected entities and all descendants
        AzToolsFramework::EntityIdSet relevantEntitiesSet;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(relevantEntitiesSet,
            &AzToolsFramework::ToolsApplicationRequestBus::Events::GatherEntitiesAndAllDescendents, selectedEntities);

        relevantEntities.reserve(relevantEntitiesSet.size());
        for (AZ::EntityId& id : relevantEntitiesSet)
        {
            relevantEntities.push_back(id);
        }
    }

    int numEntitiesToAdd = 0;
    int numEntitiesToRemove = 0;
    int numEntitiesToUpdate = 0;
    if (AzToolsFramework::SliceUtilities::SaveSlice(relevantEntities, numEntitiesToAdd, numEntitiesToRemove, numEntitiesToUpdate, QuickPushToFirstLevel))
    {
        if (numEntitiesToAdd > 0 || numEntitiesToRemove > 0 || numEntitiesToUpdate > 0)
        {
            m_notificationWindowManager->CreateNotificationWindow(
                AzToolsFramework::SliceOverridesNotificationWindow::EType::TypeSuccess,
                QString("Save slice to parent - %1 saved successfully").arg(numEntitiesToUpdate + numEntitiesToAdd + numEntitiesToRemove));
        }
        else
        {
            m_notificationWindowManager->CreateNotificationWindow(
                AzToolsFramework::SliceOverridesNotificationWindow::EType::TypeError,
                "Selected has no overrides - Select a slice entity with overrides and try again");
        }
    }
    else
    {
        m_notificationWindowManager->CreateNotificationWindow(
            AzToolsFramework::SliceOverridesNotificationWindow::EType::TypeError,
            "Save slice to parent - Failed");
    }
}

// This event handler is queued on main thread.
void SandboxIntegrationManager::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
{
    AZ::SliceComponent* editorRootSlice = nullptr;
    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(editorRootSlice, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorRootSlice);
    AZ_Assert(editorRootSlice, "Editor root slice missing!");

    for (auto restoreItr = m_sliceAssetDeletionErrorRestoreInfos.begin(); restoreItr != m_sliceAssetDeletionErrorRestoreInfos.end(); )
    {
        if (restoreItr->m_assetId == assetId)
        {
            for (const auto& entityRestore : restoreItr->m_entityRestoreInfos)
            {
                const AZ::EntityId& entityId = entityRestore.first;
                const AZ::SliceComponent::EntityRestoreInfo& restoreInfo = entityRestore.second;

                AZ::Entity* entity = editorRootSlice->FindEntity(entityId);
                if (entity)
                {
                    AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                        &AzToolsFramework::EditorEntityContextRequestBus::Events::RestoreSliceEntity, entity, restoreInfo, AzToolsFramework::SliceEntityRestoreType::Detached);
                }
                else
                {
                    AZ_Error("DetachSliceEntity", entity, "Unable to find previous detached entity of Id %s. Cannot undo \"Detach\" action.", entityId.ToString().c_str());
                }
            }

            restoreItr = m_sliceAssetDeletionErrorRestoreInfos.erase(restoreItr);
        }
        else
        {
            restoreItr++;
        }
    }
}

// This event handler will be triggered on a non-main thread. No mutex is used for now because the only 
// operation writing to shared resource is queued on main thread.
void SandboxIntegrationManager::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId)
{
    // Check to see if the removed slice asset has any instance in the level, then check if 
    // those dangling instances are directly under the root slice (not sub-slices). If yes,
    // detach them and save necessary information so they can be restored when their slice asset
    // comes back.

    AZ::Data::AssetInfo assetInfo;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
    if (assetInfo.m_assetType == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
    {
        AZ::SliceComponent* rootSlice = nullptr;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(rootSlice, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorRootSlice);
        AZ_Assert(rootSlice, "Editor root slice missing!");

        AZStd::vector<AZ::EntityId> entitiesToDetach;
        const AZ::SliceComponent::SliceList& subSlices = rootSlice->GetSlices();
        for (const AZ::SliceComponent::SliceReference& subSliceRef : subSlices)
        {
            if (subSliceRef.GetSliceAsset().GetId() == assetId)
            {
                for (const AZ::SliceComponent::SliceInstance& sliceInst : subSliceRef.GetInstances())
                {
                    const AZ::SliceComponent::InstantiatedContainer* instContainer = sliceInst.GetInstantiated();
                    for (AZ::Entity* entity : instContainer->m_entities)
                    {
                        entitiesToDetach.emplace_back(entity->GetId());
                    }
                }
            }
        }

        AZ_Error("Editor", false, "The slice asset %s is deleted from disk, to prevent further data corruption, all of its root level slice instances are detached. "
            "Restoring the slice asset on disk will revert the detaching operation.", assetInfo.m_relativePath.c_str());

        AZ::SystemTickBus::QueueFunction([this, assetId, entitiesToDetach]() {
            AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityRestoreInfo>> restoreInfos;
            bool detachSuccess = false;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                detachSuccess, &AzToolsFramework::ToolsApplicationRequestBus::Events::DetachEntities, entitiesToDetach, restoreInfos);
            if (detachSuccess)
            {
                m_sliceAssetDeletionErrorRestoreInfos.emplace_back(assetId, AZStd::move(restoreInfos));
            }
        });
    }
}

void SandboxIntegrationManager::GetEntitiesInSlices(
    const AzToolsFramework::EntityIdList& selectedEntities,
    AZ::u32& entitiesInSlices,
    AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sliceInstances)
{
    // Identify all slice instances affected by the selected entity set.
    entitiesInSlices = 0;
    for (const AZ::EntityId& entityId : selectedEntities)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::EntityIdContextQueryBus::EventResult(sliceAddress, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);

        if (sliceAddress.IsValid())
        {
            ++entitiesInSlices;

            if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), sliceAddress))
            {
                sliceInstances.push_back(sliceAddress);
            }
        }
    }
}

void SandboxIntegrationManager::Teardown()
{
    AzToolsFramework::NewViewportInteractionModelEnabledRequestBus::Handler::BusDisconnect();
    AZ::LegacyConversion::LegacyConversionRequestBus::Handler::BusDisconnect();
    AzFramework::DisplayContextRequestBus::Handler::BusDisconnect();
    AzFramework::DebugDisplayRequestBus::Handler::BusDisconnect();
    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
    AzFramework::EntityDebugDisplayRequestBus::Handler::BusDisconnect();
    AZ_POP_DISABLE_WARNING
    AzToolsFramework::HyperGraphRequestBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();

    if (!IsNewViewportInteractionModelEnabled())
    {
        AzToolsFramework::EditorPickModeNotificationBus::Handler::BusDisconnect();
    }

    AzToolsFramework::EditorRequests::Bus::Handler::BusDisconnect();
    AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
}

void SandboxIntegrationManager::SetDC(DisplayContext* dc)
{
    m_dc = dc;
}

DisplayContext* SandboxIntegrationManager::GetDC()
{
    return m_dc;
}

void SandboxIntegrationManager::OnBeginUndo(const char* label)
{
    AzToolsFramework::UndoSystem::URSequencePoint* currentBatch = nullptr;
    EBUS_EVENT_RESULT(currentBatch, AzToolsFramework::ToolsApplicationRequests::Bus, GetCurrentUndoBatch);

    AZ_Assert(currentBatch, "No undo batch is active.");

    // Only generate a Sandbox placeholder for root-level undo batches.
    if (nullptr == currentBatch->GetParent())
    {
        // start Cry Undo
        if (!CUndo::IsRecording())
        {
            GetIEditor()->BeginUndo();
            // flag that we started recording the undo batch
            m_startedUndoRecordingNestingLevel = 1;
        }

    }
    else
    {
        if (m_startedUndoRecordingNestingLevel)
        {
            // if we previously started recording the undo, increment the nesting level so we can
            // detect when we need to accept the undo in OnEndUndo()
            m_startedUndoRecordingNestingLevel++;
        }
    }
}

void SandboxIntegrationManager::OnEndUndo(const char* label, bool changed)
{
    // Add the undo only after we know it's got a legit change, we can't remove undos from the cry undo system so we do it here instead of OnBeginUndo
    if (changed && CUndo::IsRecording())
    {
        CUndo::Record(new CToolsApplicationUndoLink(label));
    }
    if (m_startedUndoRecordingNestingLevel)
    {
        m_startedUndoRecordingNestingLevel--;
        if (m_startedUndoRecordingNestingLevel == 0)
        {
            if (changed)
            {
                // only accept the undo batch that we initially started undo recording on
                GetIEditor()->AcceptUndo(label);
            }
            else
            {
                GetIEditor()->CancelUndo();
            }
        }
    }
}

void SandboxIntegrationManager::EntityParentChanged(
    const AZ::EntityId entityId,
    const AZ::EntityId newParentId,
    const AZ::EntityId oldParentId)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

    if (m_unsavedEntities.find(entityId) != m_unsavedEntities.end())
    {
        // New layers need the level to be saved.
        bool isEntityLayer = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isEntityLayer,
            entityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
        if (isEntityLayer)
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                entityId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AddLevelSaveDependency);
        }
        // Don't need to track any other unsaved changes, this is a new entity that hasn't been saved yet.
        return;
    }

    // If an entity is moved to or from a layer, then that layer can only safely be saved when the other layer or level saves, to prevent
    // accidental duplication of entities.
    // This logic doesn't clear the dependency flag if an entity changes parents multiple times between saves, so if an entity visits many layers
    // before finally being saved, it will result in all of those layers saving, too.
    AZ::EntityId oldAncestor = oldParentId;

    bool wasNotInLayer = false;
    AZ::EntityId oldLayer;
    do
    {
        if (!oldAncestor.IsValid())
        {
            wasNotInLayer = true;
            break;
        }

        bool isOldAncestorLayer = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isOldAncestorLayer,
            oldAncestor,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
        if (isOldAncestorLayer)
        {
            oldLayer = oldAncestor;
            break;
        }

        // Must pass in an invalid id, because if no parent then nothing will modify the id variable to be invalid and stop at the no-parent case
        AZ::EntityId nextParentId;
        AZ::TransformBus::EventResult(
            /*result*/ nextParentId,
            /*address*/ oldAncestor,
            &AZ::TransformBus::Events::GetParentId);
        oldAncestor = nextParentId;
    } while (oldAncestor.IsValid());

    AZ::EntityId newAncestors = newParentId;
    AZ::EntityId newAncestor = newParentId;

    bool isGoingToRootScene = false;
    AZ::EntityId newLayer;
    do
    {
        if (!newAncestor.IsValid())
        {
            isGoingToRootScene = true;
            break;
        }

        bool isNewAncestorLayer = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isNewAncestorLayer,
            newAncestor,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
        if (isNewAncestorLayer)
        {
            newLayer = newAncestor;
            break;
        }
        // The parent may not be connected to the bus yet, so start with an invalid entity ID
        // to prevent an infinite loop.
        AZ::EntityId ancestorParent;
        AZ::TransformBus::EventResult(
            /*result*/ ancestorParent,
            /*address*/ newAncestor,
            &AZ::TransformBus::Events::GetParentId);
        newAncestor = ancestorParent;
    } while (newAncestor.IsValid());

    if (oldLayer.IsValid() && newLayer != oldLayer)
    {
        if (newLayer.IsValid())
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                oldLayer,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AddLayerSaveDependency,
                newLayer);
        }
        else
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                oldLayer,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AddLevelSaveDependency);
        }
    }

    if (newLayer.IsValid() && newLayer != oldLayer)
    {
        if (oldLayer.IsValid())
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                newLayer,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AddLayerSaveDependency,
                oldLayer);
        }
        else
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                newLayer,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AddLevelSaveDependency);
        }
    }
}

void SandboxIntegrationManager::OnSaveLevel()
{
    m_unsavedEntities.clear();
}

void SandboxIntegrationManager::PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags)
{
    if (!IsLevelDocumentOpen())
    {
        return;
    }

    if (flags & AzToolsFramework::EditorEvents::eECMF_USE_VIEWPORT_CENTER)
    {
        CViewport* view = GetIEditor()->GetViewManager()->GetGameViewport();
        int width = 0;
        int height = 0;
        // If there is no 3D Viewport active to aid in the positioning of context menu
        // operations, we don't need to store anything but default values here. Any code
        // using these numbers for placement should default to the origin when there's
        // no 3D viewport to raycast into.
        if (view)
        {
            view->GetDimensions(&width, &height);
        }
        m_contextMenuViewPoint.Set(width / 2, height / 2);
    }
    else
    {
        m_contextMenuViewPoint = point;
    }

    CGameEngine* gameEngine = GetIEditor()->GetGameEngine();
    if (!gameEngine || !gameEngine->IsLevelLoaded())
    {
        return;
    }

    menu->setToolTipsVisible(true);

    AzToolsFramework::EntityIdList selected;
    GetSelectedOrHighlightedEntities(selected);

    QAction* action = nullptr;

    action = menu->addAction(QObject::tr("Create entity"));
    QObject::connect(action, &QAction::triggered, action, [this] { ContextMenu_NewEntity(); });

    if (selected.size() == 1)
    {
        action = menu->addAction(QObject::tr("Create child entity"));
        QObject::connect(action, &QAction::triggered, action, [selected]
        {
            AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu);
            EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, CreateNewEntityAsChild, selected.front());
        });
    }

    menu->addSeparator();

    action = menu->addAction(QObject::tr("Create layer"));
    QObject::connect(action, &QAction::triggered, [this] { ContextMenu_NewLayer(); });

    SetupLayerContextMenu(menu);
    AzToolsFramework::EntityIdSet flattenedSelection;
    GetSelectedEntitiesSetWithFlattenedHierarchy(flattenedSelection);
    AzToolsFramework::SetupAddToLayerMenu(menu, flattenedSelection, [this] { return ContextMenu_NewLayer(); });

    SetupSliceContextMenu(menu);


    action = menu->addAction(QObject::tr("Duplicate"));
    QObject::connect(action, &QAction::triggered, action, [this] { ContextMenu_Duplicate(); });
    if (selected.size() == 0)
    {
        action->setDisabled(true);
    }

    action = menu->addAction(QObject::tr("Delete"));
    QObject::connect(action, &QAction::triggered, action, [this] { ContextMenu_DeleteSelected(); });
    if (selected.size() == 0)
    {
        action->setDisabled(true);
    }

    menu->addSeparator();
    SetupFlowGraphContextMenu(menu);
    menu->addSeparator();

    if (selected.size() > 0)
    {
        action = menu->addAction(QObject::tr("Open pinned Inspector"));
        QObject::connect(action, &QAction::triggered, action, [this, selected]
        {
            OpenPinnedInspector(selected);
        });
        if (selected.size() > 0)
        {
            action = menu->addAction(QObject::tr("Find in Entity Outliner"));
            QObject::connect(action, &QAction::triggered, [this, selected]
            {
                AzToolsFramework::EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnFocusInEntityOutliner, selected);
            });
        }
        menu->addSeparator();
    }
}

void SandboxIntegrationManager::OpenPinnedInspector(const AzToolsFramework::EntityIdList& entities)
{
    QDockWidget* dockWidget = InstanceViewPane(LyViewPane::EntityInspectorPinned);
    if (dockWidget)
    {
        QComponentEntityEditorInspectorWindow* editor = static_cast<QComponentEntityEditorInspectorWindow*>(dockWidget->widget());
        if (editor && editor->GetPropertyEditor())
        {
            editor->GetPropertyEditor()->SetOverrideEntityIds(entities);

            AZStd::string widgetTitle;
            if (entities.size() == 1)
            {
                AZStd::string entityName;
                AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, entities[0]);
                widgetTitle = AZStd::string::format("%s Inspector", entityName.c_str());

                QObject::connect(editor->GetPropertyEditor(), &AzToolsFramework::EntityPropertyEditor::SelectedEntityNameChanged, [dockWidget](const AZ::EntityId& /*entityId*/, const AZStd::string& name)
                {
                    AZStd::string newTitle = AZStd::string::format("%s Inspector", name.c_str());
                    dockWidget->setWindowTitle(newTitle.c_str());
                });
            }
            else
            {
                widgetTitle = AZStd::string::format("%d Entities - Inspector", entities.size());
            }

            dockWidget->setWindowTitle(widgetTitle.c_str());
        }
    }
}

void SandboxIntegrationManager::ClosePinnedInspector(AzToolsFramework::EntityPropertyEditor* editor)
{
    QWidget* currentWidget = editor->parentWidget();
    while (currentWidget)
    {
        QDockWidget* dockWidget = qobject_cast<QDockWidget*>(currentWidget);
        if (dockWidget)
        {
            QtViewPaneManager::instance()->ClosePaneInstance(LyViewPane::EntityInspectorPinned, dockWidget);
            return;
        }
        currentWidget = currentWidget->parentWidget();
    }
}

void SandboxIntegrationManager::SetupLayerContextMenu(QMenu* menu)
{
    // First verify that the selection contains some layers.
    AzToolsFramework::EntityIdList selectedEntities;
    GetSelectedOrHighlightedEntities(selectedEntities);

    // Nothing selected, no layer context menus to add.
    if (selectedEntities.empty())
    {
        return;
    }

    AZStd::unordered_set<AZ::EntityId> layersInSelection;

    for (AZ::EntityId entityId : selectedEntities)
    {
        bool isLayerEntity = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerEntity,
            entityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
        if (isLayerEntity)
        {
            layersInSelection.insert(entityId);
        }
    }

    // No layers directly selected, do not add context, even if a selected entity
    // is a child of a layer.
    if (layersInSelection.empty())
    {
        return;
    }

    menu->addSeparator();

    const int selectedLayerCount = layersInSelection.size();
    QString saveTitle = QObject::tr("Save layer");
    if(selectedLayerCount > 1)
    {
        saveTitle = QObject::tr("Save %0 layers").arg(selectedLayerCount);
    }

    QAction* saveLayerAction = menu->addAction(saveTitle);
    saveLayerAction->setToolTip(QObject::tr("Save the selected layers."));
    QObject::connect(saveLayerAction, &QAction::triggered, [this, layersInSelection] { ContextMenu_SaveLayers(layersInSelection); });
}

void SandboxIntegrationManager::SetupSliceContextMenu(QMenu* menu)
{
    AzToolsFramework::EntityIdList selectedEntities;
    GetSelectedOrHighlightedEntities(selectedEntities);

    menu->addSeparator();

    if (!selectedEntities.empty())
    {
        bool anySelectedEntityFromExistingSlice = false;
        bool layerInSelection = false;
        for (AZ::EntityId entityId : selectedEntities)
        {
            if (!anySelectedEntityFromExistingSlice)
            {
                AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
                AzFramework::EntityIdContextQueryBus::EventResult(sliceAddress, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningSlice);
                if (sliceAddress.GetReference())
                {
                    anySelectedEntityFromExistingSlice = true;
                }
            }
            if (!layerInSelection)
            {
                bool isLayerEntity = false;
                AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                    isLayerEntity,
                    entityId,
                    &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
                if (isLayerEntity)
                {
                    layerInSelection = true;
                }
            }
            // Everything being searched for was found, so exit the loop.
            if (layerInSelection && anySelectedEntityFromExistingSlice)
            {
                break;
            }
        }

        // Layers can't be in slices.
        if (!layerInSelection)
        {
            QAction* createAction = menu->addAction(QObject::tr("Create slice..."));
            createAction->setToolTip(QObject::tr("Creates a slice out of the currently selected entities"));
            QObject::connect(createAction, &QAction::triggered, createAction, [this, selectedEntities] { ContextMenu_InheritSlice(selectedEntities); });

            if (anySelectedEntityFromExistingSlice)
            {
                QAction* createAction = menu->addAction(QObject::tr("Create detached slice..."));
                createAction->setToolTip(QObject::tr("Creates a slice out of the currently selected entities. This action does not nest any existing slice that might be associated with the selected entities."));
                QObject::connect(createAction, &QAction::triggered, createAction, [this, selectedEntities] { ContextMenu_MakeSlice(selectedEntities); });
            }
        }
    }

    QAction* instantiateAction = menu->addAction(QObject::tr("Instantiate slice..."));
    instantiateAction->setToolTip(QObject::tr("Instantiates a pre-existing slice asset into the level"));
    QObject::connect(instantiateAction, &QAction::triggered, instantiateAction, [this] { ContextMenu_InstantiateSlice(); });

    AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::Bus::Events::PopulateEditorGlobalContextMenu_SliceSection, menu, AZ::Vector2::CreateZero(),
        AzToolsFramework::EditorEvents::eECMF_HIDE_ENTITY_CREATION | AzToolsFramework::EditorEvents::eECMF_USE_VIEWPORT_CENTER);

    if (selectedEntities.empty())
    {
        return;
    }

    AZ::u32 entitiesInSlices;
    AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;
    GetEntitiesInSlices(selectedEntities, entitiesInSlices, sliceInstances);
    // Offer slice-related options if any selected entities belong to slice instances.
    if (0 == entitiesInSlices)
    {
        return;
    }

    // Setup push and revert options (quick push and 'advanced' push UI).
    SetupSliceContextMenu_Modify(menu, selectedEntities, entitiesInSlices);

    menu->addSeparator();

    // PopulateFindSliceMenu takes a callback to run when a slice is selected, which is called before the slice is selected in the asset browser.
    // This is so the AssetBrowser can be opened first, which can only be done from a Sandbox module. PopulateFindSliceMenu exists in the AzToolsFramework
    // module in SliceUtilities, so it can share logic with similar menus, like Quick Push.
    // Similarly, it takes a callback for the SliceRelationshipView.
    AzToolsFramework::SliceUtilities::PopulateSliceSubMenus(*menu, selectedEntities,
        []
    {
        // This will open the AssetBrowser if it's not open, and do nothing if it's already opened.
        QtViewPaneManager::instance()->OpenPane(LyViewPane::AssetBrowser);
    },
        []
    {
        //open SliceRelationshipView if necessary, and populate it
        QtViewPaneManager::instance()->OpenPane(LyViewPane::SliceRelationships);
    });
}

void SandboxIntegrationManager::SetupSliceContextMenu_Modify(QMenu* menu, const AzToolsFramework::EntityIdList& selectedEntities, const AZ::u32 numEntitiesInSlices)
{
    using namespace AzToolsFramework;

    // Gather the set of relevant entities from the selected entities and all descendants
    AzToolsFramework::EntityIdSet relevantEntitiesSet;
    EBUS_EVENT_RESULT(relevantEntitiesSet, AzToolsFramework::ToolsApplicationRequests::Bus, GatherEntitiesAndAllDescendents, selectedEntities);
    AzToolsFramework::EntityIdList relevantEntities;
    relevantEntities.reserve(relevantEntitiesSet.size());
    for (AZ::EntityId& id : relevantEntitiesSet)
    {
        relevantEntities.push_back(id);
    }

    SliceUtilities::PopulateQuickPushMenu(*menu, relevantEntities);

    SliceUtilities::PopulateDetachMenu(*menu, selectedEntities, relevantEntitiesSet);

    bool canRevert = false;

    for (AZ::EntityId id : relevantEntitiesSet)
    {
        bool entityHasOverrides = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(entityHasOverrides, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::HasSliceEntityOverrides);
        if (entityHasOverrides)
        {
            canRevert = true;
            break;
        }
    }

    QAction* revertAction = menu->addAction(QObject::tr("Revert overrides"));
    revertAction->setToolTip(QObject::tr("Revert all slice entities and their children to their last saved state."));

    QObject::connect(revertAction, &QAction::triggered, [this, relevantEntities]
    {
        ContextMenu_ResetToSliceDefaults(relevantEntities);
    });

    revertAction->setEnabled(canRevert);
}

void SandboxIntegrationManager::SetupFlowGraphContextMenu(QMenu* menu)
{
    AzToolsFramework::EntityIdList selectedEntities;
    GetSelectedOrHighlightedEntities(selectedEntities);

    if (!selectedEntities.empty())
    {
        // Separate entities into those that already have flowgraph components and those that do not.
        AzToolsFramework::EntityIdList entitiesWithFlowgraphComponent;
        for (const AZ::EntityId& entityId : selectedEntities)
        {
            if (entityId.IsValid())
            {
                AZ::Entity* foundEntity = nullptr;
                EBUS_EVENT_RESULT(foundEntity, AZ::ComponentApplicationBus, FindEntity, entityId);

                if (FlowGraphEditorRequestsBus::FindFirstHandler(FlowEntityId(entityId)))
                {
                    entitiesWithFlowgraphComponent.push_back(entityId);
                }
            }
        }

        QMenu* flowgraphMenu = nullptr;
        QAction* action = nullptr;

        // For entities with flowgraph component, create a context menu to open any existing flowgraphs within each selected entity.
        for (const AZ::EntityId& entityId : entitiesWithFlowgraphComponent)
        {
            AZStd::vector<AZStd::pair<AZStd::string, IFlowGraph*> > flowgraphs;
            EBUS_EVENT_ID(FlowEntityId(entityId), FlowGraphEditorRequestsBus, GetFlowGraphs, flowgraphs);

            if (!flowgraphMenu)
            {
                menu->addSeparator();
                flowgraphMenu = menu->addMenu(QObject::tr("Flow Graph"));
                menu->addSeparator();
            }

            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

            QMenu* entityMenu = flowgraphMenu;
            if (selectedEntities.size() > 1)
            {
                entityMenu = flowgraphMenu->addMenu(entity->GetName().c_str());
            }

            action = entityMenu->addAction("Add");
            QObject::connect(action, &QAction::triggered, action, [this, entityId] { ContextMenu_AddFlowGraph(entityId); });

            if (!flowgraphs.empty())
            {
                QMenu* openMenu = entityMenu->addMenu(QObject::tr("Open"));
                QMenu* removeMenu = entityMenu->addMenu(QObject::tr("Remove"));
                for (auto& flowgraph : flowgraphs)
                {
                    action = openMenu->addAction(flowgraph.first.c_str());
                    auto flowGraph = flowgraph.second;
                    QObject::connect(action, &QAction::triggered, action, [this, entityId, flowGraph] { ContextMenu_OpenFlowGraph(entityId, flowGraph); });
                    action = removeMenu->addAction(flowgraph.first.c_str());
                    QObject::connect(action, &QAction::triggered, action, [this, entityId, flowGraph] { ContextMenu_RemoveFlowGraph(entityId, flowGraph); });
                }
            }
        }
    }
}

void SandboxIntegrationManager::HandleObjectModeSelection(const AZ::Vector2& point, int flags, bool& handled)
{
    // Todo - Use a custom "edit tool". This will eliminate the need for this bus message entirely, which technically
    // makes this feature less intrusive on Sandbox.
    // UPDATE: This is now provided by EditorPickEntitySelection when the new Viewport Interaction Model changes are enabled.
    if (m_inObjectPickMode)
    {
        CViewport* view = GetIEditor()->GetViewManager()->GetGameViewport();
        const QPoint viewPoint(point.GetX(), point.GetY());

        HitContext hitInfo;
        hitInfo.view = view;
        if (view->HitTest(viewPoint, hitInfo))
        {
            if (hitInfo.object && (hitInfo.object->GetType() == OBJTYPE_AZENTITY))
            {
                CComponentEntityObject* entityObject = static_cast<CComponentEntityObject*>(hitInfo.object);
                AzToolsFramework::EditorPickModeRequestBus::Broadcast(
                    &AzToolsFramework::EditorPickModeRequests::PickModeSelectEntity, entityObject->GetAssociatedEntityId());
            }
        }

        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);

        handled = true;
    }
}

void SandboxIntegrationManager::UpdateObjectModeCursor(AZ::u32& cursorId, AZStd::string& cursorStr)
{
    if (m_inObjectPickMode)
    {
        cursorId = static_cast<AZ::u64>(STD_CURSOR_HAND);
        cursorStr = "Pick an entity...";
    }
}

void SandboxIntegrationManager::OnEntityPickModeStarted()
{
    m_inObjectPickMode = true;

    // Currently this object pick mode is activated only via PropertyEntityIdCtrl picker.
    // When the picker button is clicked, we transfer focus to the viewport so the
    // spacebar can still be used to activate selection helpers.
    if (CViewport* view = GetIEditor()->GetViewManager()->GetGameViewport())
    {
        view->SetFocus();
    }
}

void SandboxIntegrationManager::OnEntityPickModeStopped()
{
    m_inObjectPickMode = false;
}

void SandboxIntegrationManager::CreateEditorRepresentation(AZ::Entity* entity)
{
    IEditor* editor = GetIEditor();

    if (CComponentEntityObject* existingObject = CComponentEntityObject::FindObjectForEntity(entity->GetId()))
    {
        // Refresh sandbox object if one already exists for this AZ::EntityId.
        existingObject->AssignEntity(entity, false);
        return;
    }

    CBaseObjectPtr object = editor->NewObject("ComponentEntity", "", entity->GetName().c_str(), 0.f, 0.f, 0.f, false);

    if (object)
    {
        static_cast<CComponentEntityObject*>(object.get())->AssignEntity(entity);

        // If this object type was hidden by category, re-display it.
        int hideMask = editor->GetDisplaySettings()->GetObjectHideMask();
        hideMask = hideMask & ~(object->GetType());
        editor->GetDisplaySettings()->SetObjectHideMask(hideMask);

        // Enable display of current layer.
        CObjectLayer* layer = editor->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
        if (layer)
        {
            layer->SetFrozen(false);
            layer->SetVisible(true);
            layer->SetModified();
        }
    }
}

bool SandboxIntegrationManager::DestroyEditorRepresentation(AZ::EntityId entityId, bool deleteAZEntity)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

    IEditor* editor = GetIEditor();
    if (editor->GetObjectManager())
    {
        CEntityObject* object = nullptr;
        EBUS_EVENT_ID_RESULT(object, entityId, AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);

        if (object && (object->GetType() == OBJTYPE_AZENTITY))
        {
            static_cast<CComponentEntityObject*>(object)->AssignEntity(nullptr, deleteAZEntity);
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "SandboxIntegrationManager::DestroyEditorRepresentation:ObjManagerDeleteObject");
                editor->GetObjectManager()->DeleteObject(object);
            }
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void SandboxIntegrationManager::GoToSelectedOrHighlightedEntitiesInViewports()
{
    AzToolsFramework::EntityIdList entityIds;
    GetSelectedOrHighlightedEntities(entityIds);
    GoToEntitiesInViewports(entityIds);
}

//////////////////////////////////////////////////////////////////////////
void SandboxIntegrationManager::GoToSelectedEntitiesInViewports()
{
    AzToolsFramework::EntityIdList entityIds;
    GetSelectedEntities(entityIds);
    GoToEntitiesInViewports(entityIds);
}

bool SandboxIntegrationManager::CanGoToSelectedEntitiesInViewports()
{
    AzToolsFramework::EntityIdList entityIds;
    GetSelectedEntities(entityIds);

    if (entityIds.size() == 0)
    {
        return false;
    }

    for (const AZ::EntityId& entityId : entityIds)
    {
        if (CanGoToEntityOrChildren(entityId))
        {
            return true;
        }
    }

    return false;
}

bool SandboxIntegrationManager::CanGoToEntityOrChildren(const AZ::EntityId& entityId) const
{
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
    // If this entity is not a layer, then the camera can go to it.
    if (!isLayerEntity)
    {
        return true;
    }

    AZStd::vector<AZ::EntityId> layerChildren;
    AZ::TransformBus::EventResult(
        /*result*/ layerChildren,
        /*address*/ entityId,
        &AZ::TransformBus::Events::GetChildren);

    for (const AZ::EntityId& childId : layerChildren)
    {
        if (CanGoToEntityOrChildren(childId))
        {
            return true;
        }
    }
    return false;
}

AZ::Vector3 SandboxIntegrationManager::GetWorldPositionAtViewportCenter()
{
    if (GetIEditor() && GetIEditor()->GetViewManager())
    {
        CViewport* view = GetIEditor()->GetViewManager()->GetGameViewport();
        if (view)
        {
            int width, height;
            view->GetDimensions(&width, &height);
            return LYVec3ToAZVec3(view->ViewToWorld(QPoint(width / 2, height / 2)));
        }
    }

    return AZ::Vector3::CreateZero();
}

int SandboxIntegrationManager::GetIconTextureIdFromEntityIconPath(const AZStd::string& entityIconPath)
{
    return GetIEditor()->GetIconManager()->GetIconTexture(entityIconPath.c_str());
}

void SandboxIntegrationManager::ClearRedoStack()
{
    // We have two separate undo systems that are assumed to be kept in sync,
    // So here we tell the legacy Undo system to clear the redo stack and at the same time
    // tell the new AZ undo system to clear redo stack ("slice" the stack)

    // Clear legacy redo stack
    GetIEditor()->ClearRedoStack();

    // Clear AZ redo stack
    AzToolsFramework::UndoSystem::UndoStack* undoStack = nullptr;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(undoStack, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetUndoStack);
    if (undoStack)
    {
        undoStack->Slice();
    }
}

//////////////////////////////////////////////////////////////////////////

void SandboxIntegrationManager::CloneSelection(bool& handled)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

    AzToolsFramework::EntityIdSet duplicationSet;
    GetSelectedEntitiesSetWithFlattenedHierarchy(duplicationSet);

    if (duplicationSet.size() > 0)
    {
        AZStd::unordered_set<AZ::EntityId> clonedEntities;
        handled = AzToolsFramework::CloneInstantiatedEntities(duplicationSet, clonedEntities);
        m_unsavedEntities.insert(clonedEntities.begin(), clonedEntities.end());
    }
    else
    {
        handled = false;
    }
}

void SandboxIntegrationManager::DeleteSelectedEntities(const bool includeDescendants)
{
    if (IsNewViewportInteractionModelEnabled())
    {
        AzToolsFramework::EntityIdList selectedEntityIds;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::DeleteEntitiesAndAllDescendants, selectedEntityIds);
    }
    else
    {
        CCryEditApp::instance()->DeleteSelectedEntities(includeDescendants);
    }
}

AZ::EntityId SandboxIntegrationManager::CreateNewEntity(AZ::EntityId parentId)
{
    AZ::Vector3 position = AZ::Vector3::CreateZero();
    if (!parentId.IsValid())
    {
        position = GetWorldPositionAtViewportCenter();
    }
    return CreateNewEntityAtPosition(position, parentId);
}

AZ::EntityId SandboxIntegrationManager::CreateNewEntityAsChild(AZ::EntityId parentId)
{
    AZ_Assert(parentId.IsValid(), "Entity created as a child of an invalid parent entity.");
    auto newEntityId = CreateNewEntity(parentId);

    // Some modules need to know that this special action has taken place; broadcast an event.
    ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::EntityCreatedAsChild, newEntityId, parentId);
    return newEntityId;
}

AZ::EntityId SandboxIntegrationManager::CreateNewEntityAtPosition(const AZ::Vector3& pos, AZ::EntityId parentId)
{
    AzToolsFramework::ScopedUndoBatch undo("New Entity");

    AZ::Entity* newEntity = nullptr;

    const AZStd::string name = AZStd::string::format("Entity%d", GetIEditor()->GetObjectManager()->GetObjectCount() + 1);

    EBUS_EVENT_RESULT(newEntity, AzToolsFramework::EditorEntityContextRequestBus, CreateEditorEntity, name.c_str());

    if (newEntity)
    {
        m_unsavedEntities.insert(newEntity->GetId());
        EBUS_EVENT(AzToolsFramework::EditorMetricsEventsBus, EntityCreated, newEntity->GetId());

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        transform.SetPosition(pos);
        if (parentId.IsValid())
        {
            EBUS_EVENT_ID(newEntity->GetId(), AZ::TransformBus, SetParent, parentId);
            EBUS_EVENT_ID(newEntity->GetId(), AZ::TransformBus, SetLocalTM, transform);
        }
        else
        {
            EBUS_EVENT_ID(newEntity->GetId(), AZ::TransformBus, SetWorldTM, transform);
        }

        // Select the new entity (and deselect others).
        AzToolsFramework::EntityIdList selection = { newEntity->GetId() };

        auto selectionCommand =
            AZStd::make_unique<AzToolsFramework::SelectionCommand>(selection, "");
        selectionCommand->SetParent(undo.GetUndoBatch());
        selectionCommand.release();

        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetSelectedEntities, selection);
    }

    if (newEntity)
    {
        return newEntity->GetId();
    }

    return AZ::EntityId();
}

AzFramework::EntityContextId SandboxIntegrationManager::GetEntityContextId()
{
    AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

    return editorEntityContextId;
}

//////////////////////////////////////////////////////////////////////////
QWidget* SandboxIntegrationManager::GetMainWindow()
{
    return MainWindow::instance();
}

IEditor* SandboxIntegrationManager::GetEditor()
{
    return GetIEditor();
}

//////////////////////////////////////////////////////////////////////////
bool SandboxIntegrationManager::GetUndoSliceOverrideSaveValue()
{
    return GetIEditor()->GetEditorSettings()->m_undoSliceOverrideSaveValue;
}

//////////////////////////////////////////////////////////////////////////
bool SandboxIntegrationManager::GetShowCircularDependencyError()
{
    return GetIEditor()->GetEditorSettings()->m_showCircularDependencyError;
}

void SandboxIntegrationManager::SetShowCircularDependencyError(const bool& showCircularDependencyError)
{
    GetIEditor()->GetEditorSettings()->m_showCircularDependencyError = showCircularDependencyError;
}

//////////////////////////////////////////////////////////////////////////
void SandboxIntegrationManager::SetEditTool(const char* tool)
{
    GetIEditor()->SetEditTool(tool);
}

void SandboxIntegrationManager::LaunchLuaEditor(const char* files)
{
    GetIEditor()->ExecuteCommand(QStringLiteral("general.launch_lua_editor \'%1\'").arg(files));
}

bool SandboxIntegrationManager::IsLevelDocumentOpen()
{
    return (GetIEditor() && GetIEditor()->GetDocument() && GetIEditor()->GetDocument()->IsDocumentReady());
}

AZStd::string SandboxIntegrationManager::GetLevelName()
{
    return AZStd::string(GetIEditor()->GetGameEngine()->GetLevelName().toUtf8().constData());
}

AZStd::string SandboxIntegrationManager::SelectResource(const AZStd::string& resourceType, const AZStd::string& previousValue)
{
    SResourceSelectorContext context;
    context.parentWidget = GetMainWindow();
    context.typeName = resourceType.c_str();

    QString resource = GetEditor()->GetResourceSelectorHost()->SelectResource(context, previousValue.c_str());
    return AZStd::string(resource.toUtf8().constData());
}

void SandboxIntegrationManager::GenerateNavigationArea(const AZStd::string& name, const AZ::Vector3& position, const AZ::Vector3* points, size_t numPoints, float height)
{
    IEditor* editor = GetIEditor();
    AZ_Assert(editor, "Editor is null when calling SandboxIntegrationManager::GenerateNavigationArea()");

    CShapeObject* object = static_cast<CShapeObject*>(editor->NewObject("NavigationArea", "", name.c_str(), position.GetX(), position.GetY(), position.GetZ()));
    AZ_Assert(object, "Failed to create object of type NavigationArea");

    if (object)
    {
        // This is a little weird. The position you set upon creation of the object is reset to the first point you add
        // therefore all future points should be relative to the first, rather than the position.
        // External need not worry about this madness because we fix it here
        if (numPoints >= 1)
        {
            object->InsertPoint(-1, AZVec3ToLYVec3(points[0]), false);
        }

        for (size_t i = 1; i < numPoints; ++i)
        {
            object->InsertPoint(-1, AZVec3ToLYVec3(points[i] - points[0]), false);
        }

        // Update the height
        if (CVarBlock* cvarblock = object->GetVarBlock())
        {
            if (IVariable* heightvar = cvarblock->FindVariable("Height"))
            {
                heightvar->Set(height);
            }
        }

        // make sure the user can see the navigation area in the Editor, and it gets updated
        // We turn this on here because this is not guaranteed to be on by default and first time
        // users will not know to turn it on.
        CAIManager* aiManager = editor->GetAI();
        AZ_Assert(aiManager, "AI Manager is null when calling SandboxIntegrationManager::GenerateNavigationArea()");

        if (!aiManager->GetNavigationDebugDisplayState())
        {
            CCryEditApp::instance()->OnAINavigationDisplayAgent();
        }

        if (!aiManager->GetNavigationContinuousUpdateState())
        {
            CCryEditApp::instance()->OnAINavigationEnableContinuousUpdate();
        }

        if (!aiManager->GetShowNavigationAreasState())
        {
            CCryEditApp::instance()->OnAINavigationShowAreas();
        }
    }
}

const char* SandboxIntegrationManager::GetDefaultAgentNavigationTypeName()
{
    CAIManager* manager = GetIEditor()->GetAI();

    const size_t agentTypeCount = manager->GetNavigationAgentTypeCount();
    if (agentTypeCount > 0)
    {
        return manager->GetNavigationAgentTypeName(0);
    }

    return "";
}

float SandboxIntegrationManager::CalculateAgentNavigationRadius(const char* agentTypeName)
{
    Vec3 voxelSize;
    uint16 radiusInVoxelUnits;
    if (gEnv->pAISystem->GetNavigationSystem()->TryGetAgentRadiusData(agentTypeName, voxelSize, radiusInVoxelUnits))
    {
        return AZStd::max<f32>(voxelSize.x, voxelSize.y) * radiusInVoxelUnits;
    }
    return -1.0f;
}

AZStd::vector<AZStd::string> SandboxIntegrationManager::GetAgentTypes()
{
    CAIManager* manager = GetIEditor()->GetAI();

    AZStd::vector<AZStd::string> agentTypes;
    size_t agentTypeCount = manager->GetNavigationAgentTypeCount();
    for (size_t i = 0; i < agentTypeCount; i++)
    {
        agentTypes.push_back(manager->GetNavigationAgentTypeName(i));
    }

    return agentTypes;
}

void SandboxIntegrationManager::OnContextReset()
{
    // Deselect everything.
    EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetSelectedEntities, AzToolsFramework::EntityIdList());

    std::vector<CBaseObject*> objects;
    objects.reserve(128);
    IObjectManager* objectManager = GetIEditor()->GetObjectManager();
    objectManager->FindObjectsOfType(OBJTYPE_AZENTITY, objects);
    for (CBaseObject* object : objects)
    {
        CComponentEntityObject* componentEntity = static_cast<CComponentEntityObject*>(object);
        componentEntity->AssignEntity(nullptr, false);
        objectManager->DeleteObject(componentEntity);
    }
}

void SandboxIntegrationManager::OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& /*ticket*/)
{
    // The instantiated slice isn't valid. Other systems will report this as an error.
    // Bail out here, this is nothing to track in this case.
    if (!sliceAddress.GetInstance())
    {
        return;
    }

    const AZ::SliceComponent::EntityIdToEntityIdMap& sliceInstanceEntityIdMap = sliceAddress.GetInstance()->GetEntityIdMap();

    for (const auto& sliceInstantEntityIdPair : sliceInstanceEntityIdMap)
    {
        // The second in the pair is the local instance's entity ID.
        m_unsavedEntities.insert(sliceInstantEntityIdPair.second);
    }

}

AZStd::string SandboxIntegrationManager::GetHyperGraphName(IFlowGraph* runtimeGraphPtr)
{
    CHyperGraph* hyperGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(runtimeGraphPtr);
    if (hyperGraph)
    {
        return hyperGraph->GetName().toUtf8().data();
    }

    return AZStd::string();
}

void SandboxIntegrationManager::RegisterHyperGraphEntityListener(IFlowGraph* runtimeGraphPtr, IEntityObjectListener* listener)
{
    CFlowGraph* flowGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(runtimeGraphPtr);
    if (flowGraph && flowGraph->GetEntity())
    {
        flowGraph->GetEntity()->RegisterListener(listener);
    }
}

void SandboxIntegrationManager::UnregisterHyperGraphEntityListener(IFlowGraph* runtimeGraphPtr, IEntityObjectListener* listener)
{
    CFlowGraph* flowGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(runtimeGraphPtr);
    if (flowGraph && flowGraph->GetEntity())
    {
        flowGraph->GetEntity()->UnregisterListener(listener);
    }
}

void SandboxIntegrationManager::SetHyperGraphEntity(IFlowGraph* runtimeGraphPtr, const AZ::EntityId& id)
{
    CFlowGraph* flowGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(runtimeGraphPtr);
    if (flowGraph)
    {
        flowGraph->SetEntity(id);
    }
}

void SandboxIntegrationManager::OpenHyperGraphView(IFlowGraph* runtimeGraphPtr)
{
    CFlowGraph* flowGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(runtimeGraphPtr);
    if (flowGraph)
    {
        GetIEditor()->GetFlowGraphManager()->OpenView(flowGraph);
    }
}

void SandboxIntegrationManager::ReleaseHyperGraph(IFlowGraph* runtimeGraphPtr)
{
    CFlowGraph* flowGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(runtimeGraphPtr);
    SAFE_RELEASE(flowGraph);
}

void SandboxIntegrationManager::SetHyperGraphGroupName(IFlowGraph* runtimeGraphPtr, const char* name)
{
    CFlowGraph* flowGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(runtimeGraphPtr);
    if (flowGraph)
    {
        flowGraph->SetGroupName(name);
    }
}

void SandboxIntegrationManager::SetHyperGraphName(IFlowGraph* runtimeGraphPtr, const char* name)
{
    CFlowGraph* flowGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(runtimeGraphPtr);
    if (flowGraph)
    {
        flowGraph->SetName(name);
    }
}

void SandboxIntegrationManager::ContextMenu_NewEntity()
{
    // Navigation triggered - Right Click in ViewPort
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu);

    AZ::Vector3 worldPosition = AZ::Vector3::CreateZero();

    CViewport* view = GetIEditor()->GetViewManager()->GetGameViewport();
    // If we don't have a viewport active to aid in placement, the object
    // will be created at the origin.
    if (view)
    {
        const QPoint viewPoint(m_contextMenuViewPoint.GetX(), m_contextMenuViewPoint.GetY());
        worldPosition = LYVec3ToAZVec3(view->SnapToGrid(view->ViewToWorld(viewPoint)));
    }

    CreateNewEntityAtPosition(worldPosition);
}

AZ::EntityId SandboxIntegrationManager::ContextMenu_NewLayer()
{
    const int objectCount = GetIEditor()->GetObjectManager()->GetObjectCount();
    const AZStd::string name = AZStd::string::format("Layer%d", objectCount + 1);

    // Make sure the color is created fully opaque.
    static QColor newLayerDefaultColor = GetIEditor()->GetColorByName("NewLayerDefaultColor");
    AZ::Color newLayerColor(
        AZ::VectorFloat(newLayerDefaultColor.redF()),
        AZ::VectorFloat(newLayerDefaultColor.greenF()),
        AZ::VectorFloat(newLayerDefaultColor.blueF()),
        AZ::VectorFloat(newLayerDefaultColor.alphaF()));

    AZ::Entity* newEntity = AzToolsFramework::Layers::EditorLayerComponent::CreateLayerEntity(name, newLayerColor);
    if (newEntity == nullptr)
    {
        // CreateLayerEntity already handled reporting errors if it couldn't make a new layer.
        return AZ::EntityId();
    }
    m_unsavedEntities.insert(newEntity->GetId());
    return newEntity->GetId();
}

void SandboxIntegrationManager::ContextMenu_SaveLayers(const AZStd::unordered_set<AZ::EntityId>& layers)
{
    AZStd::unordered_map<AZStd::string, int> nameConflictMapping;
    for (AZ::EntityId layerEntityId : layers)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            layerEntityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::UpdateLayerNameConflictMapping,
            nameConflictMapping);
    }

    if (!nameConflictMapping.empty())
    {
        AzToolsFramework::Layers::NameConflictWarning* nameConflictWarning = new AzToolsFramework::Layers::NameConflictWarning(GetMainWindow(), nameConflictMapping);
        nameConflictWarning->exec();
        return;
    }

    AZStd::unordered_set<AZ::EntityId> allLayersToSave(layers);
    bool mustSaveLevel = false;
    bool mustSaveOtherContent = false;

    for (AZ::EntityId layerEntityId : layers)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            mustSaveOtherContent,
            layerEntityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GatherSaveDependencies,
            allLayersToSave,
            mustSaveLevel);
    }

    if (mustSaveOtherContent)
    {
        QWidget* mainWindow = nullptr;
        AzToolsFramework::EditorRequestBus::BroadcastResult(
            mainWindow,
            &AzToolsFramework::EditorRequestBus::Events::GetMainWindow);
        QMessageBox saveAdditionalContentMessage(mainWindow);
        saveAdditionalContentMessage.setWindowTitle(QObject::tr("Unsaved content"));
        saveAdditionalContentMessage.setText(QObject::tr("You have moved entities to or from the layer(s) that you are trying to save."));
        if (mustSaveLevel)
        {
            saveAdditionalContentMessage.setInformativeText(QObject::tr("The level and all layers will be saved."));

        }
        else
        {
            saveAdditionalContentMessage.setInformativeText(QObject::tr("All relevant layers will be saved."));
        }
        saveAdditionalContentMessage.setIcon(QMessageBox::Warning);
        saveAdditionalContentMessage.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        saveAdditionalContentMessage.setDefaultButton(QMessageBox::Save);
        int saveAdditionalContentMessageResult = saveAdditionalContentMessage.exec();
        switch (saveAdditionalContentMessageResult)
        {
        case QMessageBox::Cancel:
            // The user chose to cancel this operation.
            return;
        case QMessageBox::Save:
        default:
            break;
        }

        if (mustSaveLevel)
        {
            // Saving the level causes all layers to save.
            GetIEditor()->GetDocument()->Save();
            return;
        }
    }

    QString levelAbsoluteFolder = Path::GetPath(GetIEditor()->GetDocument()->GetActivePathName());

    // Not used here, but needed for the ebus event.
    AZStd::vector<AZ::Entity*> layerEntities;
    AZ::SliceComponent::SliceReferenceToInstancePtrs instancesInLayers;
    for (AZ::EntityId layerEntityId : allLayersToSave)
    {
        AzToolsFramework::Layers::LayerResult layerSaveResult(AzToolsFramework::Layers::LayerResult::CreateSuccess());
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerSaveResult,
            layerEntityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::WriteLayerAndGetEntities,
            levelAbsoluteFolder,
            layerEntities,
            instancesInLayers);

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            layerEntityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);
        layerSaveResult.MessageResult();

        m_unsavedEntities.erase(layerEntityId);
    }

    // Update the unsaved entities list so these entities are no longer tracked as unsaved.
    for (const AZ::Entity* entity : layerEntities)
    {
        m_unsavedEntities.erase(entity->GetId());
    }
}

void SandboxIntegrationManager::ContextMenu_MakeSlice(AzToolsFramework::EntityIdList entities)
{
    MakeSliceFromEntities(entities, false, GetIEditor()->GetEditorSettings()->sliceSettings.dynamicByDefault);
}

void SandboxIntegrationManager::ContextMenu_InheritSlice(AzToolsFramework::EntityIdList entities)
{
    MakeSliceFromEntities(entities, true, GetIEditor()->GetEditorSettings()->sliceSettings.dynamicByDefault);
}
void SandboxIntegrationManager::ContextMenu_InstantiateSlice()
{
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Slice");
    BrowseForAssets(selection);

    if (selection.IsValid())
    {
        auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
        AZ_Assert(product, "Incorrect entry type selected. Expected product.");
     
        InstantiateSliceFromAssetId(product->GetAssetId());
    }
}

void SandboxIntegrationManager::InstantiateSliceFromAssetId(const AZ::Data::AssetId& assetId)
{
    AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
    sliceAsset.Create(assetId, true);

    AZ::Transform sliceWorldTransform = AZ::Transform::CreateIdentity();

    CViewport* view = GetIEditor()->GetViewManager()->GetGameViewport();
    // If we don't have a viewport active to aid in placement, the slice
    // will be instantiated at the origin.
    if (view)
    {
        const QPoint viewPoint(m_contextMenuViewPoint.GetX(), m_contextMenuViewPoint.GetY());
        sliceWorldTransform = AZ::Transform::CreateTranslation(LYVec3ToAZVec3(view->SnapToGrid(view->ViewToWorld(viewPoint))));
    }

    EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, InstantiateEditorSlice, sliceAsset, sliceWorldTransform);

    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu);

    AZStd::string idString;
    assetId.ToString(idString);

    AzToolsFramework::EditorMetricsEventsBus::Broadcast(&AzToolsFramework::EditorMetricsEventsBusTraits::SliceInstantiated, AZ::Crc32(idString.c_str()));
}

//////////////////////////////////////////////////////////////////////////
// Returns true if at least one non-layer entity was found.
bool CollectEntityBoundingBoxesForZoom(const AZ::EntityId& entityId, AABB& selectionBounds)
{
    bool isLayerEntity = false;
    AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
        isLayerEntity,
        entityId,
        &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);

    if (isLayerEntity)
    {
        // If a layer is in the selection, zoom to its children and ignore the layer itself.
        AZStd::vector<AZ::EntityId> layerChildren;
        AZ::TransformBus::EventResult(
            /*result*/ layerChildren,
            /*address*/ entityId,
            &AZ::TransformBus::Events::GetChildren);
        bool childResults = false;
        for (const AZ::EntityId& childId : layerChildren)
        {
            if (CollectEntityBoundingBoxesForZoom(childId, selectionBounds))
            {
                // At least one child is not a layer.
                childResults = true;
            }
        }
        return childResults;
    }
    else
    {
        AABB entityBoundingBox;
        CEntityObject* componentEntityObject = nullptr;
        AzToolsFramework::ComponentEntityEditorRequestBus::EventResult(
            /*result*/ componentEntityObject,
            /*address*/ entityId,
            &AzToolsFramework::ComponentEntityEditorRequestBus::Events::GetSandboxObject);

        if (componentEntityObject)
        {
            componentEntityObject->GetBoundBox(entityBoundingBox);
            selectionBounds.Add(entityBoundingBox.min);
            selectionBounds.Add(entityBoundingBox.max);
        }
        return true;
    }
}

//////////////////////////////////////////////////////////////////////////
void SandboxIntegrationManager::GoToEntitiesInViewports(const AzToolsFramework::EntityIdList& entityIds)
{
    if (entityIds.size() == 0)
    {
        return;
    }

    AABB selectionBounds;
    selectionBounds.Reset();
    bool entitiesAvailableForGoTo = false;

    for (const AZ::EntityId& entityId : entityIds)
    {
        if(CollectEntityBoundingBoxesForZoom(entityId, selectionBounds))
        {
            entitiesAvailableForGoTo = true;
        }
    }

    if (entitiesAvailableForGoTo)
    {
        int numViews = GetIEditor()->GetViewManager()->GetViewCount();
        for (int viewIndex = 0; viewIndex < numViews; ++viewIndex)
        {
            CViewport* viewport = GetIEditor()->GetViewManager()->GetView(viewIndex);
            if (viewport)
            {
                viewport->CenterOnAABB(selectionBounds);
            }
        }
    }
}

void SandboxIntegrationManager::BuildSerializedFlowGraph(IFlowGraph* flowGraph, LmbrCentral::SerializedFlowGraph& graphData)
{
    using namespace LmbrCentral;

    graphData = SerializedFlowGraph();

    if (!flowGraph)
    {
        return;
    }

    CFlowGraph* hyperGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(flowGraph);
    if (!hyperGraph)
    {
        return;
    }

    graphData.m_name = hyperGraph->GetName().toUtf8().constData();
    graphData.m_description = hyperGraph->GetDescription().toUtf8().constData();
    graphData.m_group = hyperGraph->GetGroupName().toUtf8().constData();
    graphData.m_isEnabled = hyperGraph->IsEnabled();
    graphData.m_persistentId = AZ::Crc32(graphData.m_name.c_str());
    graphData.m_hypergraphId = hyperGraph->GetHyperGraphId();

    switch (hyperGraph->GetMPType())
    {
    case CFlowGraph::eMPT_ServerOnly:
    {
        graphData.m_networkType = FlowGraphNetworkType::ServerOnly;
    }
    break;
    case CFlowGraph::eMPT_ClientOnly:
    {
        graphData.m_networkType = FlowGraphNetworkType::ClientOnly;
    }
    break;
    case CFlowGraph::eMPT_ClientServer:
    {
        graphData.m_networkType = FlowGraphNetworkType::ServerOnly;
    }
    break;
    }

    IHyperGraphEnumerator* nodeIter = hyperGraph->GetNodesEnumerator();
    for (IHyperNode* hyperNodeInterface = nodeIter->GetFirst(); hyperNodeInterface; hyperNodeInterface = nodeIter->GetNext())
    {
        CHyperNode* hyperNode = static_cast<CHyperNode*>(hyperNodeInterface);

        graphData.m_nodes.push_back();
        SerializedFlowGraph::Node& nodeData = graphData.m_nodes.back();

        nodeData.m_name = hyperNode->GetName().toUtf8().constData();
        nodeData.m_class = hyperNode->GetClassName().toUtf8().constData();
        nodeData.m_position.Set(hyperNode->GetPos().x(), hyperNode->GetPos().y());
        nodeData.m_flags = hyperNode->GetFlags();

        const QRectF& sizeRect(hyperNode->GetRect());
        nodeData.m_size.Set(sizeRect.right() - sizeRect.left(), sizeRect.bottom() - sizeRect.top());

        const QRectF* borderRect = hyperNode->GetResizeBorderRect();
        if (borderRect)
        {
            nodeData.m_borderRect.Set(borderRect->right() - borderRect->left(), borderRect->bottom() - borderRect->top());
        }

        const HyperNodeID nodeId = hyperNode->GetId();
        const HyperNodeID flowNodeId = hyperNode->GetFlowNodeId();
        IFlowNodeData* flowData = flowNodeId != InvalidFlowNodeId ? flowGraph->GetNodeData(flowNodeId) : nullptr;

        nodeData.m_id = nodeId;
        nodeData.m_isGraphEntity = hyperNode->CheckFlag(EHYPER_NODE_GRAPH_ENTITY);
        nodeData.m_entityId = flowData ? AZ::EntityId(flowData->GetEntityId().GetId()) : AZ::EntityId();
        if (static_cast<AZ::u64>(nodeData.m_entityId) == 0)
        {
            nodeData.m_entityId.SetInvalid();
        }

        const CHyperNode::Ports* inputPorts = hyperNode->GetInputs();
        if (inputPorts)
        {
            for (size_t inputIndex = 0, inputCount = inputPorts->size(); inputIndex < inputCount; ++inputIndex)
            {
                const CHyperNodePort& port = (*inputPorts)[inputIndex];

                if (!port.bVisible)
                {
                    nodeData.m_inputHideMask |= (1 << inputIndex);
                }

                if (port.pVar)
                {
                    const IVariable::EType type = port.pVar->GetType();
                    switch (type)
                    {
                    case IVariable::UNKNOWN:
                    case IVariable::FLOW_CUSTOM_DATA:
                    {
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Unknown,
                            aznew SerializedFlowGraph::InputValueVoid());
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::INT:
                    {
                        auto* value = aznew SerializedFlowGraph::InputValueInt();
                        port.pVar->Get(value->m_value);
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Int, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::BOOL:
                    {
                        auto* value = aznew SerializedFlowGraph::InputValueBool();
                        port.pVar->Get(value->m_value);
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Bool, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::FLOAT:
                    {
                        auto* value = aznew SerializedFlowGraph::InputValueFloat();
                        port.pVar->Get(value->m_value);
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Float, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::VECTOR2:
                    {
                        Vec2 temp;
                        port.pVar->Get(temp);
                        auto* value = aznew SerializedFlowGraph::InputValueVec2(temp);
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Vector2, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::VECTOR:
                    {
                        Vec3 temp;
                        port.pVar->Get(temp);
                        auto* value = aznew SerializedFlowGraph::InputValueVec3(temp);
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Vector3, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::VECTOR4:
                    {
                        Vec4 temp;
                        port.pVar->Get(temp);
                        auto* value = aznew SerializedFlowGraph::InputValueVec4(temp);
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Vector4, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::QUAT:
                    {
                        Quat temp;
                        port.pVar->Get(temp);
                        auto* value = aznew SerializedFlowGraph::InputValueQuat(temp);
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Quat, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::STRING:
                    {
                        auto* value = aznew SerializedFlowGraph::InputValueString();
                        QString temp;
                        port.pVar->Get(temp);
                        value->m_value = temp.toUtf8().data();
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::String, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    case IVariable::DOUBLE:
                    {
                        auto* value = aznew SerializedFlowGraph::InputValueDouble();
                        port.pVar->Get(value->m_value);
                        nodeData.m_inputs.emplace_back(port.GetName().toUtf8().data(), port.GetHumanName().toUtf8().data(), FlowVariableType::Double, value);
                        nodeData.m_inputs.back().m_persistentId = AZ::Crc32(port.GetName().toUtf8().data());
                    }
                    break;
                    }
                }
            }
        }

        const CHyperNode::Ports* outputPorts = hyperNode->GetOutputs();
        if (outputPorts)
        {
            for (size_t outputIndex = 0, outputCount = outputPorts->size(); outputIndex < outputCount; ++outputIndex)
            {
                const CHyperNodePort& port = (*outputPorts)[outputIndex];

                if (!port.bVisible)
                {
                    nodeData.m_outputHideMask |= (1 << outputIndex);
                }
            }
        }
    }

    std::vector<CHyperEdge*> edges;
    edges.reserve(4096);
    hyperGraph->GetAllEdges(edges);

    for (CHyperEdge* edge : edges)
    {
        graphData.m_edges.push_back();
        SerializedFlowGraph::Edge& edgeData = graphData.m_edges.back();

        edgeData.m_nodeIn = edge->nodeIn;
        edgeData.m_nodeOut = edge->nodeOut;
        edgeData.m_portIn = edge->portIn.toUtf8().constData();
        edgeData.m_portOut = edge->portOut.toUtf8().constData();
        edgeData.m_isEnabled = edge->enabled;

        AZ::Crc32 hash;
        hash.Add(&edgeData.m_nodeIn, sizeof(edgeData.m_nodeIn));
        hash.Add(&edgeData.m_nodeOut, sizeof(edgeData.m_nodeIn));
        hash.Add(edgeData.m_portIn.c_str());
        hash.Add(edgeData.m_portOut.c_str());
        edgeData.m_persistentId = hash;
    }

    edges.resize(0);

    for (size_t tokenIndex = 0, tokenCount = flowGraph->GetGraphTokenCount(); tokenIndex < tokenCount; ++tokenIndex)
    {
        graphData.m_graphTokens.push_back();
        SerializedFlowGraph::GraphToken& tokenData = graphData.m_graphTokens.back();

        const IFlowGraph::SGraphToken* token = flowGraph->GetGraphToken(tokenIndex);
        AZ_Assert(token, "Failed to retrieve graph token at index %zu", tokenIndex);
        tokenData.m_name = token->name.c_str();
        tokenData.m_type = token->type;
        tokenData.m_persistentId = AZ::Crc32(tokenData.m_name.c_str());
    }
}

void SandboxIntegrationManager::ContextMenu_SelectSlice()
{
    AzToolsFramework::EntityIdList selectedEntities;
    GetSelectedOrHighlightedEntities(selectedEntities);

    AzToolsFramework::EntityIdList newSelectedEntities;

    for (const AZ::EntityId& entityId : selectedEntities)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        EBUS_EVENT_ID_RESULT(sliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);

        if (sliceAddress.IsValid())
        {
            const AZ::SliceComponent::InstantiatedContainer* instantiated = sliceAddress.GetInstance()->GetInstantiated();

            if (instantiated)
            {
                for (AZ::Entity* entityInSlice : instantiated->m_entities)
                {
                    newSelectedEntities.push_back(entityInSlice->GetId());
                }
            }
        }
    }

    EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus,
        SetSelectedEntities, newSelectedEntities);
}

void SandboxIntegrationManager::ContextMenu_PushEntitiesToSlice(AzToolsFramework::EntityIdList entities,
    AZ::SliceComponent::EntityAncestorList ancestors,
    AZ::Data::AssetId targetAncestorId,
    bool affectEntireHierarchy)
{
    (void)ancestors;
    (void)targetAncestorId;
    (void)affectEntireHierarchy;

    AZ::SerializeContext* serializeContext = NULL;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(serializeContext, "No serialize context");

    AzToolsFramework::SliceUtilities::PushEntitiesModal(GetMainWindow(), entities, serializeContext);
}

void SandboxIntegrationManager::ContextMenu_Duplicate()
{
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::RightClickMenu);

    bool handled = true;
    AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::CloneSelection, handled);
}

void SandboxIntegrationManager::ContextMenu_DeleteSelected()
{
    DeleteSelectedEntities(true);
}

void SandboxIntegrationManager::ContextMenu_ResetToSliceDefaults(AzToolsFramework::EntityIdList entities)
{
    AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequests::ResetEntitiesToSliceDefaults, entities);
}

bool SandboxIntegrationManager::CreateFlowGraphNameDialog(AZ::EntityId entityId, AZStd::string& flowGraphName)
{
    AZ::Entity* entity = nullptr;
    EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

    if (entity)
    {
        QString title = QString("Enter Flow Graph Name (") + QString(entity->GetName().c_str()) + QString(")");

        CFlowGraphNewDlg newFlowGraphDialog(title, "Default");
        if (newFlowGraphDialog.exec() == QDialog::Accepted)
        {
            flowGraphName = newFlowGraphDialog.GetText().toStdString().c_str();
            return true;
        }
    }

    return false;
}

void SandboxIntegrationManager::ContextMenu_NewFlowGraph(AzToolsFramework::EntityIdList entities)
{
    // This is the Uuid of the EditorFlowGraphComponent.
    // #TODO LY-21846: Add "FlowGraphService" to entity, rather than specific component-type.
    AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entities, AZ::ComponentTypeList{ AZ::Uuid("{400972DE-DD1F-4407-8F53-7E514C5767CA}") });

    for (auto entityId : entities)
    {
        ContextMenu_AddFlowGraph(entityId);
    }
}

void SandboxIntegrationManager::ContextMenu_OpenFlowGraph(AZ::EntityId entityId, IFlowGraph* flowgraph)
{
    // Launch FG editor with specified flowgraph selected.
    EBUS_EVENT_ID(FlowEntityId(entityId), FlowGraphEditorRequestsBus, OpenFlowGraphView, flowgraph);
}

void SandboxIntegrationManager::ContextMenu_RemoveFlowGraph(AZ::EntityId entityId, IFlowGraph* flowgraph)
{
    AzToolsFramework::ScopedUndoBatch undo("Remove Flow Graph");

    EBUS_EVENT_ID(FlowEntityId(entityId), FlowGraphEditorRequestsBus, RemoveFlowGraph, flowgraph, true);
}

void SandboxIntegrationManager::ContextMenu_AddFlowGraph(AZ::EntityId entityId)
{
    AZStd::string flowGraphName;
    if (CreateFlowGraphNameDialog(entityId, flowGraphName))
    {
        const AZStd::string undoName = AZStd::string::format("Add Flow Graph: %s", flowGraphName.c_str());
        AzToolsFramework::ScopedUndoBatch undo(undoName.c_str());

        IFlowGraph* flowGraph = nullptr;
        EBUS_EVENT_ID_RESULT(flowGraph, FlowEntityId(entityId), FlowGraphEditorRequestsBus, AddFlowGraph, flowGraphName);
        ContextMenu_OpenFlowGraph(entityId, flowGraph);
    }
}

void SandboxIntegrationManager::GetSelectedEntities(AzToolsFramework::EntityIdList& entities)
{
    EBUS_EVENT_RESULT(entities,
        AzToolsFramework::ToolsApplicationRequests::Bus,
        GetSelectedEntities);
}

void SandboxIntegrationManager::GetSelectedOrHighlightedEntities(AzToolsFramework::EntityIdList& entities)
{
    AzToolsFramework::EntityIdList selectedEntities;
    AzToolsFramework::EntityIdList highlightedEntities;

    EBUS_EVENT_RESULT(selectedEntities,
        AzToolsFramework::ToolsApplicationRequests::Bus,
        GetSelectedEntities);

    EBUS_EVENT_RESULT(highlightedEntities,
        AzToolsFramework::ToolsApplicationRequests::Bus,
        GetHighlightedEntities);

    entities = AZStd::move(selectedEntities);

    for (AZ::EntityId highlightedId : highlightedEntities)
    {
        if (entities.end() == AZStd::find(entities.begin(), entities.end(), highlightedId))
        {
            entities.push_back(highlightedId);
        }
    }
}

AZStd::string SandboxIntegrationManager::GetComponentEditorIcon(const AZ::Uuid& componentType, AZ::Component* component)
{
    AZStd::string iconPath = GetComponentIconPath(componentType, AZ::Edit::Attributes::Icon, component);
    return iconPath;
}

AZStd::string SandboxIntegrationManager::GetComponentIconPath(const AZ::Uuid& componentType,
    AZ::Crc32 componentIconAttrib, AZ::Component* component)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
    if (componentIconAttrib != AZ::Edit::Attributes::Icon
        && componentIconAttrib != AZ::Edit::Attributes::ViewportIcon
        && componentIconAttrib != AZ::Edit::Attributes::HideIcon)
    {
        AZ_Warning("SandboxIntegration", false, "Unrecognized component icon attribute!");
    }

    // return blank path if component shouldn't have icon at all
    AZStd::string iconPath;

    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(serializeContext, "No serialize context");

    auto classData = serializeContext->FindClassData(componentType);
    if (classData && classData->m_editData)
    {
        // check if component icon should be hidden
        bool hideIcon = false;

        auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
        if (editorElementData)
        {
            if (auto hideIconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::HideIcon))
            {
                auto hideIconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<bool>*>(hideIconAttribute);
                if (hideIconAttributeData)
                {
                    hideIcon = hideIconAttributeData->Get(nullptr);
                }
            }
        }

        if (!hideIcon)
        {
            // component should have icon. start with default
            iconPath = GetDefaultComponentEditorIcon();

            // check for specific icon
            if (editorElementData)
            {
                if (auto iconAttribute = editorElementData->FindAttribute(componentIconAttrib))
                {
                    if (auto iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute))
                    {
                        AZStd::string iconAttributeValue = iconAttributeData->Get(component);
                        if (!iconAttributeValue.empty())
                        {
                            iconPath = AZStd::move(iconAttributeValue);
                        }
                    }

                    auto iconOverrideAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::DynamicIconOverride);

                    // If it has an override and we're given an instance, then get any potential override from the instance here
                    if (component &&
                        (componentIconAttrib == AZ::Edit::Attributes::Icon || componentIconAttrib == AZ::Edit::Attributes::ViewportIcon) &&
                        iconOverrideAttribute)
                    {
                        AZStd::string iconValue;
                        AZ::AttributeReader iconReader(const_cast<AZ::Component*>(component), iconOverrideAttribute);
                        iconReader.Read<AZStd::string>(iconValue);

                        if (!iconValue.empty())
                        {
                            iconPath = AZStd::move(iconValue);
                        }
                    }
                }
            }

            // use absolute path if possible
            AZStd::string iconFullPath;
            bool pathFound = false;
            using AssetSysReqBus = AzToolsFramework::AssetSystemRequestBus;
            AssetSysReqBus::BroadcastResult(pathFound, &AssetSysReqBus::Events::GetFullSourcePathFromRelativeProductPath, iconPath, iconFullPath);
            if (pathFound)
            {
                iconPath = AZStd::move(iconFullPath);
            }
        }
    }

    return iconPath;
}

void SandboxIntegrationManager::UndoStackFlushed()
{
    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::FlushUndo);
}

void SandboxIntegrationManager::MakeSliceFromEntities(const AzToolsFramework::EntityIdList& entities, bool inheritSlices, bool setAsDynamic)
{
    // expand the list of entities to include all transform descendant entities
    AzToolsFramework::EntityIdSet entitiesAndDescendants;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(entitiesAndDescendants,
        &AzToolsFramework::ToolsApplicationRequestBus::Events::GatherEntitiesAndAllDescendents, entities);

    const AZStd::string slicesAssetsPath = "@devassets@/Slices";

    if (!gEnv->pFileIO->Exists(slicesAssetsPath.c_str()))
    {
        gEnv->pFileIO->CreatePath(slicesAssetsPath.c_str());
    }

    char path[AZ_MAX_PATH_LEN] = { 0 };
    gEnv->pFileIO->ResolvePath(slicesAssetsPath.c_str(), path, AZ_MAX_PATH_LEN);
    AzToolsFramework::SliceUtilities::MakeNewSlice(entitiesAndDescendants, path, inheritSlices, setAsDynamic);
}

void SandboxIntegrationManager::SetupFileExtensionMap()
{
    // There's no central registry for geometry file types.
    const char* geometryFileExtensions[] =
    {
        CRY_GEOMETRY_FILE_EXT,                  // .cgf
        CRY_SKEL_FILE_EXT,                      // .chr
        CRY_CHARACTER_DEFINITION_FILE_EXT,      // .cdf
    };

    // Cry geometry file extensions.
    for (const char* extension : geometryFileExtensions)
    {
        m_extensionToFileType[AZ::Crc32(extension)] = IFileUtil::EFILE_TYPE_GEOMETRY;
    }

    // Cry image file extensions.
    for (size_t i = 0; i < IResourceCompilerHelper::GetNumSourceImageFormats(); ++i)
    {
        const char* extension = IResourceCompilerHelper::GetSourceImageFormat(i, false);
        m_extensionToFileType[AZ::Crc32(extension)] = IFileUtil::EFILE_TYPE_TEXTURE;
    }
}

void SandboxIntegrationManager::RegisterViewPane(const char* name, const char* category, const AzToolsFramework::ViewPaneOptions& viewOptions, const WidgetCreationFunc& widgetCreationFunc)
{
    QtViewPaneManager::instance()->RegisterPane(name, category, widgetCreationFunc, viewOptions);
}

void SandboxIntegrationManager::UnregisterViewPane(const char* name)
{
    QtViewPaneManager::instance()->UnregisterPane(name);
}

QWidget* SandboxIntegrationManager::GetViewPaneWidget(const char* viewPaneName)
{
    return FindViewPane<QWidget>(viewPaneName);
}

void SandboxIntegrationManager::OpenViewPane(const char* paneName)
{
    const QtViewPane* pane = QtViewPaneManager::instance()->OpenPane(paneName);
    if (pane)
    {
        pane->m_dockWidget->raise();
        pane->m_dockWidget->activateWindow();
    }
}

QDockWidget* SandboxIntegrationManager::InstanceViewPane(const char* paneName)
{
    return QtViewPaneManager::instance()->InstancePane(paneName);
}

void SandboxIntegrationManager::CloseViewPane(const char* paneName)
{
    QtViewPaneManager::instance()->ClosePane(paneName);
}

void SandboxIntegrationManager::BrowseForAssets(AssetSelectionModel& selection)
{
    auto dialog = aznew AzAssetBrowserDialog(selection, GetMainWindow());
    dialog->exec();
    delete dialog;
}

void SandboxIntegrationManager::GenerateCubemapForEntity(AZ::EntityId entityId, AZStd::string* cubemapOutputPath, bool hideEntity)
{
    AZ::u32 resolution = 0;
    EBUS_EVENT_ID_RESULT(resolution, entityId, LmbrCentral::EditorLightComponentRequestBus, GetCubemapResolution);

    if (resolution > 0)
    {
        CComponentEntityObject* componentEntity = CComponentEntityObject::FindObjectForEntity(entityId);

        if (componentEntity)
        {
            QString levelfolder = GetIEditor()->GetGameEngine()->GetLevelPath();
            QString levelname = Path::GetFile(levelfolder).toLower();
            QString fullGameFolder = QString(Path::GetEditingGameDataFolder().c_str());
            QString texturename = QStringLiteral("%1_cm.tif").arg(static_cast<qulonglong>(componentEntity->GetAssociatedEntityId()));
            texturename = texturename.toLower();

            QString fullFolder = Path::SubDirectoryCaseInsensitive(fullGameFolder, {"textures", "cubemaps", levelname});
            QString fullFilename = QDir(fullFolder).absoluteFilePath(texturename);
            QString relFilename = QDir(fullGameFolder).relativeFilePath(fullFilename);

            bool directlyExists = CFileUtil::CreateDirectory(fullFolder.toUtf8().data());
            if (!directlyExists)
            {
                QMessageBox::warning(GetMainWindow(), QObject::tr("Cubemap Generation Failed"), QString(QObject::tr("Failed to create destination path '%1'")).arg(fullFolder));
                return;
            }

            if (CubemapUtils::GenCubemapWithObjectPathAndSize(fullFilename, componentEntity, static_cast<int>(resolution), hideEntity))
            {
                AZStd::string assetPath = relFilename.toUtf8().data();
                AzFramework::StringFunc::Path::ReplaceExtension(assetPath, ".dds");

                EBUS_EVENT_ID(entityId, LmbrCentral::EditorLightComponentRequestBus, SetCubemap, assetPath);

                if (cubemapOutputPath)
                {
                    *cubemapOutputPath = AZStd::move(assetPath);
                }
            }
            else
            {
                QMessageBox::warning(GetMainWindow(), QObject::tr("Cubemap Generation Failed"), QObject::tr("Unspecified error"));
            }
        }
    }
}

void SandboxIntegrationManager::GenerateAllCubemaps()
{
    AZStd::string cubemapOutputPath;

    std::vector<CBaseObject*> results;
    results.reserve(128);
    GetIEditor()->GetObjectManager()->FindObjectsOfType(OBJTYPE_AZENTITY, results);
    for (std::vector<CBaseObject*>::iterator end = results.end(), item = results.begin(); item != end; ++item)
    {
        CComponentEntityObject* componentEntity = static_cast<CComponentEntityObject*>(*item);

        //check if it's customized cubemap, only generate it if it's not.
        bool isCustomizedCubemap = true;
        EBUS_EVENT_ID_RESULT(isCustomizedCubemap, componentEntity->GetAssociatedEntityId(), LmbrCentral::EditorLightComponentRequestBus, UseCustomizedCubemap);

        if (isCustomizedCubemap)
        {
            continue;
        }

        GenerateCubemapForEntity(componentEntity->GetAssociatedEntityId(), nullptr, true);
    }
}

void SandboxIntegrationManager::SetColor(float r, float g, float b, float a)
{
    if (m_dc)
    {
        m_dc->SetColor(Vec3(r, g, b), a);
    }
}

void SandboxIntegrationManager::SetColor(const AZ::Color& color)
{
    if (m_dc)
    {
        m_dc->SetColor(AZColorToLYColorF(color));
    }
}

void SandboxIntegrationManager::SetColor(const AZ::Vector4& color)
{
    if (m_dc)
    {
        m_dc->SetColor(AZVec3ToLYVec3(color.GetAsVector3()), color.GetW());
    }
}

void SandboxIntegrationManager::SetAlpha(float a)
{
    if (m_dc)
    {
        m_dc->SetAlpha(a);
    }
}

void SandboxIntegrationManager::DrawQuad(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4)
{
    if (m_dc)
    {
        m_dc->DrawQuad(
            AZVec3ToLYVec3(p1),
            AZVec3ToLYVec3(p2),
            AZVec3ToLYVec3(p3),
            AZVec3ToLYVec3(p4));
    }
}

void SandboxIntegrationManager::DrawQuadGradient(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, const AZ::Vector3& p4, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor)
{
    if (m_dc)
    {
        m_dc->DrawQuadGradient(
            AZVec3ToLYVec3(p1),
            AZVec3ToLYVec3(p2),
            AZVec3ToLYVec3(p3),
            AZVec3ToLYVec3(p4),
            ColorF(AZVec3ToLYVec3(firstColor.GetAsVector3()), firstColor.GetW()),
            ColorF(AZVec3ToLYVec3(secondColor.GetAsVector3()), secondColor.GetW()));
    }
}

void SandboxIntegrationManager::DrawTri(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3)
{
    if (m_dc)
    {
        m_dc->DrawTri(
            AZVec3ToLYVec3(p1),
            AZVec3ToLYVec3(p2),
            AZVec3ToLYVec3(p3));
    }
}

void SandboxIntegrationManager::DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, const AZ::Color& color)
{
    if (m_dc)
    {
        // transform to world space
        const auto vecTransform = [this](const AZ::Vector3& vec)
        {
            return m_dc->GetMatrix() * AZVec3ToLYVec3(vec);
        };

        AZStd::vector<Vec3> cryVertices;
        cryVertices.reserve(vertices.size());
        AZStd::transform(vertices.begin(), vertices.end(), AZStd::back_inserter(cryVertices), vecTransform);
        m_dc->DrawTriangles(
            cryVertices,
            AZColorToLYColorF(color));
    }
}

void SandboxIntegrationManager::DrawTrianglesIndexed(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Color& color)
{
    if (m_dc)
    {
        // transform to world space
        const auto vecTransform = [this](const AZ::Vector3& vec)
        {
            return m_dc->GetMatrix() * AZVec3ToLYVec3(vec);
        };

        AZStd::vector<Vec3> cryVertices;
        cryVertices.reserve(vertices.size());
        AZStd::transform(vertices.begin(), vertices.end(), AZStd::back_inserter(cryVertices), vecTransform);
        m_dc->DrawTrianglesIndexed(
            cryVertices,
            indices,
            AZColorToLYColorF(color));
    }
}

void SandboxIntegrationManager::DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max)
{
    if (m_dc)
    {
        m_dc->DrawWireBox(
            AZVec3ToLYVec3(min),
            AZVec3ToLYVec3(max));
    }
}

void SandboxIntegrationManager::DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max)
{
    if (m_dc)
    {
        m_dc->DrawSolidBox(
            AZVec3ToLYVec3(min),
            AZVec3ToLYVec3(max));
    }
}

void SandboxIntegrationManager::DrawSolidOBB(const AZ::Vector3& center, const AZ::Vector3& axisX, const AZ::Vector3& axisY, const AZ::Vector3& axisZ, const AZ::Vector3& halfExtents)
{
    if (m_dc)
    {
        m_dc->DrawSolidOBB(AZVec3ToLYVec3(center), AZVec3ToLYVec3(axisX), AZVec3ToLYVec3(axisY), AZVec3ToLYVec3(axisZ), AZVec3ToLYVec3(halfExtents));
    }
}

void SandboxIntegrationManager::DrawPoint(const AZ::Vector3& p, int nSize)
{
    if (m_dc)
    {
        m_dc->DrawPoint(AZVec3ToLYVec3(p), nSize);
    }
}

void SandboxIntegrationManager::DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2)
{
    if (m_dc)
    {
        m_dc->DrawLine(
            AZVec3ToLYVec3(p1),
            AZVec3ToLYVec3(p2));
    }
}

void SandboxIntegrationManager::DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector4& col1, const AZ::Vector4& col2)
{
    if (m_dc)
    {
        m_dc->DrawLine(
            AZVec3ToLYVec3(p1),
            AZVec3ToLYVec3(p2),
            ColorF(AZVec3ToLYVec3(col1.GetAsVector3()), col1.GetW()),
            ColorF(AZVec3ToLYVec3(col2.GetAsVector3()), col2.GetW()));
    }
}

void SandboxIntegrationManager::DrawLines(const AZStd::vector<AZ::Vector3>& lines, const AZ::Color& color)
{
    if (m_dc)
    {
        // transform to world space
        const auto vecTransform = [this](const AZ::Vector3& vec)
        {
            return m_dc->GetMatrix() * AZVec3ToLYVec3(vec);
        };

        AZStd::vector<Vec3> cryLines;
        cryLines.reserve(cryLines.size());
        AZStd::transform(lines.begin(), lines.end(), AZStd::back_inserter(cryLines), vecTransform);
        m_dc->DrawLines(cryLines, AZColorToLYColorF(color));
    }
}

void SandboxIntegrationManager::DrawPolyLine(const AZ::Vector3* pnts, int numPoints, bool cycled)
{
    if (m_dc)
    {
        Vec3* points = new Vec3[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = AZVec3ToLYVec3(pnts[i]);
        }

        m_dc->DrawPolyLine(points, numPoints, cycled);

        delete[] points;
    }
}

void SandboxIntegrationManager::DrawWireQuad2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z)
{
    if (m_dc)
    {
        m_dc->DrawWireQuad2d(
            QPoint(static_cast<int>(p1.GetX()), static_cast<int>(p1.GetY())),
            QPoint(static_cast<int>(p2.GetX()), static_cast<int>(p2.GetY())),
            z);
    }
}

void SandboxIntegrationManager::DrawLine2d(const AZ::Vector2& p1, const AZ::Vector2& p2, float z)
{
    if (m_dc)
    {
        m_dc->DrawLine2d(
            QPoint(static_cast<int>(p1.GetX()), static_cast<int>(p1.GetY())),
            QPoint(static_cast<int>(p2.GetX()), static_cast<int>(p2.GetY())),
            z);
    }
}

void SandboxIntegrationManager::DrawLine2dGradient(const AZ::Vector2& p1, const AZ::Vector2& p2, float z, const AZ::Vector4& firstColor, const AZ::Vector4& secondColor)
{
    if (m_dc)
    {
        m_dc->DrawLine2dGradient(
            QPoint(static_cast<int>(p1.GetX()), static_cast<int>(p1.GetY())),
            QPoint(static_cast<int>(p2.GetX()), static_cast<int>(p2.GetY())),
            z,
            ColorF(AZVec3ToLYVec3(firstColor.GetAsVector3()), firstColor.GetW()),
            ColorF(AZVec3ToLYVec3(secondColor.GetAsVector3()), secondColor.GetW()));
    }
}

void SandboxIntegrationManager::DrawWireCircle2d(const AZ::Vector2& center, float radius, float z)
{
    if (m_dc)
    {
        m_dc->DrawWireCircle2d(
            QPoint(static_cast<int>(center.GetX()), static_cast<int>(center.GetY())),
            radius, z);
    }
}

void SandboxIntegrationManager::DrawTerrainCircle(const AZ::Vector3& worldPos, float radius, float height)
{
    if (m_dc)
    {
        m_dc->DrawTerrainCircle(
            AZVec3ToLYVec3(worldPos), radius, height);
    }
}

void SandboxIntegrationManager::DrawTerrainCircle(const AZ::Vector3& center, float radius, float angle1, float angle2, float height)
{
    if (m_dc)
    {
        m_dc->DrawTerrainCircle(
            AZVec3ToLYVec3(center), radius, angle1, angle2, height);
    }
}

void SandboxIntegrationManager::DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, int referenceAxis)
{
    if (m_dc)
    {
        m_dc->DrawArc(
            AZVec3ToLYVec3(pos),
            radius,
            startAngleDegrees,
            sweepAngleDegrees,
            angularStepDegrees,
            referenceAxis);
    }
}

void SandboxIntegrationManager::DrawArc(const AZ::Vector3& pos, float radius, float startAngleDegrees, float sweepAngleDegrees, float angularStepDegrees, const AZ::Vector3& fixedAxis)
{
    if (m_dc)
    {
        m_dc->DrawArc(
            AZVec3ToLYVec3(pos),
            radius,
            startAngleDegrees,
            sweepAngleDegrees,
            angularStepDegrees,
            AZVec3ToLYVec3(fixedAxis));
    }
}

void SandboxIntegrationManager::DrawCircle(const AZ::Vector3& pos, float radius, int nUnchangedAxis)
{
    if (m_dc)
    {
        m_dc->DrawCircle(
            AZVec3ToLYVec3(pos),
            radius,
            nUnchangedAxis);
    }
}

void SandboxIntegrationManager::DrawHalfDottedCircle(const AZ::Vector3& pos, float radius, const AZ::Vector3& viewPos, int nUnchangedAxis)
{
    if (m_dc)
    {
        m_dc->DrawHalfDottedCircle(
            AZVec3ToLYVec3(pos),
            radius,
            AZVec3ToLYVec3(viewPos),
            nUnchangedAxis);
    }
}

void SandboxIntegrationManager::DrawCone(const AZ::Vector3& pos, const AZ::Vector3& dir, float radius, float height)
{
    if (m_dc)
    {
        m_dc->DrawCone(
            AZVec3ToLYVec3(pos),
            AZVec3ToLYVec3(dir),
            radius,
            height);
    }
}

void SandboxIntegrationManager::DrawWireCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height)
{
    if (m_dc)
    {
        m_dc->DrawWireCylinder(
            AZVec3ToLYVec3(center),
            AZVec3ToLYVec3(axis),
            radius,
            height);
    }
}

void SandboxIntegrationManager::DrawSolidCylinder(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height)
{
    if (m_dc)
    {
        m_dc->DrawSolidCylinder(
            AZVec3ToLYVec3(center),
            AZVec3ToLYVec3(axis),
            radius,
            height);
    }
}

void SandboxIntegrationManager::DrawWireCapsule(const AZ::Vector3& center, const AZ::Vector3& axis, float radius, float height)
{
    if (m_dc)
    {
        m_dc->DrawWireCapsule(
            AZVec3ToLYVec3(center),
            AZVec3ToLYVec3(axis),
            radius,
            height);
    }
}

void SandboxIntegrationManager::DrawTerrainRect(float x1, float y1, float x2, float y2, float height)
{
    if (m_dc)
    {
        m_dc->DrawTerrainRect(x1, y1, x2, y2, height);
    }
}

void SandboxIntegrationManager::DrawTerrainLine(AZ::Vector3 worldPos1, AZ::Vector3 worldPos2)
{
    if (m_dc)
    {
        m_dc->DrawTerrainLine(
            AZVec3ToLYVec3(worldPos1),
            AZVec3ToLYVec3(worldPos2));
    }
}

void SandboxIntegrationManager::DrawWireSphere(const AZ::Vector3& pos, float radius)
{
    if (m_dc)
    {
        m_dc->DrawWireSphere(AZVec3ToLYVec3(pos), radius);
    }
}

void SandboxIntegrationManager::DrawWireSphere(const AZ::Vector3& pos, const AZ::Vector3 radius)
{
    if (m_dc)
    {
        m_dc->DrawWireSphere(
            AZVec3ToLYVec3(pos),
            AZVec3ToLYVec3(radius));
    }
}

void SandboxIntegrationManager::DrawBall(const AZ::Vector3& pos, float radius)
{
    if (m_dc)
    {
        m_dc->DrawBall(AZVec3ToLYVec3(pos), radius);
    }
}

void SandboxIntegrationManager::DrawArrow(const AZ::Vector3& src, const AZ::Vector3& trg, float fHeadScale, bool b2SidedArrow)
{
    if (m_dc)
    {
        m_dc->DrawArrow(
            AZVec3ToLYVec3(src),
            AZVec3ToLYVec3(trg),
            fHeadScale,
            b2SidedArrow);
    }
}

void SandboxIntegrationManager::DrawTextLabel(const AZ::Vector3& pos, float size, const char* text, const bool bCenter, int srcOffsetX, int srcOffsetY)
{
    if (m_dc)
    {
        m_dc->DrawTextLabel(
            AZVec3ToLYVec3(pos),
            size,
            text,
            bCenter,
            srcOffsetX,
            srcOffsetY);
    }
}

void SandboxIntegrationManager::Draw2dTextLabel(float x, float y, float size, const char* text, bool bCenter)
{
    if (m_dc)
    {
        m_dc->Draw2dTextLabel(x, y, size, text, bCenter);
    }
}

void SandboxIntegrationManager::DrawTextOn2DBox(const AZ::Vector3& pos, const char* text, float textScale, const AZ::Vector4& textColor, const AZ::Vector4& textBackColor)
{
    if (m_dc)
    {
        m_dc->DrawTextOn2DBox(
            AZVec3ToLYVec3(pos),
            text,
            textScale,
            ColorF(AZVec3ToLYVec3(textColor.GetAsVector3()), textColor.GetW()),
            ColorF(AZVec3ToLYVec3(textBackColor.GetAsVector3()), textBackColor.GetW()));
    }
}

void SandboxIntegrationManager::DrawTextureLabel(ITexture* texture, const AZ::Vector3& pos, float sizeX, float sizeY, int texIconFlags)
{
    if (m_dc)
    {
        if (texture)
        {
            float textureWidth = aznumeric_caster(texture->GetWidth());
            float textureHeight = aznumeric_caster(texture->GetHeight());

            // resize the label in proportion to the actual texture size
            if (textureWidth > textureHeight)
            {
                sizeY = sizeX * (textureHeight / textureWidth);
            }
            else
            {
                sizeX = sizeY * (textureWidth / textureHeight);
            }

            m_dc->DrawTextureLabel(AZVec3ToLYVec3(pos), sizeX, sizeY, texture->GetTextureID(), texIconFlags);
        }
    }
}

void SandboxIntegrationManager::DrawTextureLabel(int textureId, const AZ::Vector3& pos, float sizeX, float sizeY, int texIconFlags)
{
    ITexture* texture = GetIEditor()->GetRenderer()->EF_GetTextureByID(textureId);
    DrawTextureLabel(texture, pos, sizeX, sizeY, texIconFlags);
}

void SandboxIntegrationManager::SetLineWidth(float width)
{
    if (m_dc)
    {
        m_dc->SetLineWidth(width);
    }
}

bool SandboxIntegrationManager::IsVisible(const AZ::Aabb& bounds)
{
    if (m_dc)
    {
        const AABB aabb(
            AZVec3ToLYVec3(bounds.GetMin()),
            AZVec3ToLYVec3(bounds.GetMax()));

        return m_dc->IsVisible(aabb);
    }

    return 0;
}

int SandboxIntegrationManager::SetFillMode(int nFillMode)
{
    if (m_dc)
    {
        return m_dc->SetFillMode(nFillMode);
    }

    return 0;
}

float SandboxIntegrationManager::GetLineWidth()
{
    if (m_dc)
    {
        return m_dc->GetLineWidth();
    }

    return 0.f;
}

float SandboxIntegrationManager::GetAspectRatio()
{
    if (m_dc && m_dc->GetView())
    {
        return m_dc->GetView()->GetAspectRatio();
    }

    return 0.f;
}

void SandboxIntegrationManager::DepthTestOff()
{
    if (m_dc)
    {
        m_dc->DepthTestOff();
    }
}

void SandboxIntegrationManager::DepthTestOn()
{
    if (m_dc)
    {
        m_dc->DepthTestOn();
    }
}

void SandboxIntegrationManager::DepthWriteOff()
{
    if (m_dc)
    {
        m_dc->DepthWriteOff();
    }
}

void SandboxIntegrationManager::DepthWriteOn()
{
    if (m_dc)
    {
        m_dc->DepthWriteOn();
    }
}

void SandboxIntegrationManager::CullOff()
{
    if (m_dc)
    {
        m_dc->CullOff();
    }
}

void SandboxIntegrationManager::CullOn()
{
    if (m_dc)
    {
        m_dc->CullOn();
    }
}

bool SandboxIntegrationManager::SetDrawInFrontMode(bool bOn)
{
    if (m_dc)
    {
        return m_dc->SetDrawInFrontMode(bOn);
    }

    return 0.f;
}

AZ::u32 SandboxIntegrationManager::GetState()
{
    if (m_dc)
    {
        return m_dc->GetState();
    }

    return 0;
}

AZ::u32 SandboxIntegrationManager::SetState(AZ::u32 state)
{
    if (m_dc)
    {
        return m_dc->SetState(state);
    }

    return 0;
}

AZ::u32 SandboxIntegrationManager::SetStateFlag(AZ::u32 state)
{
    if (m_dc)
    {
        return m_dc->SetStateFlag(state);
    }

    return 0;
}

AZ::u32 SandboxIntegrationManager::ClearStateFlag(AZ::u32 state)
{
    if (m_dc)
    {
        return m_dc->ClearStateFlag(state);
    }

    return 0;
}

void SandboxIntegrationManager::PushMatrix(const AZ::Transform& tm)
{
    if (m_dc)
    {
        const Matrix34 m = AZTransformToLYTransform(tm);
        m_dc->PushMatrix(m);
    }
}

void SandboxIntegrationManager::PopMatrix()
{
    if (m_dc)
    {
        m_dc->PopMatrix();
    }
}

AZ::Outcome<AZ::Entity*, AZ::LegacyConversion::CreateEntityResult> SandboxIntegrationManager::CreateConvertedEntity(CBaseObject* sourceObject, bool failIfParentNotFound, const AZ::ComponentTypeList& componentsToAdd)
{
    using namespace LmbrCentral;
    using namespace AzToolsFramework;

    AZStd::string entityName = sourceObject->GetName().toUtf8().data();
    if (entityName.empty())
    {
        entityName = AZStd::string::format("ConvertedLegacyEntity%d", GetIEditor()->GetObjectManager()->GetObjectCount());
    }

    CObjectLayer* layer = sourceObject->GetLayer();
    AZ::Uuid layerGUID;
    AZStd::string layerName;
    layerGUID.CreateNull();
    if (layer)
    {
        layerGUID = layer->GetGUID();
        layerName = layer->GetName().toUtf8().data();
    }

    CBaseObject* parentEntity = sourceObject->GetParent();
    AZ::EntityId parentEntityId;
    parentEntityId.SetInvalid();

    if (parentEntity)
    {
        // we might already be parented to a Component Entity
        AZStd::string parentClass(parentEntity->GetClassDesc()->ClassName().toUtf8().data());
        if (parentClass == "ComponentEntity")
        {
            // the parent is already a component entity.
            CComponentEntityObject* entityObject = static_cast<CComponentEntityObject*>(parentEntity);
            parentEntityId = entityObject->GetAssociatedEntityId();
        }
        else
        {
            parentEntityId = FindCreatedEntity(parentEntity->GetId(), parentEntity->GetName().toUtf8().data());
        }
    }

    if (!parentEntityId.IsValid())
    {
        if ((parentEntity) && (failIfParentNotFound))
        {
            AZ_TracePrintf("Legacy Conversion", "Entity '%s' has a parent '%s' (Type: '%s') which cannot be converted, so it will be retained as a Legacy CryEntity\n",
                sourceObject->GetName().toUtf8().data(),
                parentEntity->GetName().toUtf8().data(),
                parentEntity->GetClassDesc()->ClassName().toUtf8().data()
            );
            return AZ::Failure(AZ::LegacyConversion::CreateEntityResult::FailedNoParent);
        }
        // if it has no parent, (or we could not find one) then we see if there's a parent created for the layer it belongs to, and if so, parent it to the layer entity instead
        if (!layerGUID.IsNull())
        {
            // check to see if the Layer Entity is already created.
            parentEntityId = FindCreatedEntity(layerGUID, layerName.c_str());
        }
        if (!parentEntityId.IsValid())
        {
            // we need to create one that "represents" the layer...
            AZ::Entity* layerEntity = nullptr;
            EBUS_EVENT_RESULT(layerEntity, AzToolsFramework::EditorEntityContextRequestBus, CreateEditorEntity, (layerName + "_layer").c_str());
            if (layerEntity)
            {
                // deactivate the entity in order to add components:
                if (layerEntity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    layerEntity->Deactivate();
                }

                layerEntity->CreateComponent("{5272B56C-6CCC-4118-8539-D881F463ACD1}"); // add the tag component

                layerEntity->Activate();

                AZStd::string conversionGUIDTag = AZStd::string::format("Original CryEntity ID: %s", layerGUID.ToString<AZStd::string>().c_str());
                AZStd::string conversionNameTag = AZStd::string::format("Original CryEntity Name: %s", layerName.c_str());

                // add the layer tags.
                // First, a tag which indicates that this entity comes from a converted layer, not an entity:
                EditorTagComponentRequestBus::Event(layerEntity->GetId(), &EditorTagComponentRequests::AddTag, "ConvertedLayer");
                EditorTagComponentRequestBus::Event(layerEntity->GetId(), &EditorTagComponentRequests::AddTag, conversionGUIDTag.c_str());
                EditorTagComponentRequestBus::Event(layerEntity->GetId(), &EditorTagComponentRequests::AddTag, conversionNameTag.c_str());
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, layerEntity->GetId());

                parentEntityId = layerEntity->GetId();
            }
        }
    }

    AZ::Entity* newEntity = nullptr;
    EBUS_EVENT_RESULT(newEntity, AzToolsFramework::EditorEntityContextRequestBus, CreateEditorEntity, entityName.c_str());

    if (!newEntity)
    {
        AZ_Error("SandboxIntegration", false, "Failed to create a new entity during legacy conversion.");
        return AZ::Failure(AZ::LegacyConversion::CreateEntityResult::Failed);
    }
    m_unsavedEntities.insert(newEntity->GetId());

    // deactivate the entity in order to add components:
    if (newEntity->GetState() == AZ::Entity::ES_ACTIVE)
    {
        newEntity->Deactivate(); // must happen in order to talk to its transform component.
    }

    AZ::ComponentTypeList actualComponentsToAdd = componentsToAdd;
    actualComponentsToAdd.push_back("{5272B56C-6CCC-4118-8539-D881F463ACD1}");  // add the tag component

    EntityIdList entityList;
    EntityCompositionRequests::AddComponentsOutcome outcome = AZ::Failure(AZStd::string("Failed to call AddComponentsToEntities on EntityCompositionRequestBus"));
    entityList.push_back(newEntity->GetId());
    EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::AddComponentsToEntities, entityList, actualComponentsToAdd);
    if (!outcome.IsSuccess())
    {
        AZ_Error("Legacy Conversion", false, "Could not add component: %s", outcome.GetError().c_str());
        return AZ::Failure(AZ::LegacyConversion::CreateEntityResult::FailedInvalidComponent);
    }

    if (newEntity->GetState() != AZ::Entity::ES_ACTIVE)
    {
        newEntity->Activate(); // must happen in order to talk to its transform component.
    }

    if (parentEntityId.IsValid())
    {
        EBUS_EVENT_ID(newEntity->GetId(), AZ::TransformBus, SetParent, parentEntityId);
    }

    // Move entity to same transform.
    const AZ::Transform transform = LYTransformToAZTransform(sourceObject->GetWorldTM());
    EBUS_EVENT_ID(newEntity->GetId(), AZ::TransformBus, SetWorldTM, transform);

    // add the tags.  we basically tag as much information as we can just in case there's something we can do later to convert even more data across.
    // there's no point in losing the information we have if we can preserve it.
    AZ::Uuid sourceObjectId = sourceObject->GetId();
    AZStd::string entityIdTag = AZStd::string::format("Original CryEntity ID: %s", sourceObjectId.ToString<AZStd::string>().c_str());
    AZStd::string entityNameTag = AZStd::string::format("Original CryEntity Name: %s", sourceObject->GetName().toUtf8().data());
    AZStd::string originalLayerNameTag = AZStd::string::format("Original Layer Name: %s", layerName.c_str());
    AZStd::string originalLayerUuidTag = AZStd::string::format("Original Layer ID: %s", layerGUID.ToString<AZStd::string>().c_str());
    AZStd::string originalClassNameTag = AZStd::string::format("Original CryEntity ClassName: %s", sourceObject->GetClassDesc()->ClassName().toUtf8().data());

    EditorTagComponentRequestBus::Event(newEntity->GetId(), &EditorTagComponentRequests::AddTag, "ConvertedEntity");
    EditorTagComponentRequestBus::Event(newEntity->GetId(), &EditorTagComponentRequests::AddTag, entityIdTag.c_str());
    EditorTagComponentRequestBus::Event(newEntity->GetId(), &EditorTagComponentRequests::AddTag, entityNameTag.c_str());
    EditorTagComponentRequestBus::Event(newEntity->GetId(), &EditorTagComponentRequests::AddTag, originalLayerNameTag.c_str());
    EditorTagComponentRequestBus::Event(newEntity->GetId(), &EditorTagComponentRequests::AddTag, originalLayerUuidTag.c_str());
    EditorTagComponentRequestBus::Event(newEntity->GetId(), &EditorTagComponentRequests::AddTag, originalClassNameTag.c_str());

    return AZ::Success(newEntity);
}

AZ::EntityId SandboxIntegrationManager::FindCreatedEntity(const AZ::Uuid& sourceObjectUUID, const char* sourceObjectName)
{
    // these are 100% programmer error, not data error
    AZ_Assert(!sourceObjectUUID.IsNull(), "NULL sourceObjectUUID passed in to FindCreatedEntityByExistingObject - Programmer error!");
    AZ_Assert(sourceObjectName, "NULL sourceObjectName passed in to FindCreatedEntityByExistingObject - Programmer error!");

    using namespace LmbrCentral;
    AZStd::string entityIdTag = AZStd::string::format("Original CryEntity ID: %s", sourceObjectUUID.ToString<AZStd::string>().c_str());
    AZStd::string entityNameTag = AZStd::string::format("Original CryEntity Name: %s", sourceObjectName);

    AZ::EBusAggregateResults<AZ::EntityId> resultSet;
    TagGlobalRequestBus::EventResult(resultSet, Tag(entityIdTag.c_str()), &TagGlobalRequests::RequestTaggedEntities);

    // note that this result set is actually very, very small, usually its exactly one entity (or zero)
    // in the case of a CRC collision, it may be more than 1, but thats a very low probability.
    // Just to make 100% sure its not a CRC collision, we'll ask for the EXACT string.
    for (const AZ::EntityId& id1 : resultSet.values)
    {
        bool hasExactMatch = false;
        EditorTagComponentRequestBus::EventResult(hasExactMatch, id1, &EditorTagComponentRequests::HasTag, entityIdTag.c_str());
        if (hasExactMatch)
        {
            hasExactMatch = false;
            EditorTagComponentRequestBus::EventResult(hasExactMatch, id1, &EditorTagComponentRequests::HasTag, entityNameTag.c_str());
            if (hasExactMatch)
            {
                // absolute exact match, both name and uuid
                return id1;
            }
        }
    }

    return AZ::EntityId();
}

AZ::EntityId SandboxIntegrationManager::FindCreatedEntityByExistingObject(const CBaseObject* sourceObject)
{
    // these are 100% programmer error, not data error
    AZ_Assert(sourceObject, "NULL SourceObject passed in to FindCreatedEntityByExistingObject - Programmer error!");

    AZ::Uuid parentEntityId = sourceObject->GetId();
    return FindCreatedEntity(parentEntityId, sourceObject->GetName().toUtf8().data());
}

bool SandboxIntegrationManager::IsNewViewportInteractionModelEnabled()
{
    return GetIEditor()->IsNewViewportInteractionModelEnabled();
}

bool SandboxIntegrationManager::DisplayHelpersVisible()
{
    return GetIEditor()->GetDisplaySettings()->IsDisplayHelpers();
}