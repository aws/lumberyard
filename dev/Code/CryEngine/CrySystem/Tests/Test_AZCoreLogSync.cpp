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

#include <AzTest/AzTest.h>
#include <AZCoreLogSink.h>

#include <Mocks/ISystemMock.h>
#include <Mocks/ILogMock.h>

namespace AZCoreLogSinkTests
{
    using ::testing::NiceMock;
    using ::testing::_;

    class AZCoreLogSinkTests
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            m_priorEnv = gEnv;

            m_data = AZStd::make_unique<DataMembers>();
            m_data->m_stubEnv.pSystem = &m_data->m_system;
            m_data->m_stubEnv.pLog = &m_data->m_log;

            gEnv = &m_data->m_stubEnv;
        }

        void TearDown() override
        {
            m_data.reset();

            // restore state.
            gEnv = m_priorEnv;
        }

        struct DataMembers
        {
            SSystemGlobalEnvironment m_stubEnv;
            NiceMock<SystemMock> m_system;
            NiceMock<LogMock> m_log;
        };

        AZStd::unique_ptr<DataMembers> m_data;
        SSystemGlobalEnvironment* m_priorEnv = nullptr;

        const char* m_message = "Test_Message";
    };

    TEST_F(AZCoreLogSinkTests, OnOutput_DefaultWindowString_CryLogAlways_FT)
    {
        AZCoreLogSink logSink;

        EXPECT_CALL(m_data->m_log, LogV(ILog::eAlways, "%s", _));

        // Copy the same string to a different memory address to test that its actually doing a strcmp
        AZStd::string defaultWindowStr = AZ::Debug::Trace::GetDefaultSystemWindow();

        EXPECT_TRUE(logSink.OnOutput(defaultWindowStr.c_str(), m_message));
    }

    TEST_F(AZCoreLogSinkTests, OnOutput_NonDefaultWindowString_CryLog_FT)
    {
        AZCoreLogSink logSink;

        EXPECT_CALL(m_data->m_log, LogV(ILog::eMessage, "(%s) - %s", _));

        EXPECT_TRUE(logSink.OnOutput("NonDefaultWindowStr", m_message));
    }
} // end namespace AZCoreLogSinkTests
