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
#include "CryLegacy_precompiled.h"
#include <AzTest/AzTest.h>

#include <IFlowSystem.h>
#include <FlowSystem/FlowSystem.h>
#include <Tests/FlowNodeTestHarness.h> // CryCommon

#include <iostream>

namespace
{
    const char* mathRoundNodeName = "Math:Round";
    const char* roundActivatePortName = "Activate";
    const char* roundInPortName = "In";
    const char* roundOutPortName = "OutRounded";
}

class MathRoundArgs
{
public:
    MathRoundArgs(float in, int out)
        : m_in(in)
        , m_out(out)
    { }

    const float m_in;
    const int m_out;
};

void PrintTo(const MathRoundArgs& args, std::ostream* os)
{
    *os << "MathRoundArgs(in=" << args.m_in << ", " << args.m_out << ")";
}

class MathRoundTest
    : public FlowNodeTestHarness
    , public ::testing::WithParamInterface<MathRoundArgs>
{
public:
    // Constructor and destructor are the recommended locations for
    // setup/teardown code in google test. They are called once
    // for every test.
    MathRoundTest()
        : FlowNodeTestHarness(CreateMathFlowNode(mathRoundNodeName))
    {
        outPortAddress = FindOutputPortAddress(roundOutPortName);
    }

    virtual ~MathRoundTest()
    {
    }

    SFlowAddress outPortAddress;
};


TEST_P(MathRoundTest, Activate_InputSet_OutputIsRounded)
{
    const MathRoundArgs& param = GetParam();

    EXPECT_CALL(m_mockFlowGraph,
        DoActivatePort(outPortAddress, OutPortEquals(param.m_out))
        ).Times(1);

    SetInputPort(roundActivatePortName, true); // activation input
    SetInputPort(roundInPortName, param.m_in); // value to be rounded

    Activate();
}

INSTANTIATE_TEST_CASE_P(SimpleValuesRound, MathRoundTest, ::testing::Values(
        MathRoundArgs(0.1f, 0),
        MathRoundArgs(0.49f, 0),
        MathRoundArgs(0.50f, 1),
        MathRoundArgs(0.99f, 1),
        MathRoundArgs(1.0f, 1),
        MathRoundArgs(-0.1f, 0), // ceil
        MathRoundArgs(-0.49f, 0),
        MathRoundArgs(-0.50f, -1), // neg 0.5 rounds down to -1.0
        MathRoundArgs(-0.51f, -1), // floor
        MathRoundArgs(-0.9f, -1)
        ));
