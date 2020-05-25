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

#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <SourceControl/PerforceComponent.h>
#include <SourceControl/PerforceConnection.h>

namespace UnitTest
{
    struct MockPerforceComponent
        : AzToolsFramework::PerforceComponent
    {
        friend struct PerforceComponentFixture;
    };

    struct MockPerforceCommand
        : AzToolsFramework::PerforceCommand
    {
        void ExecuteCommand() override
        {
            m_rawOutput.Clear();
            m_applicationFound = true;

            if(m_commandArgs == "set")
            {
                m_rawOutput.outputResult = R"(
P4PORT=ssl:unittest.amazon.com:1666 (set)
                )";
            }
            else if(m_commandArgs == "info")
            {
                m_rawOutput.outputResult = R"(
... userName unittest
... clientName unittest
... clientRoot c:\depot
... clientLock none
... clientCwd c:\depot\dev\Engine\Fonts
... clientHost unittest
... clientCase insensitive
... peerAddress 127.0.0.1:64565
... clientAddress 127.0.0.1
... serverName unittest
... lbr.replication readonly
... monitor enabled
... security enabled
... externalAuth enabled
... serverAddress unittest.amazon.com:1666
... serverRoot /data/repos/p4root/root
... serverDate 2020/01/01 10:00:00 -0500 PST
... tzoffset -28800
... serverUptime 1234:12:34
... serverVersion P4D/LINUX33X86_64/2020.4/1234567 (2020/06/06)
... serverEncryption encrypted
... serverCertExpires Dec 24 04:10:00 3030 GMT
... ServerID unittest
... serverServices edge-server
... authServer ssl:127.0.0.1:1666
... changeServer ssl:127.0.0.1:1666
... serverLicense ssl:127.0.0.1:1666
... caseHandling insensitive
... replica ssl:127.0.0.1:1666)";
            }
            else if(m_commandArgs.starts_with("fstat"))
            {
                m_rawOutput.outputResult = m_fstatResponse;
                m_rawOutput.errorResult = m_fstatErrorResponse;

                m_fstatResponse = m_fstatErrorResponse = "";
            }
        }

        void ExecuteRawCommand() override
        {
            ExecuteCommand();
        }

        AZStd::string m_fstatResponse;
        AZStd::string m_fstatErrorResponse;
    };

    struct MockPerforceConnection
        : AzToolsFramework::PerforceConnection
    {
        MockPerforceConnection(MockPerforceCommand& command) : PerforceConnection(command)
        {
        }
    };

    struct PerforceComponentFixture
        : ::testing::Test
    , AzToolsFramework::SourceControlNotificationBus::Handler

    {
        void SetUp() override
        {
            BusConnect();

            AZ::TickBus::AllowFunctionQueuing(true);

            m_perforceComponent = AZStd::make_unique<MockPerforceComponent>();
            m_perforceComponent->Activate();
            m_perforceComponent->SetConnection(new MockPerforceConnection(m_command));

            AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, true);

            WaitForSourceControl(m_connectSignal);
            ASSERT_TRUE(m_connected);
        }

        void TearDown() override
        {
            BusDisconnect();

            m_perforceComponent->Deactivate();
            m_perforceComponent = nullptr;
        }

        void ConnectivityStateChanged(const AzToolsFramework::SourceControlState state) override
        {
            m_connected = (state == AzToolsFramework::SourceControlState::Active);
            m_connectSignal.release();
        }

        static void WaitForSourceControl(AZStd::binary_semaphore& waitSignal)
        {
            int retryLimit = 500;

            do
            {
                AZ::TickBus::ExecuteQueuedEvents();
                --retryLimit;
            } while (!waitSignal.try_acquire_for(AZStd::chrono::milliseconds(10)) && retryLimit >= 0);

            ASSERT_GE(retryLimit, 0);
        }

        bool m_connected = false;
        AZStd::binary_semaphore m_connectSignal;
        AZStd::unique_ptr<MockPerforceComponent> m_perforceComponent;
        MockPerforceCommand m_command;
    };

    TEST_F(PerforceComponentFixture, TestGetBulkFileInfo_MultipleFiles_Succeeds)
    {
        static constexpr char FileAPath[] = R"(C:\depot\dev\default.font)";
        static constexpr char FileBPath[] = R"(C:\depot\dev\default.xml)";

        m_command.m_fstatResponse =
            R"(... depotFile //depot/dev/default.xml)" "\r\n"
            R"(... clientFile C:\depot\dev\default.xml)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... headAction integrate)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1454346715)" "\r\n"
            R"(... headRev 3)" "\r\n"
            R"(... headChange 147109)" "\r\n"
            R"(... headModTime 1452731919)" "\r\n"
            R"(... haveRev 3)" "\r\n"
            "\r\n"
            R"(... depotFile //depot/dev/default.font)" "\r\n"
            R"(... clientFile C:\depot\dev\default.font)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... headAction branch)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1479280355)" "\r\n"
            R"(... headRev 1)" "\r\n"
            R"(... headChange 317116)" "\r\n"
            R"(... headModTime 1478804078)" "\r\n"
            R"(... haveRev 1)" "\r\n"
            "\r\n";

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { FileAPath, FileBPath };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetBulkFileInfo, requestFiles, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), 2);

        for (int i = 0; i < 2; ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, AzToolsFramework::SourceControlStatus::SCS_OpSuccess);
            ASSERT_TRUE(fileInfo[i].IsManaged());
        }
    }

    TEST_F(PerforceComponentFixture, TestGetBulkFileInfo_MissingFile_Succeeds)
    {
        static constexpr char FileAPath[] = R"(C:\depot\dev\does-not-exist.txt)";
        static constexpr char FileBPath[] = R"(C:\depot\dev\does-not-exist-two.txt)";

        m_command.m_fstatErrorResponse =
            R"(C:\depot\dev\does-not-exist.txt - no such file(s).)" "\r\n"
            R"(C:\depot\dev\does-not-exist-two.txt - no such file(s).)" "\r\n"
            "\r\n";

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { FileAPath, FileBPath };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetBulkFileInfo, requestFiles, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), 2);

        for (int i = 0; i < 2; ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, AzToolsFramework::SourceControlStatus::SCS_OpSuccess);
            ASSERT_EQ(fileInfo[i].m_flags, AzToolsFramework::SourceControlFlags::SCF_Writeable); // Writable should be the only flag
        }
    }

    TEST_F(PerforceComponentFixture, TestGetBulkFileInfo_CompareWithGetFileInfo_ResultMatches)
    {
        static constexpr char FileAPath[] = R"(C:\depot\dev\default.font)";

        static constexpr char FstatResponse[] =
            R"(... depotFile //depot/dev/default.font)" "\r\n"
            R"(... clientFile C:\depot\dev\default.font)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... headAction branch)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1479280355)" "\r\n"
            R"(... headRev 1)" "\r\n"
            R"(... headChange 317116)" "\r\n"
            R"(... headModTime 1478804078)" "\r\n"
            R"(... haveRev 1)" "\r\n"
            "\r\n";

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AzToolsFramework::SourceControlFileInfo fileInfoSingle;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto singleCallback = [&callbackSignal, &result, &fileInfoSingle](bool success, AzToolsFramework::SourceControlFileInfo info)
        {
            result = success;
            fileInfoSingle = info;
            callbackSignal.release();
        };

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { FileAPath };

        m_command.m_fstatResponse = FstatResponse;
        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetBulkFileInfo, requestFiles, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);

        m_command.m_fstatResponse = FstatResponse;
        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetFileInfo, FileAPath, singleCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_FALSE(fileInfo.empty());
        ASSERT_EQ(fileInfoSingle.m_flags, fileInfo[0].m_flags);
    }

}
