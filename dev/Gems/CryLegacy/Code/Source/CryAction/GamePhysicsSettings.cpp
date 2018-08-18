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
#include "GamePhysicsSettings.h"

void GamePhysicsSettings::ExportCollisionClassesToLua()
{
    IScriptSystem* pScriptSystem = gEnv->pScriptSystem;
    IScriptTable* collisionClassTable = pScriptSystem->CreateTable();
    collisionClassTable->AddRef();
    collisionClassTable->BeginSetGetChain();

    for (int i = 0; i < IGamePhysicsSettings::kNumCollisionClasses; i++)
    {
        const char* collisionName = GetCollisionClassName(i);

        if (collisionName != nullptr && collisionName[0] != '\0')
        {
            if (i >= IGamePhysicsSettings::kNumCollisionClasses)
            {
                CRY_ASSERT_MESSAGE(false, "Collision class index out of bounds");
                continue;
            }

            stack_string name;
            name.Format("bT_%s", collisionName);
            collisionClassTable->SetValueChain(name.c_str(), 1 << i);
        }
    }

    collisionClassTable->EndSetGetChain();
    pScriptSystem->SetGlobalValue("g_PhysicsCollisionClass", collisionClassTable);
    collisionClassTable->Release();

    for (int i = 0; i < IGamePhysicsSettings::kNumCollisionClasses; i++)
    {
        const char* collisionName = GetCollisionClassName(i);

        if (collisionName != nullptr && collisionName[0] != '\0')
        {
            pScriptSystem->SetGlobalValue(collisionName, 1 << i);
        }
    }
}
