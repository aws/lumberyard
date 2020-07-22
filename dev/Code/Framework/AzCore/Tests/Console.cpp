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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/Console.h>


namespace AZ
{
    using namespace UnitTest;

    AZ_CVAR(bool,   testBool, false, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(char,     testChar,   0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(int8_t,   testInt8,   0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(int16_t,  testInt16,  0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(int32_t,  testInt32,  0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(int64_t,  testInt64,  0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(uint8_t,  testUInt8,  0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(uint16_t, testUInt16, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(uint32_t, testUInt32, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(uint64_t, testUInt64, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float,    testFloat,  0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(double,   testDouble, 0, nullptr, ConsoleFunctorFlags::Null, "");

    AZ_CVAR(std::string, testString, "default", nullptr, ConsoleFunctorFlags::Null, "");

    AZ_CVAR(AZ::Vector2, testVec2, AZ::Vector2(0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Vector3, testVec3, AZ::Vector3(0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Vector4, testVec4, AZ::Vector4(0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Quaternion, testQuat, AZ::Quaternion(0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Color, testColorNormalized, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Color, testColorNormalizedMixed, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Color, testColorRgba, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), nullptr, ConsoleFunctorFlags::Null, "");

    class ConsoleTests
        : public AllocatorsFixture
    {
    public:
        ConsoleTests()
        {
            if (!AZ::Interface<AZ::IConsole>::Get())
            {
                new Console;
                AZ::Interface<AZ::IConsole>::Get()->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            }
        }
    };

    template <typename _CVAR_TYPE, typename _TYPE>
    void TestCVarHelper(_CVAR_TYPE& cvarInstance, const char* cVarName, const char* setCommand, const char* failCommand, _TYPE testInit, _TYPE initialValue, _TYPE setValue)
    {
        AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();

        _TYPE getCVarTest = testInit;

        ConsoleFunctorBase* foundCommand = console->FindCommand(cVarName);
        AZ_TEST_ASSERT(foundCommand != nullptr); // Find command works and returns a non-null result
        AZ_TEST_ASSERT(strcmp(foundCommand->GetName(), cVarName) == 0); // Find command actually returned the correct command

        AZ_TEST_ASSERT(console->GetCvarValue(cVarName, getCVarTest) == GetValueResult::Success); // Console finds and retrieves cvar value
        AZ_TEST_ASSERT(getCVarTest == initialValue); // Retrieved cvar value

        console->PerformCommand(setCommand);

        AZ_TEST_ASSERT(_TYPE(cvarInstance) == setValue); // Set works for type

        AZ_TEST_ASSERT(console->GetCvarValue(cVarName, getCVarTest) == GetValueResult::Success); // Console finds and retrieves cvar value
        AZ_TEST_ASSERT(getCVarTest == setValue); // Retrieved cvar value

        if (failCommand != nullptr)
        {
            console->PerformCommand(failCommand);
            AZ_TEST_ASSERT(_TYPE(cvarInstance) == setValue); // Failed command did not affect cvar state
        }
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Bool)
    {
        TestCVarHelper(testBool, "testBool", "testBool true", "testBool asdf", false, false, true);
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Char)
    {
        TestCVarHelper(testChar, "testChar", "testChar 1", nullptr, char(100), char(0), '1'); // Char interprets the input as ascii, so if you print it, you get back a '1'
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Int8)
    {
        TestCVarHelper(testInt8, "testInt8", "testInt8 1", "testInt8 asdf", int8_t(100), int8_t(0), int8_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Int16)
    {
        TestCVarHelper(testInt16, "testInt16", "testInt16 1", "testInt16 asdf", int16_t(100), int16_t(0), int16_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Int32)
    {
        TestCVarHelper(testInt32, "testInt32", "testInt32 1", "testInt32 asdf", int32_t(100), int32_t(0), int32_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Int64)
    {
        TestCVarHelper(testInt64, "testInt64", "testInt64 1", "testInt64 asdf", int64_t(100), int64_t(0), int64_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_UInt8)
    {
        TestCVarHelper(testUInt8, "testUInt8", "testUInt8 1", "testUInt8 asdf", uint8_t(100), uint8_t(0), uint8_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_UInt16)
    {
        TestCVarHelper(testUInt16, "testUInt16", "testUInt16 1", "testUInt16 asdf", uint16_t(100), uint16_t(0), uint16_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_UInt32)
    {
        TestCVarHelper(testUInt32, "testUInt32", "testUInt32 1", "testUInt32 asdf", uint32_t(100), uint32_t(0), uint32_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_UInt64)
    {
        TestCVarHelper(testUInt64, "testUInt64", "testUInt64 1", "testUInt64 asdf", uint64_t(100), uint64_t(0), uint64_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Float)
    {
        TestCVarHelper(testFloat, "testFloat", "testFloat 1", "testFloat asdf", float(100), float(0), float(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Double)
    {
        TestCVarHelper(testDouble, "testDouble", "testDouble 1", "testDouble asdf", double(100), double(0), double(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_String)
    {
        // There is really no failure condition for string, since even an empty string simply causes the console to echo the current cvar value
        TestCVarHelper(testString, "testString", "testString notdefault", nullptr, std::string("garbage"), std::string("default"), std::string("notdefault"));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Vector2)
    {
        TestCVarHelper(testVec2, "testVec2", "testVec2 1 1", "testVec2 asdf", AZ::Vector2(100, 100), AZ::Vector2(0, 0), AZ::Vector2(1, 1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Vector3)
    {
        TestCVarHelper(testVec3, "testVec3", "testVec3 1 1 1", "testVec3 asdf", AZ::Vector3(100, 100, 100), AZ::Vector3(0, 0, 0), AZ::Vector3(1, 1, 1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Vector4)
    {
        TestCVarHelper(testVec4, "testVec4", "testVec4 1 1 1 1", "testVec4 asdf", AZ::Vector4(100, 100, 100, 100), AZ::Vector4(0, 0, 0, 0), AZ::Vector4(1, 1, 1, 1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Quaternion)
    {
        TestCVarHelper(testQuat, "testQuat", "testQuat 1 1 1 1", "testQuat asdf", AZ::Quaternion(100, 100, 100, 100), AZ::Quaternion(0, 0, 0, 0), AZ::Quaternion(1, 1, 1, 1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Color_Normalized)
    {
        TestCVarHelper(
            testColorNormalized, "testColorNormalized", "testColorNormalized 1.0 1.0 1.0 1.0", "testColorNormalized asdf",
            AZ::Color(0.5f, 0.5f, 0.5f, 0.5f), AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Color_Normalized_Mixed)
    {
        TestCVarHelper(
            testColorNormalizedMixed, "testColorNormalizedMixed", "testColorNormalizedMixed 1.0 0 1.0 1", "testColorNormalizedMixed asdf",
            AZ::Color(0.5f, 0.5f, 0.5f, 0.5f), AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), AZ::Color(1.0f, 0.0f, 1.0f, 1.0f));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Color_Rgba)
    {
        TestCVarHelper(
            testColorRgba, "testColorRgba", "testColorRgba 255 255 255 255", "testColorRgba asdf",
            AZ::Color(0.5f, 0.5f, 0.5f, 0.5f), AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

    TEST_F(ConsoleTests, CVar_ConstructAfterDeferredInit)
    {
        AZ_CVAR(int32_t, testInit, 0, nullptr, ConsoleFunctorFlags::Null, "");
        TestCVarHelper(testInit, "testInit", "testInit 1", "testInit asdf", int32_t(100), int32_t(0), int32_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_FormatConversion)
    {
        AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();

        // This simply tests format conversion
        float testValue = 0.0f;
        console->PerformCommand("testString 100.5f");
        AZ_TEST_ASSERT(console->GetCvarValue("testString", testValue) == GetValueResult::Success); // Console finds and retrieves cvar value
        AZ_TEST_ASSERT(testValue == 100.5f); // Retrieved cvar value

        console->PerformCommand("testString asdf");
        AZ_TEST_ASSERT(static_cast<std::string>(testString) == "asdf"); // String changed state
        AZ_TEST_ASSERT(console->GetCvarValue("testString", testValue) != GetValueResult::Success); // Console can't convert an arbitrary string to a float
    }

    TEST_F(ConsoleTests, CVar_Autocomplete)
    {
        AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();

        // Empty input
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("");
            AZ_TEST_ASSERT(completeCommand == "");
        }

        // Prefix
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("t");
            AZ_TEST_ASSERT(completeCommand == "test");
        }

        // Prefix
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("testV");
            AZ_TEST_ASSERT(completeCommand == "testVec");
        }

        // Unique
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("testQ");
            AZ_TEST_ASSERT(completeCommand == "testQuat");
        }

        // Complete
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("testVec3");
            AZ_TEST_ASSERT(completeCommand == "testVec3");
        }
    }
}
