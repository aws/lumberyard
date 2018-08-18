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

#include "Entity.h"

#pragma warning( push )
#pragma warning( disable: 4800 )    // 'int' : forcing value to bool 'true' or 'false' (performance warning)


INTEG_TEST(EntityTests, RegisterComponent_EntityHasComponent_ReturnsFalse)
{
    SEntitySpawnParams spawnParams;
    spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();
    spawnParams.sName = "_testEntity";
    spawnParams.nFlags = 0;

    IEntity* testEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams, true);
    gEnv->pEntitySystem->CreateComponentAndRegister<IComponentRender>(IComponent::SComponentInitializer(testEntity));

    // Try getting an existing component.
    EXPECT_TRUE(testEntity->GetComponent<IComponentRender>().get());

    // Try registering a duplicate component
    EXPECT_FALSE(testEntity->RegisterComponent(testEntity->GetComponent<IComponentRender>()));

    gEnv->pEntitySystem->RemoveEntity(testEntity->GetId(), true);
}

#pragma warning( pop )