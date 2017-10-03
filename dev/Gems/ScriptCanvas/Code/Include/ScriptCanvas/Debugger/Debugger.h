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

#include "Bus.h"

#include <AzCore/Component/Component.h>
#include <AzFramework/TargetManagement/TargetManagementAPI.h>
#include <AzCore/Component/TickBus.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        //! The ScriptCanvas debugger component, this is the runtime debugger code that directly controls the execution
        //! and provides insight into a running ScriptCanvas graph.
        class Component 
            : public AZ::Component
            , RequestBus::Handler
            , ConnectionRequestBus::Handler
            , public AzFramework::TargetManagerClient::Bus::Handler
            , public AzFramework::TmMsgBus::Handler
            , AZ::SystemTickBus::Handler
        {
        public:
            AZ_COMPONENT(Component, "{794B1BA5-DE13-46C7-9149-74FFB02CB51B}");

            Component();
            ~Component() override;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // TargetManagerClient
            void DesiredTargetConnected(bool connected) override;
            void TargetJoinedNetwork(AzFramework::TargetInfo info) override;
            void TargetLeftNetwork(AzFramework::TargetInfo info) override;
            void DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // TmMsgBus
            void OnReceivedMsg(AzFramework::TmMsgPtr msg) override;
            //////////////////////////////////////////////////////////////////////////

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            bool GetDesiredTarget(AzFramework::TargetInfo& targetInfo);

            static void Reflect(AZ::ReflectContext* reflection);

            void Attach(const AzFramework::TargetInfo& ti, const AZ::EntityId& graphId);
            void Detach();
            void Process();

            void OnSystemTick();

            //////////////////////////////////////////////////////////////////////////
            // Debugger::ConnectionRequestBus
            void Attach(const AZ::EntityId&) override;
            void Detach(const AZ::EntityId&) override;
            void DetachAll() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Debugger::RequestBus
            void SetBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot) override;
            void RemoveBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot) override;

            void StepOver() override;
            void StepIn() override;
            void StepOut() override;
            void Stop() override;
            void Continue() override;

            void NodeStateUpdate(Node* node, const AZ::EntityId& graphId) override;
            //////////////////////////////////////////////////////////////////////////

        private:
            Component(const Component&) = delete;
            AzFramework::TargetInfo m_debugger;
            AzFramework::TmMsgQueue m_msgQueue;
            AZStd::mutex m_msgMutex;

            enum DebuggerState
            {
                Detached,
                Running,
                Paused,
                Detaching
            };

            struct Target
            {
                AzFramework::TargetInfo m_info;
                DebuggerState m_state;
            };

            AZStd::unordered_map<AZ::EntityId, Target> m_debugTargets;

            AZStd::atomic_uint      m_debuggerState;
        };
    }
}