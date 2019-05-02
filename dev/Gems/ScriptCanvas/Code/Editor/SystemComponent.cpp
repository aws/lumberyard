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
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzCore/std/string/wildcard.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <GraphCanvas/GraphCanvasBus.h>

#include "SystemComponent.h"

#include <Editor/View/Windows/MainWindow.h>
#include <Editor/View/Dialogs/NewGraphDialog.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/Metrics.h>
#include <Editor/Settings.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>

#include <LyViewPaneNames.h>

#include <QMenu>

#include <ScriptCanvas/View/EditCtrls/GenericComboBoxCtrl.h>
#include <ScriptCanvas/View/EditCtrls/GenericLineEditCtrl.h>

namespace ScriptCanvasEditor
{
    static const size_t cs_jobThreads = 1;

    SystemComponent::SystemComponent()
    {
    }

    SystemComponent::~SystemComponent()
    {
        AzToolsFramework::UnregisterViewPane(LyViewPane::ScriptCanvas);
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
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
        required.push_back(AZ_CRC("MemoryService", 0x5c4d473c)); // AZ::JobManager needs the thread pool allocator
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        required.push_back(GraphCanvas::GraphCanvasRequestsServiceId);
        required.push_back(AZ_CRC("ScriptCanvasReflectService", 0xb3bfe139));
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SystemComponent::Init()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void SystemComponent::Activate()
    {
        AZ::JobManagerDesc jobDesc;
        for (size_t i = 0; i < cs_jobThreads; ++i)
        {
            jobDesc.m_workerThreads.push_back({ static_cast<int>(i) });
        }
        m_jobManager = AZStd::make_unique<AZ::JobManager>(jobDesc);
        m_jobContext = AZStd::make_unique<AZ::JobContext>(*m_jobManager);

        PopulateEditorCreatableTypes();

        m_propertyHandlers.emplace_back(RegisterGenericComboBoxHandler<ScriptCanvas::VariableId>());
        m_propertyHandlers.emplace_back(RegisterGenericComboBoxHandler<AZ::Crc32>());
        SystemRequestBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        m_documentContext.Activate();
    }

    void SystemComponent::NotifyRegisterViews()
    {
        QtViewOptions options;
        options.canHaveMultipleInstances = false;
        options.isPreview = true;
        options.showInMenu = true;
        options.sendViewPaneNameBackToAmazonAnalyticsServers = true;

        AzToolsFramework::RegisterViewPane<ScriptCanvasEditor::MainWindow>(LyViewPane::ScriptCanvas, LyViewPane::CategoryTools, options);
    }

    void SystemComponent::Deactivate()
    {
        m_documentContext.Deactivate();

        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();

        for (auto&& propertyHandler : m_propertyHandlers)
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, propertyHandler.get());
        }
        m_propertyHandlers.clear();
    }

    void SystemComponent::AddAsyncJob(AZStd::function<void()>&& jobFunc)
    {
        auto* asyncFunction = AZ::CreateJobFunction(AZStd::move(jobFunc), true, m_jobContext.get());
        asyncFunction->Start();
    }

    void SystemComponent::GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes)
    {
        outCreatableTypes.insert(m_creatableTypes.begin(), m_creatableTypes.end());
    }

    void SystemComponent::CreateEditorComponentsOnEntity(AZ::Entity* entity)
    {
        if (entity)
        {
            auto graph = entity->CreateComponent<Graph>();
            entity->CreateComponent<EditorGraphVariableManagerComponent>(graph->GetUniqueId());
        }
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

    AzToolsFramework::AssetBrowser::SourceFileDetails SystemComponent::GetSourceFileDetails(const char* fullSourceFileName)
    {
        if (AZStd::wildcard_match("*.scriptcanvas", fullSourceFileName))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails("Editor/Icons/AssetBrowser/ScriptCanvas_16.png");
        }

        // not one of our types.
        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }

    void SystemComponent::PopulateEditorCreatableTypes()
    {
        AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Behavior Context should not be missing at this point");

        bool showExcludedPreviewNodes = false;
        // Local User Settings are not registered in an asset builders
        if (AZ::UserSettingsBus::GetNumOfEventHandlers(AZ::UserSettings::CT_LOCAL) > 0)
        {
            AZStd::intrusive_ptr<ScriptCanvasEditor::EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<ScriptCanvasEditor::EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
            showExcludedPreviewNodes = settings ? settings->m_showExcludedNodes : false;
        }

        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        for (const auto& scType : dataRegistry->m_creatableTypes)
        {
            if (!showExcludedPreviewNodes && scType.GetType() == ScriptCanvas::Data::eType::BehaviorContextObject)
            {
                if (const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, ScriptCanvas::Data::ToAZType(scType)))
                {
                    // BehaviorContext classes with the ExcludeFrom attribute with a value of the ExcludeFlags::Preview are not added to the list of 
                    // types that can be created in the editor
                    const AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::Preview;
                    auto excludeClassAttributeData = azrtti_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));
                    if (excludeClassAttributeData && (excludeClassAttributeData->Get(nullptr) & exclusionFlags))
                    {
                        continue;
                    }
                }
            }

            m_creatableTypes.insert(scType);
        }
    }
}