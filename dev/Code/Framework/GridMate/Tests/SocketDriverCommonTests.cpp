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
#include "Tests.h"

#include <GridMate/Carrier/SocketDriver.h>

namespace UnitTest
{
    class IPv4ToIPv6ConversionTests
        : public GridMateMPTestFixture
        , public UnitTest::TraceBusRedirector
    {
    public:
        IPv4ToIPv6ConversionTests()
        {
            // Ignore warnings in the unit tests
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        ~IPv4ToIPv6ConversionTests()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }
        void run()
        {
            // Value out of range
            GridMate::string ipv4 = "660.550.440.330";
            GridMate::string ipv6;
            ASSERT_FALSE(GridMate::SocketDriverCommon::IPv4ToIPv6(ipv4, ipv6));

            // Invalid character
            ipv4 = "12.23.34.25A";
            ASSERT_FALSE(GridMate::SocketDriverCommon::IPv4ToIPv6(ipv4, ipv6));

            //Invalid length
            ipv4 = "12.23.0000.34";
            ASSERT_FALSE(GridMate::SocketDriverCommon::IPv4ToIPv6(ipv4, ipv6));
            ipv4 = "120.230.255.0000";
            ASSERT_FALSE(GridMate::SocketDriverCommon::IPv4ToIPv6(ipv4, ipv6));

            // Invalid number of sections
            ipv4 = "12.23.34";
            ASSERT_FALSE(GridMate::SocketDriverCommon::IPv4ToIPv6(ipv4, ipv6));

            ipv4 = "12.23.34.250";
            ASSERT_TRUE(GridMate::SocketDriverCommon::IPv4ToIPv6(ipv4, ipv6));
            AZ_TEST_ASSERT(ipv6 == "0:0:0:0:0:FFFF:C17:22FA");
        }
    };
}

GM_TEST_SUITE(SocketDriverCommonTests)
GM_TEST(IPv4ToIPv6ConversionTests);
GM_TEST_SUITE_END()
