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

#include "Debugger.h"
#include "Reflection.h"

#include "Messages/Acknowledge.h"
#include "Messages/Request.h"

#include "Bus.h"

namespace ScriptCanvas
{
    namespace Debugger
    {

        void Component::OnSystemTick()
        {
            // If we are detached, we call process to know when we need to enable the debugger.
            // Once the debugger is enabled, processing will happen in the attached context.
            //if (m_debuggerState == DebuggerState::Detached)
            {
                Process();
            }
        }

        void Component::Process()
        {
            while (!m_msgQueue.empty())
            {
                m_msgMutex.lock();
                AzFramework::TmMsgPtr msg = *m_msgQueue.begin();
                m_msgQueue.pop_front();
                m_msgMutex.unlock();
                AZ_Assert(msg, "We received a NULL message in the script debug agent's message queue!");
                AzFramework::TargetInfo sender;
                EBUS_EVENT_RESULT(sender, AzFramework::TargetManager::Bus, GetTargetInfo, msg->GetSenderTargetId());

                // The only message we accept without a target match is AttachDebugger
                if (m_debugger.IsValid() && m_debugger.GetNetworkId() != sender.GetNetworkId())
                {
                    AzFramework::TmMsg* request = azdynamic_cast<AzFramework::TmMsg*>(msg.get());
                    if (!request || (request->GetId() != AZ_CRC("ScriptCanvasDebugRequest", 0xa4c45706)))
                    {
                        AZ_TracePrintf("Script Canvas Debug", "Rejecting msg 0x%x (%s is not the attached debugger)\n", request->GetId(), sender.GetDisplayName());
                       // AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::SendTmMessage, sender, Message::Acknowledge(0, AZ_CRC("AccessDenied", 0xde72ce21)));
                        continue;
                    }
                }

                if (azrtti_istypeof<Message::Request*>(msg.get()))
                {
                    Message::Request* request = azdynamic_cast<Message::Request*>(msg.get());
                    if (request)
                    {
                        if (request->m_request == AZ_CRC("ScriptCanvas_OnEntry", 0x07ee76d4))
                        {
                            Message::NodeState* nodeState = azdynamic_cast<Message::NodeState*>(msg.get());

                            AZ_TracePrintf("Script Canvas Debugger", "On Entry: %s\n", request->m_graphId.ToString().c_str());
                        }
                        else if (request->m_request == AZ_CRC("ScriptCanvas_AttachDebugger", 0xaf64de1f))
                        {
                            Attach(request->m_graphId);
                            return;
                        }
                        else if (request->m_request == AZ_CRC("ScriptCanvas_DetachDebugger", 0x4256cf2d))
                        {
                            Detach(request->m_graphId);
                            return;
                        }
                        else
                        {
                            AZ_TracePrintf("Script Canvas Debugger", "Received invalid command 0x%x.\n", request->m_request);
                            AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::SendTmMessage, sender, Message::Acknowledge(request->m_request, AZ_CRC("InvalidCommand", 0x2114fa46)));
                        }
                    }
                }
            }
        }

        void Component::OnReceivedMsg(AzFramework::TmMsgPtr msg)
        {
            AZStd::lock_guard<AZStd::mutex> l(m_msgMutex);
            m_msgQueue.push_back(msg);
        }

        void Component::Attach(const AzFramework::TargetInfo& targetInfo, const AZ::EntityId& graphId)
        {

            // Notify debugger that it has successfully connected
            if (graphId.IsValid())
            {
                auto debugTarget = m_debugTargets.find(graphId);
                if (debugTarget != m_debugTargets.end())
                {
                    if (debugTarget->second.m_state == DebuggerState::Running)
                    {
                        AZ_TracePrintf("Script Canvas Debugger", "Already debugging graph %s.\n", debugTarget->second.m_info.GetDisplayName());
                    }
                    else
                    {
                        debugTarget->second.m_state = DebuggerState::Running;
                    }
                }
                else
                {
                    m_debugTargets[graphId] = { targetInfo, DebuggerState::Running };

                    AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::SendTmMessage, targetInfo, Message::Request(AZ_CRC("ScriptCanvas_AttachDebugger", 0xaf64de1f), graphId));
                    AZ_TracePrintf("Script Canvas Debugger", "Remote debugger %s has attached.\n", targetInfo.GetDisplayName());

                    if (m_debuggerState != DebuggerState::Running)
                    {
                        m_debuggerState = DebuggerState::Running;
                    }
                }
            }
        }

        void Component::Attach(const AZ::EntityId& graphId)
        {
            if (!graphId.IsValid())
            {
                AZ_TracePrintf("Script Canvas Debugger", "Not a valid graph to debug\n"/*, scriptContextName*/);
                return;
            }

            RequestBus::Handler::BusConnect(graphId);

            AzFramework::TargetInfo targetInfo;
            if (GetDesiredTarget(targetInfo))
            {
                Attach(targetInfo, graphId);
                NotificationBus::Broadcast(&Notifications::OnAttach, graphId);
                
                AZ_TracePrintf("Script Canvas Debugger", "Debugger::Component::Attach: %s: %s\n", targetInfo.GetDisplayName(), graphId.ToString().c_str());
            }
        }

        void Component::Detach(const AZ::EntityId& graphId)
        {
            if (!graphId.IsValid())
            {
                AZ_TracePrintf("Script Canvas Debugger", "Not currently debugging\n");
                return;
            }

            auto debugTarget = m_debugTargets.find(graphId);
            if (debugTarget == m_debugTargets.end())
            {
                AZ_TracePrintf("Script Canvas Debugger", "Not attached to graph trying to detach %s.\n", graphId.ToString().data());
            }
            else
            {
                debugTarget->second.m_state = DebuggerState::Detaching;
                AZ_TracePrintf("Script Canvas Debugger", "Debugger::Component::Detach: %s: %s\n", debugTarget->second.m_info.GetDisplayName(), graphId.ToString().c_str());
                NotificationBus::Broadcast(&Notifications::OnDetach, graphId);
            }

            RequestBus::Handler::BusDisconnect();

        }


        void Component::DetachAll()
        {
            AZStd::for_each(m_debugTargets.begin(), m_debugTargets.end(), [](AZStd::pair<AZ::EntityId, Target>& targetIterator) { targetIterator.second.m_state = Detached; });

            AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::SendTmMessage, m_debugger, Message::Acknowledge(AZ_CRC("ScriptCanvas_DetachDebugger", 0x4256cf2d), AZ_CRC("ScriptCanvas_Acknowledge", 0x208b98da)));
            m_debugger = AzFramework::TargetInfo();
            m_debuggerState = DebuggerState::Detached;
        }

        Component::Component()
            : AZ::Component()
        {}

        Component::~Component()
        {}

        void Component::Init()
        {}

        void Component::Activate()
        {
            m_debuggerState = DebuggerState::Detached;

            AzFramework::TargetManagerClient::Bus::Handler::BusConnect();
            AzFramework::TmMsgBus::Handler::BusConnect(AZ_CRC("ScriptCanvasDebugger", 0x9d223ef5));
            AZ::SystemTickBus::Handler::BusConnect();

            ConnectionRequestBus::Handler::BusConnect();
        }

        void Component::Deactivate()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            RequestBus::Handler::BusDisconnect();
            AzFramework::TargetManagerClient::Bus::Handler::BusDisconnect();
            AzFramework::TmMsgBus::Handler::BusDisconnect(AZ_CRC("ScriptCanvasDebugger", 0x9d223ef5));

            if (m_debuggerState != DebuggerState::Detached)
            {
                DetachAll();
            }

            ConnectionRequestBus::Handler::BusDisconnect();

            AZStd::lock_guard<AZStd::mutex> l(m_msgMutex);
            m_msgQueue.clear();
        }

        void Component::DesiredTargetConnected(bool connected)
        {
            (void)connected;
        }

        //-------------------------------------------------------------------------
        void Component::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasDebugService", 0x7ece424b));
        }
        //-------------------------------------------------------------------------
        void Component::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ScriptCanvasDebugService", 0x7ece424b));
        }
        //-------------------------------------------------------------------------
        void Component::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        }
        //-------------------------------------------------------------------------

        void Component::Reflect(AZ::ReflectContext* context)
        {
            ReflectMessages(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Component, AZ::Component>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Component>(
                        "Script Canvas Runtime Debugger", "Provides remote debugging services for Script Canvas")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ;
                }
            }

            //static bool registeredComponentUuidWithMetricsAlready = false;
            //if (!registeredComponentUuidWithMetricsAlready)
            //{
            //    // have to let the metrics system know that it's ok to send back the name of the ScriptDebugAgent component to Amazon as plain text, without hashing
            //    EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, AZStd::vector<AZ::Uuid>{ azrtti_typeid<ScriptDebugAgent>() });

            //    // only ever do this once
            //    registeredComponentUuidWithMetricsAlready = true;
            //}
        }


        // Returns true if a valid target was found, in which case the info is returned in targetInfo.
        bool Component::GetDesiredTarget(AzFramework::TargetInfo& targetInfo)
        {
            AzFramework::TargetContainer targets;
            AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::EnumTargetInfos, targets);

            // discover what target the user is currently connected to, if any?
            AzFramework::TargetInfo info;
            AzFramework::TargetManager::Bus::BroadcastResult(info, &AzFramework::TargetManager::GetDesiredTarget);
            if (!info.GetPersistentId())
            {
                AZ_TracePrintf("Script Canvas Debug", "The user has not chosen a target to connect to.\n");
                return false;
            }

            targetInfo = info;
            if (
                (!targetInfo.IsValid()) ||
                ((targetInfo.GetStatusFlags() & AzFramework::TF_SELF) == 0) || // TODO #lsempe: only supporting self for now
                ((targetInfo.GetStatusFlags() & AzFramework::TF_ONLINE) == 0) ||
                ((targetInfo.GetStatusFlags() & AzFramework::TF_DEBUGGABLE) == 0)
                )
            {
                AZ_TracePrintf("Script Canvas Debug", "The target is currently not in a state that would allow debugging code (offline or not debuggable)\n");
                return false;
            }
            return true;
        }


        void Component::SetBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot)
        {

        }

        void Component::RemoveBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot)
        {

        }

        void Component::StepOver()
        {

        }

        void Component::StepIn()
        {

        }

        void Component::StepOut()
        {

        }

        void Component::Stop()
        {

        }

        void Component::Continue()
        {

        }

        void Component::NodeStateUpdate(Node* node, const AZ::EntityId& graphId)
        {
            AzFramework::TargetManager::Bus::Broadcast(&AzFramework::TargetManager::SendTmMessage, m_debugTargets[graphId].m_info, Message::NodeState(node, graphId));
        }

        void Component::TargetJoinedNetwork(AzFramework::TargetInfo info)
        {
            (void)info;
        }

        void Component::TargetLeftNetwork(AzFramework::TargetInfo info)
        {   
            (void)info;
        }

        void Component::DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID)
        {
            (void)newTargetID; (void)oldTargetID;
        }

    }
}