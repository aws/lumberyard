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

#include "Twitch_precompiled.h"
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/IO/SystemFile.h>
#include <AzTest/AzTest.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include "TwitchReflection.h"

class Integ_TwitchTest
    : public ::testing::Test
{
protected:
    using ScriptContextPtr = AZStd::shared_ptr<AZ::ScriptContext>;

public:
    Integ_TwitchTest()
        : m_script( AZStd::make_shared<AZ::ScriptContext>() )
    {
    }

    virtual ~Integ_TwitchTest()
    {
    }

    void SetUp() override
    {
        AZ::BehaviorContext behaviorContext;
        Twitch::Internal::Reflect(behaviorContext);

        m_script->BindTo(&behaviorContext);
    }

    bool RunTest()
    {
       bool success = false;
       AZStd::string scriptBody( LoadScript("Scripts\\Twitch.lua") );

       if( !scriptBody.empty() )
       {
           success = m_script->Execute(scriptBody.c_str(), "Twitch.lua", scriptBody.size());
       }

       return success;
    }

    AZStd::string LoadScript(const AZStd::string & scriptName) const
    {
        AZStd::string scriptBody;
        AZ::IO::SystemFile file;

        if(file.Open(scriptName.c_str(), AZ::IO::SystemFile::OpenMode::SF_OPEN_READ_ONLY) )
        {
            AZ::u64 fileSize = file.Length();

            if(fileSize > 0)
            {
                AZStd::string temp(static_cast<size_t>(fileSize), '0');

                AZ::u64 dataRead = file.Read(fileSize, reinterpret_cast<void *>(temp.data()) );

                if (dataRead == fileSize)
                {
                    scriptBody.swap(temp);
                }
            }
        }
    
        return scriptBody;        
    }

private:
    ScriptContextPtr       m_script;
};


TEST_F(Integ_TwitchTest, TwitchScriptTest)
{
    bool success = RunTest();

    EXPECT_NE(success, true);
}


class TwitchTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(TwitchTest, ExampleTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
