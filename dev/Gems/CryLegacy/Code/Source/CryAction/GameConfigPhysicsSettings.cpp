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
#include "CryLegacy_precompiled.h"
#include "GameConfigPhysicsSettings.h"
#include "ISystem.h"
#include <stdio.h>
#include <cstdlib>
#include <AzFramework/StringFunc/StringFunc.h>

void GameConfigPhysicsSettings::Init()
{
    REGISTER_STRING("collision_classes", "", 0, "Specifies collision classes");

    ICVar* collisionClassesVar = gEnv->pConsole->GetCVar("collision_classes");

    if (collisionClassesVar == nullptr)
    {
        CRY_ASSERT_MESSAGE(false, "collision_classes cvar could not be found");
        return;
    }

    AZStd::vector<AZStd::string> collisionClassPairList;
    AzFramework::StringFunc::Tokenize(collisionClassesVar->GetString(), collisionClassPairList, ",");

    CRY_ASSERT_MESSAGE(collisionClassPairList.size() <= IGamePhysicsSettings::kNumCollisionClasses, "Too many collision classes specified in config file");

    AZStd::vector<AZStd::string> collisionClassPair;
    collisionClassPair.reserve(2);

    for (size_t i = 0; i < collisionClassPairList.size() && i < IGamePhysicsSettings::kNumCollisionClasses; ++i)
    {
        AzFramework::StringFunc::Tokenize(collisionClassPairList[i].c_str(), collisionClassPair, "=");

        if (collisionClassPair.size() >= 2)
        {
            int bitIndex = std::atoi(collisionClassPair[1].c_str());

            CRY_ASSERT_MESSAGE(bitIndex >= 0 && bitIndex < kNumCollisionClasses, "Specified bit index for collision class is out of bounds");

            if (bitIndex >= 0 && bitIndex < kNumCollisionClasses)
            {
                m_collisionClasses[bitIndex] = string(collisionClassPair[0].c_str());
            }
        }

        collisionClassPair.clear();
    }

    ExportCollisionClassesToLua();
}

const char* GameConfigPhysicsSettings::GetCollisionClassName(unsigned int bitIndex)
{
    return bitIndex < kNumCollisionClasses ? m_collisionClasses[bitIndex].c_str() : "";
}