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

#include "precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/EBus/Results.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>

#include "SystemComponent.h"

#include <Editor/View/Windows/MainWindow.h>
#include <Editor/View/Dialogs/NewGraphDialog.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/Metrics.h>
#include <Editor/Settings.h>

#include <Libraries/Libraries.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Assets/ScriptCanvasAssetHolder.h>
#include <Editor/Assets/ScriptCanvasAssetReference.h>
#include <Editor/Assets/ScriptCanvasAssetInstance.h>
#include <Editor/Nodes/EditorLibrary.h>
#include <Editor/Undo/ScriptCanvasUndoCache.h>

#include <LyViewPaneNames.h>

#include <QMenu>

#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>

namespace ScriptCanvasEditor
{
    static const size_t cs_jobThreads = 1;

    SystemComponent::SystemComponent()
    {
    }

    SystemComponent::~SystemComponent()
    {
        AzToolsFramework::UnregisterViewPane(GetViewPaneName());
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ScriptCanvasData::Reflect(context);
        ScriptCanvasAssetReference::Reflect(context);
        ScriptCanvasAssetInstance::Reflect(context);
        ScriptCanvasAssetHolder::Reflect(context);
        EditorSettings::WindowSavedState::Reflect(context);
        EditorSettings::PreviewSettings::Reflect(context);
        Library::Editor::Reflect(context);
        UndoData::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            ScriptCanvasEditor::Settings::Reflect(serialize);

            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("Script Canvas Editor", "Script Canvas Editor System Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        // Base Mime Event
        CreateNodeMimeEvent::Reflect(context);
        SpecializedCreateNodeMimeEvent::Reflect(context);

        // Specific Mime Event Implementations
        CreateClassMethodMimeEvent::Reflect(context);
        CreateCommentNodeMimeEvent::Reflect(context);
        CreateCustomNodeMimeEvent::Reflect(context);
        CreateEBusHandlerMimeEvent::Reflect(context);
        CreateEBusHandlerEventMimeEvent::Reflect(context);
        CreateEBusSenderMimeEvent::Reflect(context);
        CreateEntityRefNodeMimeEvent::Reflect(context);
        CreateGetVariableNodeMimeEvent::Reflect(context);
        CreateSetVariableNodeMimeEvent::Reflect(context);
        CreateVariablePrimitiveNodeMimeEvent::Reflect(context);
        CreateVariableObjectNodeMimeEvent::Reflect(context);
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasEditorService", 0x4fe2af98));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        required.push_back(AZ_CRC("GraphCanvasService", 0x138a9c46));
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SystemComponent::Init()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        AZ::JobManagerDesc jobDesc;
        for (size_t i = 0; i < cs_jobThreads; ++i)
        {
            jobDesc.m_workerThreads.push_back({ static_cast<int>(i) });
        }
        m_jobManager = AZStd::make_unique<AZ::JobManager>(jobDesc);
        m_jobContext = AZStd::make_unique<AZ::JobContext>(*m_jobManager);
    }

    void SystemComponent::Activate()
    {
        SystemRequestBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        m_documentContext.Activate();
    }

    void SystemComponent::NotifyRegisterViews()
    {
        QtViewOptions options;
        options.canHaveMultipleInstances = false;
        options.isPreview = true;
        options.showInMenu = true;
        options.sendViewPaneNameBackToAmazonAnalyticsServers = true;

        AzToolsFramework::RegisterViewPane<ScriptCanvasEditor::MainWindow>(GetViewPaneName(), LyViewPane::CategoryTools, options);
    }

    void SystemComponent::Deactivate()
    {
        m_documentContext.Deactivate();

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();
    }

    void SystemComponent::AddAsyncJob(AZStd::function<void()>&& jobFunc)
    {
        auto* asyncFunction = AZ::CreateJobFunction(AZStd::move(jobFunc), true, m_jobContext.get());
        asyncFunction->Start();
    }

    void SystemComponent::PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags)
    {
        (void)point;
        (void)flags;

        AzToolsFramework::EntityIdList entitiesWithScriptCanvas;

        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::EntityIdList highlightedEntities;

        EBUS_EVENT_RESULT(selectedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetSelectedEntities);

        FilterForScriptCanvasEnabledEntities(selectedEntities, entitiesWithScriptCanvas);

        EBUS_EVENT_RESULT(highlightedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetHighlightedEntities);

        FilterForScriptCanvasEnabledEntities(highlightedEntities, entitiesWithScriptCanvas);

        if (!entitiesWithScriptCanvas.empty())
        {
            QMenu* scriptCanvasMenu = nullptr;
            QAction* action = nullptr;

            // For entities with script canvas component, create a context menu to open any existing script canvases within each selected entity.
            for (const AZ::EntityId& entityId : entitiesWithScriptCanvas)
            {
                if (!scriptCanvasMenu)
                {
                    menu->addSeparator();
                    scriptCanvasMenu = menu->addMenu(QObject::tr("Edit Script Canvas"));
                    menu->addSeparator();
                }

                AZ::Entity* entity = nullptr;
                EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

                if (entity)
                {
                    AZ::EBusAggregateResults<AZ::EntityId> graphIds;
                    EditorContextMenuRequestBus::EventResult(graphIds, entity->GetId(), &EditorContextMenuRequests::GetGraphId);

                    if (!graphIds.values.empty())
                    {
                        QMenu* entityMenu = scriptCanvasMenu;
                        if (entitiesWithScriptCanvas.size() > 1)
                        {
                            entityMenu = scriptCanvasMenu->addMenu(entity->GetName().c_str());
                        }

                        AZStd::unordered_set< AZ::EntityId > usedIds;

                        for (const auto& graphId : graphIds.values)
                        {
                            if (!graphId.IsValid() || usedIds.count(graphId) != 0)
                            {
                                continue;
                            }

                            usedIds.insert(graphId);

                            AZStd::string scriptName;
                            EditorScriptCanvasRequestBus::EventResult(scriptName, graphId, &EditorScriptCanvasRequests::GetName);
                            action = entityMenu->addAction(QString("%1").arg(QString(scriptName.c_str())));
                            QObject::connect(action, &QAction::triggered, [graphId] { EditorScriptCanvasRequestBus::Event(graphId, &EditorScriptCanvasRequests::OpenEditor); });
                        }
                    }
                }
            }
        }
    }

    void SystemComponent::FilterForScriptCanvasEnabledEntities(AzToolsFramework::EntityIdList& sourceList, AzToolsFramework::EntityIdList& targetList)
    {
        for (const AZ::EntityId& entityId : sourceList)
        {
            if (entityId.IsValid())
            {
                if (EditorContextMenuRequestBus::FindFirstHandler(entityId))
                {
                    if (targetList.end() == AZStd::find(targetList.begin(), targetList.end(), entityId))
                    {
                        targetList.push_back(entityId);
                    }
                }
            }
        }
    }
}