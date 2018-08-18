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
#include <Tests/FlowNodeTestHarness.h> // CryCommon

namespace
{
    const char* mathAddNodeName = "Math:Add";
    const char* activatePortName = "Activate";
    const char* aPortName = "A";
    const char* bPortName = "B";
    const char* outPortName = "Out";
}

// Arguments to Math:Add must be floating point values or the tests will fail.
// Retrieval of non-float inputs as floating point will return 0.0f.
class MathAddArgs
{
public:
    MathAddArgs(float a, float b, float expected)
        : m_a(a)
        , m_b(b)
        , m_expected(expected) { }

    const float m_a;
    const float m_b;
    const float m_expected;
};

void PrintTo(const MathAddArgs& args, std::ostream* os)
{
    *os << "a:" << args.m_a << ", b:" << args.m_b << ", expected:" << args.m_expected;
}

//////////////////////////////////////////////////////////////////////////

class MathAddTest
    : public FlowNodeTestHarness
    , public ::testing::WithParamInterface<MathAddArgs>
{
public:
    MathAddTest()
        : FlowNodeTestHarness(CreateMathFlowNode(mathAddNodeName))
    {
        outPortAddress = FindOutputPortAddress(outPortName);
    }

    virtual ~MathAddTest()
    {
    }

    SFlowAddress outPortAddress;
};

TEST_P(MathAddTest, Activate_AAndBSet_OutputIsSumOfInputs)
{
    const MathAddArgs& param = GetParam();

    EXPECT_CALL(m_mockFlowGraph,
        DoActivatePort(outPortAddress, OutPortEquals(param.m_expected))
        ).Times(1);

    SetInputPort(activatePortName, true);
    SetInputPort(aPortName, param.m_a);
    SetInputPort(bPortName, param.m_b);

    Activate();
}

// WARNING! math flow nodes only work with floating point values
INSTANTIATE_TEST_CASE_P(SimpleValues, MathAddTest, ::testing::Values(
        MathAddArgs(0, 0, 0.0f),
        MathAddArgs(1, 2, 3.f), // positive
        MathAddArgs(2, 1, 3.f), // commutative
        MathAddArgs(-1, -2, -3.f), // negative
        MathAddArgs(-3, 2, -1.f), // neg, pos
        MathAddArgs(2, -3, -1.f)) // pos, neg
    );
