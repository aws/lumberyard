/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "LevelBuilderComponent.h"
#include <AzCore/Component/Entity.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

void BuilderOnInit()
{
}

void BuilderDestroy()
{
}

void BuilderRegisterDescriptors()
{
    AssetBuilderSDK::AssetBuilderBus::Broadcast(
        &AssetBuilderSDK::AssetBuilderBus::Events::RegisterComponentDescriptor,
        LevelBuilder::LevelBuilderComponent::CreateDescriptor());
}

void BuilderAddComponents(AZ::Entity* entity)
{
    if (!entity)
    {
        return;
    }
    // BuilderComponents can be added in multiple ways: here, or with a special tag in the reflect function.
    // The LevelBuilderComponent is added here because it matches the pattern of other builders in the AssetProcessor\Builders
    // folder. Adding it via tags in the reflect function caused asset processing to fail.
    entity->CreateComponentIfReady<LevelBuilder::LevelBuilderComponent>();
}

REGISTER_ASSETBUILDER