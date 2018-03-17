
#include "CryLegacyAnimation_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "CryLegacyAnimationSystemComponent.h"

#include <IGem.h>

namespace CryLegacyAnimation
{
    class CryLegacyAnimationModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CryLegacyAnimationModule, "{C63A0203-24A1-4375-852B-CBD1827B937E}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(CryLegacyAnimationModule, AZ::SystemAllocator, 0);

        CryLegacyAnimationModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CryLegacyAnimationSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CryLegacyAnimationSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CryLegacyAnimation_b4807336b070494d974275a581593ed8, CryLegacyAnimation::CryLegacyAnimationModule)
