
#include "Footsteps_precompiled.h"
#include <platform_impl.h>

#include "FootstepsSystemComponent.h"
#include "FootstepComponent.h"

#include <IGem.h>

namespace Footsteps
{
    class FootstepsModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(FootstepsModule, "{D0DF3F65-CF4D-4E96-849E-FFDB10543E47}", CryHooksModule);

        FootstepsModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                FootstepsSystemComponent::CreateDescriptor(),
                FootstepComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<FootstepsSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Footsteps_257393f09006498ab706cc4d4c8d4721, Footsteps::FootstepsModule)
