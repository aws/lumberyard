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
#include <AzTest/AzTest.h>
#include "GameObjects/GameObjectSystem.h"


TEST(CryGameObjectSystemTests, EmptyExtensionInfoTest_FT)
{
    //game object system instance with empty extension info
    CGameObjectSystem gameObjSys;

    //this should return 0xFFFFFFFF when trying to get element greater the array length
    EXPECT_EQ(gameObjSys.GetExtensionSerializationPriority(0), 0xFFFFFFFF);

    //get extension info name at first element, the array is empty so we expect a null
    const char* objName = gameObjSys.GetName(0);
    EXPECT_TRUE(objName == NULL);

    //attempt to instantiate using extension info at index 0, which doesn't exist so this should return a nullptr
    IGameObjectExtensionPtr objPtr = gameObjSys.Instantiate(0, NULL);
    EXPECT_TRUE(objPtr == NULL);
}