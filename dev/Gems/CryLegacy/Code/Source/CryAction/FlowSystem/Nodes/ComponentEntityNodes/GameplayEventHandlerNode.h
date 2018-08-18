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

#include <ISystem.h>
#include <IFlowSystem.h>

#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <GameplayEventBus.h>
#include <AzCore/Math/Vector3.h>

class GameplayEventHandlerNode
    : public CFlowBaseNode<eNCT_Instanced>
    , public AZ::GameplayNotificationBus::Handler
{
    //////////////////////////////////////////////////////////////////////////
    // EventActionHandlerNode
    //////////////////////////////////////////////////////////////////////////
public:
    AZ_CLASS_ALLOCATOR(GameplayEventHandlerNode, AZ::SystemAllocator, 0);
    // This constructor only exists for compatibility with construction code outside of this node, it does not need the Activation info
    GameplayEventHandlerNode(SActivationInfo* /*activationInformation*/)
    {
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override;

    IFlowNodePtr Clone(SActivationInfo* activationInformation) override
    {
        return new GameplayEventHandlerNode(activationInformation);
    }

    void ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation) override;

    //////////////////////////////////////////////////////////////////////////
    // AZ::GameplayNotificationBus
    void OnEventBegin(const AZStd::any& value) override;
    void OnEventUpdating(const AZStd::any& value) override;
    void OnEventEnd(const AZStd::any& value) override;

private:
    void ActivatePortWithAny(SActivationInfo* activationInformation);
    void SetCurrentHandler(SActivationInfo* activationInformation);
    void ConnectToHandlerBus();
    void DisconnectFromHandlerBus();
    enum InputPorts
    {
        ChannelId = 0,
        EventName,
        Enable,
        Disable,
    };

    enum CachedEventType
    {
        Begin = 0,
        Update,
        End,
        None
    };
    AZ_ALIGN(AZStd::any m_storedValue, 32);
    AZ::EntityId m_channelId;
    CachedEventType m_cachedEventType = None;
    string m_eventName;
    bool m_enabled = false;
};

class GameplayEventSenderNode
    : public CFlowBaseNode<eNCT_Instanced>
{
    //////////////////////////////////////////////////////////////////////////
    // EventActionHandlerNode
    //////////////////////////////////////////////////////////////////////////

public:

    // This constructor only exists for compatibility with construction code outside of this node, it does not need the Activation info
    GameplayEventSenderNode(SActivationInfo* /*activationInformation*/)
    {
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override;

    IFlowNodePtr Clone(SActivationInfo* activationInformation) override
    {
        return new GameplayEventSenderNode(activationInformation);
    }

    void ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation) override;

private:
    enum GameplayEventType
    {
        Begin,
        Updating,
        End
    };

    enum InputPorts
    {
        Activate = 0,
        ChannelId,
        Value,
        EventName,
        EventType,
    };

    void SendGameplayEvent(const AZ::GameplayNotificationId& actionBusId, const AZStd::any& value, GameplayEventType eventType);
    AZStd::any GetPortValueAsAny(SActivationInfo* activationInformation);
    AZ::EntityId m_channelId;
};


//////////////////////////////////////////////////////////////////////////
/// Specializations for FLowVec3 AZ::Vector3
