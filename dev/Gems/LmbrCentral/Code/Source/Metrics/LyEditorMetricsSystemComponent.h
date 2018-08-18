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

#ifdef METRICS_SYSTEM_COMPONENT_ENABLED

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzCore/std/containers/set.h>

#include <IEditor.h>

namespace LyEditorMetrics
{
    class ActionMetricsTracker;

    class LyEditorMetricsSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorMetricsEventsBus::Handler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private IEditorNotifyListener
    {
    public:
        AZ_COMPONENT(LyEditorMetricsSystemComponent, "{B8C74085-F6B7-4E2F-8135-78C991CC53C5}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        LyEditorMetricsSystemComponent();

        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LyEditorMetricsRequestBus interface implementation

        void BeginUserAction(NavigationTrigger behaviour) override;
        void EndUserAction() override;

        void EntityCreated(const AZ::EntityId& entityId) override;
        void EntityDeleted(const AZ::EntityId& entityId) override;

        void SliceInstantiated(const AZ::Crc32& sliceIdentifier) override;

        void ComponentAdded(const AZ::EntityId& entityId, const AZ::Uuid& componentTypeId) override;
        void ComponentRemoved(const AZ::EntityId& entityId, const AZ::Uuid& componentTypeId) override;

        void UpdateTransformParentEntity(const AZ::EntityId& entityId, const AZ::EntityId& newParentId, const AZ::EntityId& oldParentId) override;

        void LegacyEntityCreated(const char* entityType, const char* scriptEntityType);

        void Undo() override;
        void Redo() override;

        void EntitiesCloned() override;

        void MenuTriggered(const char* menuIdentifier, AzToolsFramework::MetricsActionTriggerType triggerType) override;

        void AssetBrowserAction(AzToolsFramework::AssetBrowserActionType actionType, const AZ::Uuid& sourceUuid, const char* extension, int numberOfProducts) override;

        void RegisterAction(QAction* action, const QString& metricsText) override;
        void UnregisterAction(QAction* action) override;

        // From the EditorMetricsEventsBus; same thing as BeforeEntitySelectionChanged and AfterEntitySelectionChanged but called properly
        void BeginSelectionChange() override;
        void EndSelectionChange() override;

        ////////////////////////////////////////////////////////////////////////
        // ToolsApplicationEvents
        void BeforeEntitySelectionChanged() override;
        void AfterEntitySelectionChanged() override;
        void BeforeUndoRedo() override;
        void AfterUndoRedo() override;

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEvents implementation
        void NotifyIEditorAvailable(IEditor* editor) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        bool m_notifierRegistered = false;

        AZ::u64 m_actionId = 0;
        NavigationTrigger m_navigationBehaviour;
        bool m_needToFireNavigationEvent = false;
        AZStd::string m_actionIdString;
        ActionMetricsTracker* m_actionTracker;
        AZStd::set<AZStd::string> m_legacyEntityNameWhiteList;
        AZStd::set<AZStd::string> m_legacyScriptEntityNameWhiteList;
        AZStd::set<AZStd::string> m_extensionWhiteList;
        AZStd::unordered_map<AzToolsFramework::AssetBrowserActionType, AZStd::string> m_assetBrowserActionMap;

        int m_batchingSelectionStackSize = 0;
        int m_queuedSelectionChangedEvents = 0;

        void InitializeLegacyEntityList();
        void InitializeLegacyScriptEntityList();
        void InitializeActionBrowserData();

        // helper function for instantiating slice metrics events
        void SendSliceInstantiatedMetricsEvent(const char* eventName, const AZ::Crc32& sliceIdentifier);

        // helper function for create/delete entity metrics events
        void SendEntitiesMetricsEvent(const char* eventName, const AZ::EntityId& entityId);

        // helper function for add/remove component metrics events
        void SendComponentsMetricsEvent(const char* eventName, const AZ::EntityId& entityId, const AZ::Uuid& componentTypeId);

        // helper function for sending parent changed events
        void SendParentIdMetricsEvent(const char* eventName, const AZ::EntityId& entityId, const AZ::EntityId& newParentId, const AZ::EntityId& oldParentId);

        // helper function for undo/redo events
        void SendUndoRedoMetricsEvent(const char* eventName);

        void SendNavigationEventIfNeeded();
    };
}

#endif // #ifdef METRICS_SYSTEM_COMPONENT_ENABLED
