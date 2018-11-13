
#include "AWSLambdaLanguageDemo_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "AWSLambdaLanguageDemoSystemComponent.h"

#include <IGem.h>

namespace AWSLambdaLanguageDemo
{
    class AWSLambdaLanguageDemoModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSLambdaLanguageDemoModule, "{7235403F-D1C0-46A2-89B6-F00C10E52D35}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSLambdaLanguageDemoModule, AZ::SystemAllocator, 0);

        AWSLambdaLanguageDemoModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AWSLambdaLanguageDemoSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AWSLambdaLanguageDemoSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(AWSLambdaLanguageDemo_35d2c0caf522417398ce5807cbedb60c, AWSLambdaLanguageDemo::AWSLambdaLanguageDemoModule)
