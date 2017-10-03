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

#include <TranslationBuilder/Source/TranslationBuilderComponent.h>
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
    // register our COMPONENTS here.  We can register as many components as we want.
    EBUS_EVENT(AssetBuilderSDK::AssetBuilderBus, RegisterComponentDescriptor, TranslationBuilder::BuilderPluginComponent::CreateDescriptor());
}

void BuilderAddComponents(AZ::Entity* entity)
{
    // we can attach any components we want to this entity, including management components.
    // however we need at least 1 component that will be the "life cycle component"
    entity->CreateComponentIfReady<TranslationBuilder::BuilderPluginComponent>();
}

// we must use this macro to register this as an assetbuilder
REGISTER_ASSETBUILDER
