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

#pragma once

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

namespace EMotionFX
{
    class CommandRunnerFixture
        : public UIFixture
        , public ::testing::WithParamInterface<std::vector<std::string>>
    {
    };

    TEST_P(CommandRunnerFixture, RunCommands)
    {
        AZStd::string result;
        for (const auto& commandStr : GetParam())
        {
            if (commandStr == "UNDO")
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << "Undo: " << result.c_str();
            }
            else if (commandStr == "REDO")
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->Redo(result)) << "Redo: " << result.c_str();
            }
            else
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(commandStr.c_str(), result)) << commandStr.c_str() << ": " << result.c_str();
            }
        }
    }
} // end namespace EMotionFX
