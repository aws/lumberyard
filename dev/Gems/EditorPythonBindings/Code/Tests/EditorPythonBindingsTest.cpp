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
#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>

#include "PythonTraceMessageSink.h"
#include "PythonTestingUtility.h"
#include <Source/PythonSystemComponent.h>
#include <EditorPythonBindings/EditorPythonBindingsBus.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzFramework/CommandLine/CommandRegistrationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

namespace UnitTest
{
    struct EditorPythonBindingsNotificationBusSink final
        : public EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler
    {
        EditorPythonBindingsNotificationBusSink()
        {
            EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusConnect();
        }

        ~EditorPythonBindingsNotificationBusSink()
        {
            EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        // handles EditorPythonBindingsNotificationBus

        int m_OnPreInitializeCount = 0;
        int m_OnPostInitializeCount = 0;
        int m_OnPreFinalizeCount = 0;
        int m_OnPostFinalizeCount = 0;

        void OnPreInitialize() override { m_OnPreInitializeCount++;  }
        void OnPostInitialize() override { m_OnPostInitializeCount++; }
        void OnPreFinalize() override { m_OnPreFinalizeCount++; }
        void OnPostFinalize() override { m_OnPostFinalizeCount++; }

    };

    class EditorPythonBindingsTest
        : public PythonTestingFixture
    {
    public:
        PythonTraceMessageSink m_testSink;
        EditorPythonBindingsNotificationBusSink m_notificationSink;

        void SetUp() override
        {
            PythonTestingFixture::SetUp();
            m_app.RegisterComponentDescriptor(EditorPythonBindings::PythonSystemComponent::CreateDescriptor());
        }

        void TearDown() override
        {
            // clearing up memory
            m_notificationSink = EditorPythonBindingsNotificationBusSink();
            m_testSink = PythonTraceMessageSink();

            // shutdown time!
            PythonTestingFixture::TearDown();
        }
    };

    TEST_F(EditorPythonBindingsTest, FireUpPythonVM)
    {
        enum class LogTypes
        {
            Skip = 0,
            General,
            RedirectOutputInstalled
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "RedirectOutput installed"))
                {
                    return static_cast<int>(LogTypes::RedirectOutputInstalled);
                }
                return static_cast<int>(LogTypes::General);
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        e.Deactivate();

        EXPECT_GT(m_testSink.m_evaluationMap[(int)LogTypes::General], 0);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::RedirectOutputInstalled], 1);
        EXPECT_EQ(m_notificationSink.m_OnPreInitializeCount, 1);
        EXPECT_EQ(m_notificationSink.m_OnPostFinalizeCount, 1);
        EXPECT_EQ(m_notificationSink.m_OnPreFinalizeCount, 1);
        EXPECT_EQ(m_notificationSink.m_OnPostFinalizeCount, 1);
    }

    TEST_F(EditorPythonBindingsTest, RunScriptTextBuffer)
    {
        enum class LogTypes
        {
            Skip = 0,
            ScriptWorked
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "EditorPythonBindingsTest_RunScriptTextBuffer"))
                {
                    return static_cast<int>(LogTypes::ScriptWorked);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        const char* script = 
R"(
import sys
print ('EditorPythonBindingsTest_RunScriptTextBuffer')
)";
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script);

        e.Deactivate();

        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::ScriptWorked], 1);
    }

    TEST_F(EditorPythonBindingsTest, RunScriptFile)
    {
        enum class LogTypes
        {
            Skip = 0,
            RanFromFile
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                AZStd::string_view m(message);
                if (AzFramework::StringFunc::Equal(message, "EditorPythonBindingsTest_RunScriptFile"))
                {
                    return static_cast<int>(LogTypes::RanFromFile);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZStd::string filename;
        AzFramework::StringFunc::Path::ConstructFull(m_engineRoot, "Gems/EditorPythonBindings/Code/Tests", "EditorPythonBindingsTest", "py", filename);

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilename, filename.c_str());

        e.Deactivate();

        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::RanFromFile], 1);
    }

    TEST_F(EditorPythonBindingsTest, RunScriptFileWithArgs)
    {
        enum class LogTypes
        {
            Skip = 0,
            RanFromFile,
            NumArgsCorrect,
            ScriptNameCorrect,
            Arg1Correct,
            Arg2Correct,
            Arg3Correct
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                AZStd::string_view m(message);
                if (AzFramework::StringFunc::Equal(message, "EditorPythonBindingsTestWithArgs_RunScriptFile"))
                {
                    return static_cast<int>(LogTypes::RanFromFile);
                }
                else if (AzFramework::StringFunc::Equal(message, "num args: 4"))
                {
                    return static_cast<int>(LogTypes::NumArgsCorrect);
                }
                else if (AzFramework::StringFunc::Equal(message, "script name: EditorPythonBindingsTestWithArgs.py"))
                {
                    return static_cast<int>(LogTypes::ScriptNameCorrect);
                }
                else if (AzFramework::StringFunc::Equal(message, "arg 1: arg1"))
                {
                    return static_cast<int>(LogTypes::Arg1Correct);
                }
                else if (AzFramework::StringFunc::Equal(message, "arg 2: 2"))
                {
                    return static_cast<int>(LogTypes::Arg2Correct);
                }
                else if (AzFramework::StringFunc::Equal(message, "arg 3: arg3"))
                {
                    return static_cast<int>(LogTypes::Arg3Correct);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        AZStd::string filename;
        AzFramework::StringFunc::Path::ConstructFull(m_engineRoot, "Gems/EditorPythonBindings/Code/Tests", "EditorPythonBindingsTestWithArgs", "py", filename);

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        SimulateEditorBecomingInitialized();

        AZStd::vector<AZStd::string_view> args;
        args.push_back("arg1");
        args.push_back("2");
        args.push_back("arg3");

        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, filename.c_str(), args);

        e.Deactivate();

        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::RanFromFile], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::NumArgsCorrect], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::ScriptNameCorrect], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::Arg1Correct], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::Arg2Correct], 1);
        EXPECT_EQ(m_testSink.m_evaluationMap[(int)LogTypes::Arg3Correct], 1);
    }


    struct ProxyCommandRegistrationBusHandler final
        : public AzFramework::CommandRegistrationBus::Handler
    {
        ProxyCommandRegistrationBusHandler()
        {
            BusConnect();
        }

        ~ProxyCommandRegistrationBusHandler()
        {
            BusDisconnect();
        }

        AzFramework::CommandFunction m_pyRunFileCallback;

        bool RegisterCommand(AZStd::string_view identifier, AZStd::string_view helpText, AZ::u32 commandFlags, AzFramework::CommandFunction callback) override
        {
            if (identifier == "pyRunFile")
            {
                m_pyRunFileCallback = callback;
                return true;
            }
            return false;
        }

        bool UnregisterCommand(AZStd::string_view identifier) override
        {
            if (identifier == "pyRunFile")
            {
                m_pyRunFileCallback = {};
                return true;
            }
            return false;
        }
    };

    TEST_F(EditorPythonBindingsTest, RunRegisteredFunctionByFile)
    {
        ProxyCommandRegistrationBusHandler proxyCommandRegistrationBusHandler;
        bool bExecutedFile = false;

        m_testSink.m_evaluateMessage = [&bExecutedFile](const char* window, const char* message)
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "EditorPythonBindingsTest_RunScriptFile"))
                {
                    bExecutedFile = true;
                }
            }
            return false;
        };

        AZ::Entity e;
        e.CreateComponent<EditorPythonBindings::PythonSystemComponent>();
        e.Init();
        e.Activate();

        // starting SimulateEditorBecomingInitialized() without the default CommandRegistrationBus connected
        SimulateEditorBecomingInitialized(false);

        // file running test
        ASSERT_TRUE(proxyCommandRegistrationBusHandler.m_pyRunFileCallback);
        AZStd::string filename;
        AzFramework::StringFunc::Path::ConstructFull(m_engineRoot, "Gems/EditorPythonBindings/Code/Tests", "EditorPythonBindingsTest", "py", filename);
        EXPECT_EQ(AzFramework::CommandResult::Success, proxyCommandRegistrationBusHandler.m_pyRunFileCallback({ "pyRunFile", filename.c_str() }));
        EXPECT_EQ(AzFramework::CommandResult::ErrorWrongNumberOfArguments, proxyCommandRegistrationBusHandler.m_pyRunFileCallback({ "pyRunFile" }));
        EXPECT_TRUE(bExecutedFile);

        e.Deactivate();
    }

}


AZ_UNIT_TEST_HOOK();
