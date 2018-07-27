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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include <ISystem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <IFlowSystem.h>

#include <AzCore/std/utils.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/deque.h>
#include <LmbrCentral/Scripting/TriggerAreaComponentBus.h>



class ComponentAreaTriggerNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    //////////////////////////////////////////////////////////////////////////
    // ComponentAreaTriggerNode
    //////////////////////////////////////////////////////////////////////////

public:

    // This constructor only exists for compatibility with construction code outside of this node, it does not beed the Activation info
    ComponentAreaTriggerNode(SActivationInfo* /*activationInformation*/)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* activationInformation) override
    {
        return new ComponentAreaTriggerNode(activationInformation);
    }

    void GetConfiguration(SFlowNodeConfig& config) override;

    void ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation) override;

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

private:

    //////////////////////////////////////////////////////////////////////////
    // AreaTriggerEventHandler helper class
    //////////////////////////////////////////////////////////////////////////

    /*
    * The Area Trigger Event Handler keeps a track of all the Area Trigger Component events that happened after the Last Update was called
    * every update, the node asks it for the latest events that it knows of for the Area Trigger Component on a particular Entity.
    * If this Entity's EntityId matches the one that was asked for in the previous update, it informs the caller (node) of all the events via
    * a queue of tuples arranged in chronological order.
    * If the Id is different however, all events in the queue are discarded and a new queue is started for this new entity id
    */
    class AreaTriggerEventHandler
        : LmbrCentral::TriggerAreaNotificationBus::Handler
    {
    public:

        enum class TriggerEventType
        {
            Entry,
            Exit
        };

        AreaTriggerEventHandler()
            : m_lastCheckedEntityId(FlowEntityId::s_invalidFlowEntityID)
        {
        }

        AreaTriggerEventHandler(FlowEntityId triggerEntityId)
        {
            ResetHandlerForEntityId(triggerEntityId);
        }

        /**
        * Resets the Trigger event handler to respond to a new trigger entity
        * @param triggerEntityId Entity id of the entity that owns the area trigger
        */
        void ResetHandlerForEntityId(FlowEntityId triggerEntityId);

        using TriggeringEvent = AZStd::pair<TriggerEventType, FlowEntityId>;
        /**
        * Gets the Next triggering event if there is one in the Queue
        * @param triggerComponentOwnerEntityId Id for the AZ::Entity that owns this Area Trigger Component
        * @return An Outcome that gives a pair of 'EventType' and FlowEntityId of the 'AZ::Entity'
        that triggered said event OR failure if there are no more events in the Queue
        */
        AZ::Outcome<TriggeringEvent> GetNextTriggeringEvent(FlowEntityId triggerComponentOwnerEntityId);

        //////////////////////////////////////////////////////////////////////////
        // TriggerAreaEventBus::Handler implementation
        //////////////////////////////////////////////////////////////////////////

        void OnTriggerAreaEntered(AZ::EntityId enteringEntityId) override
        {
            AddEventToBuffer(TriggerEventType::Entry, FlowEntityId(enteringEntityId));
        }

        void OnTriggerAreaExited(AZ::EntityId exitingEntityId) override
        {
            AddEventToBuffer(TriggerEventType::Exit, FlowEntityId(exitingEntityId));
        }


    private:

        inline void AddEventToBuffer(TriggerEventType eventType, FlowEntityId triggeringEntityId)
        {
            m_triggeringEventBuffer.push_back(AZStd::make_pair<TriggerEventType, FlowEntityId>(eventType, FlowEntityId(triggeringEntityId)));
        }

        //! Buffer for all the events that the Area Trigger attached to the entity on this flowgraph has had between updates
        AZStd::deque<TriggeringEvent> m_triggeringEventBuffer;

        //! The entity id for which triggering events were requested the last update
        FlowEntityId m_lastCheckedEntityId;
    };

    //////////////////////////////////////////////////////////////////////////
    // ComponentAreaTriggerNode
    //////////////////////////////////////////////////////////////////////////

    enum OutputPorts
    {
        Entered = 0,
        Exited,
    };

    AreaTriggerEventHandler m_areaTriggerEvents;
};
