
#include "StdAfx.h"
#include <platform_impl.h>

#include "ImGuiEditorWindowSystemComponent.h"
#include <IGem.h>


namespace ImGui
{
    class ImGuiEditorWindowModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(ImGuiEditorWindowModule, "{DDC7A763-A36F-46D8-9885-43E0293C1D03}", CryHooksModule);

        ImGuiEditorWindowModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ImGuiEditorWindowSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ImGuiEditorWindowSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(ImGuiEditorWindow_c7b2b62e224d42418dfa5b6fad36c5cc, ImGui::ImGuiEditorWindowModule)
