
#pragma once

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/ImGuiEditorWindowBus.h>
#include "ImGuiManager.h"
#include "ImGuiBus.h"

namespace ImGui
{
    class ImGuiEditorWindowSystemComponent
        : public AZ::Component
        , protected ImGuiEditorWindowRequestBus::Handler
        , public ImGuiUpdateListenerBus::Handler
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(ImGuiEditorWindowSystemComponent, "{91021F3E-B5F0-4E26-A7C9-6ED0F6CB6C5A}");

        virtual ~ImGuiEditorWindowSystemComponent();

        static void Reflect(AZ::ReflectContext* context);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ImGuiEditorWindowRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ImGuiUpdateListenerBus interface implementation
        void OnOpenEditorWindow() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorEvents
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
