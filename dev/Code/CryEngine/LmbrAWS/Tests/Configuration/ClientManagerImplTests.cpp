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
#include "StdAfx.h"

#include <LmbrAWS.h>
#include <Configuration/ClientManagerImpl.h>
#include <AzTest/AzTest.h>

using namespace LmbrAWS;

///////////////////////////////////////////////////////////////////////////
// Common

class LmbrAWS_ClientManagerImpl_Test
    : public ::testing::Test
{
public:

    ClientManagerImpl cm;
};

///////////////////////////////////////////////////////////////////////////
// ConfigurationParameters

class LmbrAWS_ConfigurationParamtersImpl_Test
    : public LmbrAWS_ClientManagerImpl_Test
{
public:
    ConfigurationParameters& cp = cm.GetConfigurationParameters();
};

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, GetParameter_UnsetValue_ReturnsEmptyString)
{
    ASSERT_EQ(cp.GetParameter("foo"), "");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, SetParameter_UnsetValue_SetsValue)
{
    cp.SetParameter("foo", "foo-value");
    ASSERT_EQ(cp.GetParameter("foo"), "foo-value");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, SetParameter_AlreadySetValue_ReplacesValue)
{
    cp.SetParameter("foo", "foo-value-1");
    cp.SetParameter("foo", "foo-value-2");
    ASSERT_EQ(cp.GetParameter("foo"), "foo-value-2");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, SetParameter_ValueWithRef_SetValueHasRef)
{
    cp.SetParameter("foo", "$ref$");
    ASSERT_EQ(cp.GetParameter("foo"), "$ref$");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_UnsetValue_EmptyString)
{
    ParameterValue value {
        "$foo$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_SimpleRef_ParameterValue)
{
    cp.SetParameter("foo", "foo-value");
    ParameterValue value {
        "$foo$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "foo-value");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_PrefixedRef_PrefixedParameterValue)
{
    cp.SetParameter("foo", "foo-value");
    ParameterValue value {
        "prefix $foo$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "prefix foo-value");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_PostfixedRef_PostfixedParameterValue)
{
    cp.SetParameter("foo", "foo-value");
    ParameterValue value {
        "$foo$ postfix"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "foo-value postfix");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_InfixedRef_InfixedParameterValue)
{
    cp.SetParameter("foo", "foo-value");
    ParameterValue value {
        "prefix $foo$ postfix"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "prefix foo-value postfix");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_TwoRefs_BothParameterValues)
{
    cp.SetParameter("foo", "foo-value");
    cp.SetParameter("bar", "bar-value");
    ParameterValue value {
        "$foo$ $bar$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "foo-value bar-value");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_NestedRef_BothValues)
{
    cp.SetParameter("foo", "foo-value $bar$");
    cp.SetParameter("bar", "bar-value");
    ParameterValue value {
        "$foo$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "foo-value bar-value");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_DoubleDollar_SingleDollar)
{
    ParameterValue value {
        "$$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "$");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_PrefixedDoubleDollar_PrefixedDollar)
{
    ParameterValue value {
        "prefix $$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "prefix $");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_PostfixedDoubleDollar_PostfixedDollar)
{
    ParameterValue value {
        "$$ postfix"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "$ postfix");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_InfixedDoubleDollar_InfixedDollar)
{
    ParameterValue value {
        "prefix $$ postfix"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "prefix $ postfix");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_NestedDoubleDollar_SingleDollar)
{
    cp.SetParameter("foo", "$$");
    ParameterValue value {
        "$foo$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "$");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_SelfRef_UnexpandedRef)
{
    cp.SetParameter("foo", "foo-value $foo$");
    ParameterValue value {
        "$foo$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "foo-value $foo$");
}

TEST_F(LmbrAWS_ConfigurationParamtersImpl_Test, ExpandParameters_NestedSelfRef_UnexpantedRef)
{
    cp.SetParameter("foo", "foo-value $bar$");
    cp.SetParameter("bar", "bar-value $foo$");
    ParameterValue value {
        "$foo$"
    };
    cp.ExpandParameters(value);
    ASSERT_EQ(value, "foo-value bar-value $foo$");
}
