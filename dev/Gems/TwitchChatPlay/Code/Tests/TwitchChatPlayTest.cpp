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

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <TwitchChatPlay/TwitchChatPlayBus.h>
#include <Websockets/WebsocketsBus.h>

class TwitchChatPlayTestStub
    : public UnitTest::AllocatorsTestFixture
{
public:
    void CreateClient()
    {

    }
protected:
    void SetUp() override
    {
        UnitTest::AllocatorsTestFixture::SetUp();
    }

    void TearDown() override
    {
        UnitTest::AllocatorsTestFixture::TearDown();
    }
};

TEST_F(TwitchChatPlayTestStub, TestLogin)
{

}

AZ_UNIT_TEST_HOOK();
