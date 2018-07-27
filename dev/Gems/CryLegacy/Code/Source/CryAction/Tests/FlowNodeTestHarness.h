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
#include <FlowSystem/FlowSystem.h>
#include <Mocks/IFlowGraphMock.h>

namespace
{
    const SFlowAddress invalidFlowAddress(0, 0, false);

    template <typename T>
    bool IsEqual(const T& a, const T& b)
    {
        return a == b;
    }

    template<>
    bool IsEqual<float>(const float& a, const float& b)
    {
        const float eps = 1.0e-6f;
        return fabs(a - b) < eps;
    }
}

//////////////////////////////////////////////////////////////////////////
// Externally defined helpers
//////////////////////////////////////////////////////////////////////////

// [FlowMathNodes.cpp]
// Flow node factory
IFlowNode* CreateMathFlowNode(const char* name);



//////////////////////////////////////////////////////////////////////////
// Matchers
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//! typed output port matcher
template<typename DataType>
class OutPortEqualsMatcher
    : public ::testing::MatcherInterface < const NFlowSystemUtils::Wrapper<DataType>& >
{
public:
    typedef const NFlowSystemUtils::Wrapper<DataType>& wrapper_type;

    //////////////////////////////////////////////////////////////////////////
    OutPortEqualsMatcher(DataType expected)
        : m_expected(expected)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    bool MatchAndExplain(wrapper_type wrapper, ::testing::MatchResultListener* listener) const override
    {
        return IsEqual(wrapper.value, m_expected);
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void DescribeTo(::std::ostream* os) const override
    {
        *os << "out port equals " << ::testing::PrintToString(m_expected);
    }

    //////////////////////////////////////////////////////////////////////////
    void DescribeNegationTo(::std::ostream* os) const override
    {
        *os << "out port does not equal " << ::testing::PrintToString(m_expected);
    }

private:
    const DataType m_expected;
};

template<typename DataType>
inline ::testing::Matcher<const NFlowSystemUtils::Wrapper<DataType>&> OutPortEquals(DataType expected)
{
    return ::testing::MakeMatcher(new OutPortEqualsMatcher<DataType>(expected));
}

//////////////////////////////////////////////////////////////////////////
//! "any" type output port matcher
template<typename DataType>
class OutPortAnyEqualsMatcher
    : public ::testing::MatcherInterface < const TFlowInputData& >
{
public:
    OutPortAnyEqualsMatcher(DataType expected)
        : m_expected(expected)
        , m_expectedValue(expected)
    {
    }

    bool MatchAndExplain(const TFlowInputData& actual, ::testing::MatchResultListener* listener) const override
    {
        return actual == m_expected;
    }

    virtual void DescribeTo(::std::ostream* os) const override
    {
        *os << "out port <any> equals " << ::testing::PrintToString(m_expectedValue);
    }

    void DescribeNegationTo(::std::ostream* os) const override
    {
        *os << "out port <any> does not equal " << ::testing::PrintToString(m_expectedValue);
    }

private:
    const TFlowInputData m_expected;
    const DataType m_expectedValue;
};

template<typename DataType>
inline ::testing::Matcher<const TFlowInputData&> OutPortAnyEquals(DataType expected)
{
    return ::testing::MakeMatcher(new OutPortAnyEqualsMatcher<DataType>(expected));
}

//////////////////////////////////////////////////////////////////////////
//! Flow Graph Test Harness
//!
//! Enables easy unit testing of individual flow nodes.
class FlowNodeTestHarness
    : public ::testing::Test
{
public:
    FlowNodeTestHarness(IFlowNode* flowNode)
        : m_flowNode(flowNode)
        , m_fakeNodeId(99)
        , m_config(GetConfig(flowNode))
        , m_inputPorts(nullptr)
        , m_numInputPorts(0)
        , m_numOutputPorts(0)
    {
        if (m_config.pInputPorts)
        {
            int portCount = 0;


            // activation port automatically offsets input ports by 1
            if (m_config.nFlags & EFLN_ACTIVATION_INPUT)
            {
                ++portCount;
                SInputPortConfig config = { 0 };
                config.name = "Activate";
                config.humanName = "Activate";
                config.description = "";
                config.sUIConfig = "";
                m_inputPortConfig.push_back(config);
            }

            // iter over the normal input ports
            int i = 0;
            while (m_config.pInputPorts[i].name)
            {
                m_inputPortConfig.push_back(m_config.pInputPorts[i]);
                ++i;
            }

            m_numInputPorts = i + portCount;
            if (m_numInputPorts)
            {
                m_inputPorts = new TFlowInputData[m_numInputPorts];
            }
        }

        // make a vector of the output ports
        if (m_config.pOutputPorts)
        {
            int i = 0;
            while (m_config.pOutputPorts[i].name)
            {
                m_outputPortConfig.push_back(m_config.pOutputPorts[i]);
                ++i;
            }
            m_numOutputPorts = m_outputPortConfig.size();
        }
    }

    // activate updates the node once and then clears all input activity
    void Activate()
    {
        // activate
        IFlowNode::SActivationInfo act(&m_mockFlowGraph, m_fakeNodeId, nullptr /*userData*/, m_inputPorts);
        m_flowNode->ProcessEvent(IFlowNode::EFlowEvent::eFE_Activate, &act);

        ClearActivation();
    }

    void Update()
    {
        IFlowNode::SActivationInfo act(&m_mockFlowGraph, m_fakeNodeId, nullptr /*userData*/, m_inputPorts);
        m_flowNode->ProcessEvent(IFlowNode::EFlowEvent::eFE_Update, &act);

        ClearActivation();
    }

    template<typename T>
    void SetInputPort(const SFlowAddress& addr, T value)
    {
        m_inputPorts[addr.port].Set(value);
        m_inputPorts[addr.port].SetUserFlag(true);
    }

    template<typename T>
    void SetInputPort(const char* portName, T value)
    {
        SFlowAddress portAddress = FindInputPortAddress(portName);
        if (portAddress != invalidFlowAddress)
        {
            SetInputPort<T>(portAddress, value);
        }
    }

    // Find the index of a port by name. Returns -1 if the port
    // is not found.
    SFlowAddress FindInputPortAddress(const char* portName) const
    {
        const std::string compareTo(portName);

        for (TFlowPortId i = 0; i < m_numInputPorts; i++)
        {
            if (compareTo == m_inputPortConfig[i].name)
            {
                return SFlowAddress(m_fakeNodeId, i, true);
            }
        }
        return invalidFlowAddress;
    }

    SFlowAddress FindOutputPortAddress(const char* portName) const
    {
        const std::string compareTo(portName);
        for (TFlowPortId i = 0; i < m_numOutputPorts; i++)
        {
            if (compareTo == m_outputPortConfig[i].name)
            {
                return SFlowAddress(m_fakeNodeId, i, true);
            }
        }

        return invalidFlowAddress;
    }

protected:

    IFlowNode* GetFlowNodeInterface() { return m_flowNode; }

    // Get a copy of the config data for the flow node
    // to store in a constant member.
    static SFlowNodeConfig GetConfig(IFlowNode* flowNode)
    {
        SFlowNodeConfig config;
        flowNode->GetConfiguration(config);
        return config;
    }

    const SFlowNodeConfig m_config;
    const TFlowNodeId m_fakeNodeId;

    MockFlowGraph m_mockFlowGraph;

    IFlowNode* m_flowNode;
    TFlowInputData* m_inputPorts;
    std::vector<SInputPortConfig> m_inputPortConfig;
    std::vector<SOutputPortConfig> m_outputPortConfig;
    uint m_numOutputPorts;
    uint m_numInputPorts;

private:
    void ClearActivation()
    {
        for (int i = 0; i < m_numInputPorts; i++)
        {
            TFlowInputData& port = m_inputPorts[i];
            port.ClearUserFlag();
        }
    }
};