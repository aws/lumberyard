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
#include <AzCore/EBus/EBus.h>

namespace ImGui
{
    enum class DisplayState
    {
        Hidden,
        Visible,
        VisibleNoMouse
    };

    // Bus for getting updates from ImGui manager
    class IImGuiUpdateListener : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiUpdateListener"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiUpdateListener>;

        // ImGui Lifecycle Callbacks
        virtual void OnImGuiInitialize() {}
        virtual void OnImGuiUpdate() {}
        virtual void OnImGuiMainMenuUpdate() {}
        virtual void OnOpenEditorWindow() {}
    };
    typedef AZ::EBus<IImGuiUpdateListener> ImGuiUpdateListenerBus;

    // Bus for sending events and getting state from the ImGui manager
    class IImGuiManagerListener : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiManagerListener"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiManagerListener>;

        virtual DisplayState GetEditorWindowState() = 0;
        virtual void SetEditorWindowState(DisplayState state) = 0;
    };
    typedef AZ::EBus<IImGuiManagerListener> ImGuiManagerListenerBus;

    // Bus for getting notifications from the IMGUI Entity Outliner
    class IImGuiEntityOutlinerNotifcations : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiEntityOutlinerNotifcations"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiEntityOutlinerNotifcations>;

        // Callback for game code to handle targetting an IMGUI entity
        virtual void OnImGuiEntityOutlinerTarget(AZ::EntityId target) {}
    };
    typedef AZ::EBus<IImGuiEntityOutlinerNotifcations> ImGuiEntityOutlinerNotifcationBus;

    // a pair of an entity id, and a typeid, used to represent component rtti type info
    typedef AZStd::pair<AZ::EntityId, AZ::TypeId> ImGuiEntComponentId;
    // Bus for requests to the IMGUI Entity Outliner
    class IImGuiEntityOutlinerRequests : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiEntityOutlinerRequests"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiEntityOutlinerRequests>;

        // Callback for game code to handle targetting an IMGUI entity
        virtual void RequestEntityView(AZ::EntityId entity) = 0;
        virtual void RequestComponentView(ImGuiEntComponentId component) = 0;
        virtual void EnableTargetViewMode(bool enabled) = 0;
        virtual void EnableComponentDebug(const AZ::TypeId& comType, int priority = 1) = 0;
    };
    typedef AZ::EBus<IImGuiEntityOutlinerRequests> ImGuiEntityOutlinerRequestBus;

    // Bus for getting debug Component updates from ImGui manager
    class IImGuiUpdateDebugComponentListener : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiUpdateDebugComponentListener"; }
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ImGuiEntComponentId;
        using Bus = AZ::EBus<IImGuiUpdateDebugComponentListener>;

        // OnImGuiDebugLYComponentUpdate - must implement this, this is the callback for a componenet instance
        //      to draw it's required debugging information. 
        virtual void OnImGuiDebugLYComponentUpdate() = 0;

        // GetComponentDebugPriority - an optional implementation. The Entity Outliner will ask components what their debug priority
        //      is, no override on the handler will return the below value of 1, you can optionally override in the handler to give it 
        //      a higher priority. Priority only really matters for giving a shortcut to the highest priority debugging component on a given entity
        virtual int GetComponentDebugPriority() { return 1; }

        // Connection Policy, at component connect time, Ask the component what priority they are via ebus, then
        //      register that component type with the priority returned with the Entity Outliner
        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, id);

                // Get the debug priority for the component
                int priority = 1;
                AZ::EBus<IImGuiUpdateDebugComponentListener>::EventResult( priority, id, &AZ::EBus<IImGuiUpdateDebugComponentListener>::Events::GetComponentDebugPriority);

                // Register
                ImGuiEntityOutlinerRequestBus::Broadcast(&ImGuiEntityOutlinerRequestBus::Events::EnableComponentDebug, id.second, priority);
            }
        };
    };
    typedef AZ::EBus<IImGuiUpdateDebugComponentListener> ImGuiUpdateDebugComponentListenerBus;
}
