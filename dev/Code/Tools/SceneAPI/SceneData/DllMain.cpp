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
static AZ::SceneAPI::SceneData::Registry::ComponentDescriptorList g_componentDescriptors;

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env)
{
    if (AZ::Environment::IsReady())
    {
        return;
    }

    AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));
    
    if (!g_manifestMetaInfoHandler)
    {
        g_manifestMetaInfoHandler = aznew AZ::SceneAPI::SceneData::ManifestMetaInfoHandler();
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
    }
    
    // Descriptor registration is done in Reflect instead of Initialize because the ResourceCompilerScene initializes the libraries before
    // there's an application.
    if (g_componentDescriptors.empty())
    {
        AZ::SceneAPI::SceneData::Registry::RegisterComponents(g_componentDescriptors);
        for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::RegisterComponentDescriptor, descriptor);
        }
    }
}

extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    if (!AZ::Environment::IsReady())
    {
        return;
    }

    AZ::SerializeContext* context = nullptr;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    if (context)
    {
        context->EnableRemoveReflection();
        Reflect(context);
        context->DisableRemoveReflection();
        context->CleanupModuleGenericClassInfo();
    }

    if (!g_componentDescriptors.empty())
    {
        for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
        {
            descriptor->ReleaseDescriptor();
        }
        g_componentDescriptors.clear();
        g_componentDescriptors.shrink_to_fit();
    }

    delete g_manifestMetaInfoHandler;
    g_manifestMetaInfoHandler = nullptr;

    // This module does not own these allocators, but must clear its cached EnvironmentVariables
    // because it is linked into other modules, and thus does not get unloaded from memory always
    if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
    if (AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    }

    AZ::Environment::Detach();
}

#endif // !defined(AZ_MONOLITHIC_BUILD)
