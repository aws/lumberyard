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

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneData/ManifestMetaInfoHandler.h>
#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/Behaviors/Registry.h>

static AZ::SceneAPI::SceneData::ManifestMetaInfoHandler* g_manifestMetaInfoHandler = nullptr;
static AZ::SceneAPI::SceneData::Registry* g_behaviorRegistry = nullptr;
static bool g_hasBeenReflected = false;

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env)
{
    AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));
    
    if (!g_manifestMetaInfoHandler)
    {
        g_manifestMetaInfoHandler = aznew AZ::SceneAPI::SceneData::ManifestMetaInfoHandler();
    }
    if (!g_behaviorRegistry)
    {
        g_behaviorRegistry = aznew AZ::SceneAPI::SceneData::Registry();
    }
}

extern "C" AZ_DLL_EXPORT void Reflect(AZ::SerializeContext* context)
{
    if (!context)
    {
        EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    }
    if (context)
    {
        AZ::SceneAPI::RegisterDataTypeReflection(context);
        
        if (!g_hasBeenReflected)
        {
            AZ::SceneAPI::SceneData::Registry::ComponentDescriptorList components;
            AZ::SceneAPI::SceneData::Registry::RegisterComponents(components);
            for (AZ::ComponentDescriptor* descriptor : components)
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::RegisterComponentDescriptor, descriptor);
            }
            // There's no need to keep track of the component descriptor pointers. The descriptors register themselves with the ComponentDescriptorBus
            //      which calls them to unregister and delete themselves upon application termination.

            g_hasBeenReflected = true;
        }
    }
}

extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    delete g_behaviorRegistry;
    g_behaviorRegistry = nullptr;

    delete g_manifestMetaInfoHandler;
    g_manifestMetaInfoHandler = nullptr;

    AZ::Environment::Detach();
}

#endif // !defined(AZ_MONOLITHIC_BUILD)
