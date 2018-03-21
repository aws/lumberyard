
#include "CloudGemFramework_precompiled.h"
#include <platform_impl.h>

#include <CloudGemFrameworkModule.h>

#include "CloudGemFrameworkSystemComponent.h"

#include <CloudGemFramework/HttpClientComponent.h>
#include <MappingsComponent.h>
#include <PlayerIdentityComponent.h>
#include <IGem.h>

namespace CloudGemFramework
{
    CloudGemFrameworkModule::CloudGemFrameworkModule()
        : CryHooksModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            CloudGemFrameworkSystemComponent::CreateDescriptor(),
            HttpClientComponent::CreateDescriptor(),
            CloudCanvasMappingsComponent::CreateDescriptor(),
            CloudCanvasPlayerIdentityComponent::CreateDescriptor(),
        });
    }

    /**
        * Add required SystemComponents to the SystemEntity.
        */
    AZ::ComponentTypeList CloudGemFrameworkModule::GetRequiredSystemComponents() const 
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<CloudGemFrameworkSystemComponent>(),
            azrtti_typeid<CloudCanvasMappingsComponent>(),
            azrtti_typeid<CloudCanvasPlayerIdentityComponent>(),
        };
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
#if !defined(CLOUD_GEM_FRAMEWORK_EDITOR)
AZ_DECLARE_MODULE_CLASS(CloudGemFramework_6fc787a982184217a5a553ca24676cfa, CloudGemFramework::CloudGemFrameworkModule)
#endif
