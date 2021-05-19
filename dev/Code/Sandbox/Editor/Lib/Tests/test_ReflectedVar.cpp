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
#include "stdafx.h"
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Controls/ReflectedPropertyControl/ReflectedVarWrapper.h>
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>

namespace ReflectedVarTests
{
    // Wrapper class to expose the min/max value of the internal reflected var to the unit tests.
    class TestReflectedVarIntAdapter : public ReflectedVarIntAdapter
    {
    public:
        int GetMinValue() { return m_reflectedVar.data()->minValue(); }
        int GetMaxValue() { return m_reflectedVar.data()->maxValue(); }
    };

    // Wrapper class to expose the min/max value of the internal reflected var to the unit tests.
    class TestReflectedVarFloatAdapter : public ReflectedVarFloatAdapter
    {
    public:
        float GetMinValue() { return m_reflectedVar.data()->minValue(); }
        float GetMaxValue() { return m_reflectedVar.data()->maxValue(); }
    };

    using ReflectedVarTestFixture = UnitTest::AllocatorsFixture;

    TEST_F(ReflectedVarTestFixture, ReflectedVarIntAdapter_HasCorrectHardMinMaxValues)
    {
        CVariable<int> intVar;
        TestReflectedVarIntAdapter intAdapter;
        intAdapter.SetVariable(&intVar);

        // By default, a ReflectedVarIntAdapter should have hard min/max values that match the full range of int values.
        EXPECT_EQ(intAdapter.GetMinValue(), std::numeric_limits<int>::lowest());
        EXPECT_EQ(intAdapter.GetMaxValue(), std::numeric_limits<int>::max());
    }

    TEST_F(ReflectedVarTestFixture, ReflectedVarFloatAdapter_HasCorrectHardMinMaxValues)
    {
        CVariable<float> floatVar;
        TestReflectedVarFloatAdapter floatAdapter;
        floatAdapter.SetVariable(&floatVar);

        // By default, a ReflectedVarFloatAdapter should have hard min/max values that "sort of" match the full range of int values.
        // In reality, the int->float conversion causes the max value to be 2147483648 instead of 2147483647.
        EXPECT_EQ(floatAdapter.GetMinValue(), static_cast<float>(std::numeric_limits<int>::lowest()));
        EXPECT_EQ(floatAdapter.GetMaxValue(), static_cast<float>(std::numeric_limits<int>::max()));
        EXPECT_EQ(floatAdapter.GetMaxValue(), 2147483648.0f);
    }
}
