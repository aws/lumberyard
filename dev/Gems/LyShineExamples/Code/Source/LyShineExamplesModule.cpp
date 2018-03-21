
#include "LyShineExamples_precompiled.h"
#include <platform_impl.h>

#include "LyShineExamplesSystemComponent.h"
#include "UiTestScrollBoxDataProviderComponent.h"
#include "UiCustomImageComponent.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <IGem.h>

namespace LyShineExamples
{
    class LyShineExamplesModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(LyShineExamplesModule, "{BC028F50-D2C4-4A71-84D1-F1BDC727019A}", CryHooksModule);

        LyShineExamplesModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                LyShineExamplesSystemComponent::CreateDescriptor(),
                UiTestScrollBoxDataProviderComponent::CreateDescriptor(),
                UiCustomImageComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LyShineExamplesSystemComponent>(),
            };
        }

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override
        {
            switch (event)
            {
            case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
                RegisterExternalFlowNodes();
                break;
            }
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(LyShineExamples_c7935ecf5e8047fe8ca947b34b11cadb, LyShineExamples::LyShineExamplesModule)
