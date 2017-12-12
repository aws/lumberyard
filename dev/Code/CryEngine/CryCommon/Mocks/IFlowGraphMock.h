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
#include <IFlowSystem.h>
#include <AzTest/AzTest.h>

#pragma warning( push )
#pragma warning( disable: 4373 )    // virtual function overrides differ only by const/volatile qualifiers, mock issue


class MockFlowGraph
    : public IFlowGraph
{
public:
    // IFlowSystemTyped
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<SFlowSystemVoid>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<int>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<float>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<double>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<EntityId>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<FlowEntityId>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<AZ::Vector3>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<Vec3>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<string>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<bool>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<FlowCustomData>&value));
    MOCK_METHOD2(DoActivatePort,
        void(const SFlowAddress, const NFlowSystemUtils::Wrapper<SFlowSystemPointer>&value));
    // ~IFlowSystemTyped

    // IFlowGraph
    MOCK_METHOD0(AddRef,
        void());
    MOCK_METHOD0(Release,
        void());
    MOCK_METHOD0(Clone,
        IFlowGraphPtr());
    MOCK_METHOD0(Clear,
        void());
    MOCK_METHOD1(RegisterHook,
        void(IFlowGraphHookPtr));
    MOCK_METHOD1(UnregisterHook,
        void(IFlowGraphHookPtr));
    MOCK_METHOD0(CreateNodeIterator,
        IFlowNodeIteratorPtr());
    MOCK_METHOD0(CreateEdgeIterator,
        IFlowEdgeIteratorPtr());
    MOCK_METHOD2(SetGraphEntity,
        void(FlowEntityId, int));
    MOCK_CONST_METHOD1(GetGraphEntity,
        FlowEntityId(int nIndex));
    MOCK_METHOD1(SetEnabled,
        void(bool bEnable));
    MOCK_CONST_METHOD0(IsEnabled,
        bool());
    MOCK_METHOD1(SetActive,
        void(bool bActive));
    MOCK_CONST_METHOD0(IsActive,
        bool());
    MOCK_METHOD0(UnregisterFromFlowSystem,
        void());
    MOCK_METHOD1(SetType,
        void(IFlowGraph::EFlowGraphType type));
    MOCK_CONST_METHOD0(GetType,
        IFlowGraph::EFlowGraphType());
    MOCK_METHOD0(Update,
        void());
    MOCK_METHOD2(SerializeXML,
        bool(const XmlNodeRef&root, bool reading));
    MOCK_METHOD1(Serialize,
        void(TSerialize ser));
    MOCK_METHOD0(PostSerialize,
        void());
    MOCK_METHOD0(InitializeValues,
        void());
    MOCK_METHOD0(Uninitialize,
        void());
    MOCK_METHOD0(PrecacheResources,
        void());
    MOCK_METHOD0(EnsureSortedEdges,
        void());
    MOCK_METHOD2(ResolveAddress,
        SFlowAddress(const char* addr, bool isOutput));
    MOCK_METHOD1(ResolveNode,
        TFlowNodeId(const char* name));
    MOCK_METHOD3(CreateNode,
        TFlowNodeId(TFlowNodeTypeId typeId, const char* name, void* pUserData));
    MOCK_METHOD3(CreateNode,
        TFlowNodeId(const char* typeName, const char* name, void* pUserData));
    MOCK_METHOD1(GetNodeData,
        IFlowNodeData * (TFlowNodeId id));
    MOCK_METHOD2(SetNodeName,
        bool(TFlowNodeId id, const char* sName));
    MOCK_METHOD1(GetNodeName,
        const char*(TFlowNodeId id));
    MOCK_METHOD1(GetNodeTypeId,
        TFlowNodeTypeId(TFlowNodeId id));
    MOCK_METHOD1(GetNodeTypeName,
        const char*(TFlowNodeId id));
    MOCK_METHOD1(RemoveNode,
        void(const char* name));
    MOCK_METHOD1(RemoveNode,
        void(TFlowNodeId id));
    MOCK_METHOD2(SetUserData,
        void(TFlowNodeId id, const XmlNodeRef&data));
    MOCK_METHOD1(GetUserData,
        XmlNodeRef(TFlowNodeId id));
    MOCK_METHOD2(LinkNodes,
        bool(SFlowAddress from, SFlowAddress to));
    MOCK_METHOD2(UnlinkNodes,
        void(SFlowAddress from, SFlowAddress to));
    MOCK_METHOD1(RegisterFlowNodeActivationListener,
        void(SFlowNodeActivationListener * listener));
    MOCK_METHOD1(RemoveFlowNodeActivationListener,
        void(SFlowNodeActivationListener * listener));
    MOCK_METHOD5(NotifyFlowNodeActivationListeners,
        bool(TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value));
    MOCK_METHOD2(SetEntityId,
        void(TFlowNodeId, FlowEntityId));
    MOCK_METHOD1(GetEntityId,
        FlowEntityId(TFlowNodeId));
    MOCK_CONST_METHOD0(GetClonedFlowGraph,
        IFlowGraphPtr());
    MOCK_METHOD2(GetNodeConfiguration,
        void(TFlowNodeId id, SFlowNodeConfig & ));
    MOCK_METHOD2(SetRegularlyUpdated,
        void(TFlowNodeId, bool));
    MOCK_METHOD1(RequestFinalActivation,
        void(TFlowNodeId));
    MOCK_METHOD1(ActivateNode,
        void(TFlowNodeId));
    MOCK_METHOD2(ActivatePortAny,
        void(SFlowAddress output, const TFlowInputData&));
    MOCK_METHOD2(ActivatePortCString,
        void(SFlowAddress output, const char* cstr));
    MOCK_METHOD3(SetInputValue,
        bool(TFlowNodeId node, TFlowPortId port, const TFlowInputData&));
    MOCK_METHOD1(IsOutputConnected,
        bool(SFlowAddress output));
    MOCK_METHOD2(GetInputValue,
        const TFlowInputData * (TFlowNodeId node, TFlowPortId port));
    MOCK_METHOD2(GetActivationInfo,
        bool(const char* nodeName, IFlowNode::SActivationInfo & actInfo));
    MOCK_METHOD1(SetSuspended,
        void(bool));
    MOCK_CONST_METHOD0(IsSuspended,
        bool());
    MOCK_METHOD1(SetAIAction,
        void(IAIAction * pAIAction));
    MOCK_CONST_METHOD0(GetAIAction,
        IAIAction * ());
    MOCK_METHOD1(SetCustomAction,
        void(ICustomAction * pCustomAction));
    MOCK_CONST_METHOD0(GetCustomAction,
        ICustomAction * ());
    MOCK_CONST_METHOD1(GetMemoryUsage,
        void(ICrySizer * s));
    MOCK_METHOD0(RemoveGraphTokens,
        void());
    MOCK_METHOD1(AddGraphToken,
        bool(const SGraphToken&token));
    MOCK_CONST_METHOD0(GetGraphTokenCount,
        size_t());
    MOCK_CONST_METHOD1(GetGraphToken,
        const IFlowGraph::SGraphToken * (size_t index));
    MOCK_CONST_METHOD1(GetGlobalNameForGraphToken,
        string(const string&tokenName));
    MOCK_CONST_METHOD0(GetGraphId,
        TFlowGraphId());
    MOCK_METHOD1(SetControllingHyperGraphId,
        void(const AZ::Uuid &));
    MOCK_CONST_METHOD0(GetControllingHyperGraphId,
        AZ::Uuid());
    // ~IFlowGraph
};

#pragma warning( pop )
