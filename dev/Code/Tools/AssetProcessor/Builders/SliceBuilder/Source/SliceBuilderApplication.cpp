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

#include <SliceBuilder/Source/SliceBuilderComponent.h>
#include <AzCore/Component/Entity.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/UiAssetTypes.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>

void BuilderOnInit()
{
}

void BuilderDestroy()
{
}

void BuilderRegisterDescriptors()
{
    EBUS_EVENT(AssetBuilderSDK::AssetBuilderBus, RegisterComponentDescriptor, SliceBuilder::BuilderPluginComponent::CreateDescriptor());
}

void BuilderAddComponents(AZ::Entity* entity)
{
    entity->CreateComponentIfReady<SliceBuilder::BuilderPluginComponent>();

    // Add the LyShineSystemComponent to an entity and activate it so that it connects to
    // the UiSystemToolsBus which we will use to load the canvas.
    // Don't do it if the LyShineSystemComponent hasn't been registered
    if (AZ::ComponentDescriptorBus::GetNumOfEventHandlers(LyShine::lyShineSystemComponentUuid) > 0)
    {
        entity->CreateComponent(LyShine::lyShineSystemComponentUuid);
    }

    // Add the ScriptCanvas Builder System Component to an entity so that the AssetHandler is registered with the AssetManager
    // Furthermore this also registers the ScriptCanvas asset type as a source asset type so that it is opened in the source asset folder
    AZ::Component* scriptCanvasBuilderComponent{};
    AZ::ComponentDescriptorBus::EventResult(scriptCanvasBuilderComponent, AZ::Uuid("{2FB1C848-B863-4562-9C4B-01E18BD61583}"), &AZ::ComponentDescriptorBus::Events::CreateComponent); // {2FB1C848-B863-4562-9C4B-01E18BD61583} is the ScriptCanvas BuilderSystemComponent
    if (scriptCanvasBuilderComponent)
    {
        entity->AddComponent(scriptCanvasBuilderComponent); 
    }

    // Add the GenericComponentUnwrapper for slice processing
    entity->CreateComponent(azrtti_typeid<AzToolsFramework::Components::GenericComponentUnwrapper>());
}

// we must use this macro to register this as an assetbuilder
REGISTER_ASSETBUILDER
