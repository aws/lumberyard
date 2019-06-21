
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <EmptySystemComponent.h>

namespace Empty
{
    class EmptyModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(EmptyModule, "{FE2F715A-7DE5-43BD-B24C-421F444F9D98}", AZ::Module);
        AZ_CLASS_ALLOCATOR(EmptyModule, AZ::SystemAllocator, 0);

        EmptyModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                EmptySystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<EmptySystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Empty_bb8388ecc5f048ad983648900478cf81, Empty::EmptyModule)
