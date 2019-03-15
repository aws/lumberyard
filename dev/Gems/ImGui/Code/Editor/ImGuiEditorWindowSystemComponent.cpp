
#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "ImGuiEditorWindowSystemComponent.h"
#include "ImGuiMainWindow.h"
#include <AzToolsFramework/API/ViewPaneOptions.h>

static const char* s_ImGuiQtViewPaneName = "ImGui Editor";

namespace ImGui
{
    void ImGuiEditorWindowSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ImGuiEditorWindowSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    void ImGuiEditorWindowSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        ImGuiEditorWindowRequestBus::Handler::BusConnect();
        ImGuiUpdateListenerBus::Handler::BusConnect();
    }

    void ImGuiEditorWindowSystemComponent::Deactivate()
    {
        ImGuiUpdateListenerBus::Handler::BusDisconnect();
        ImGuiEditorWindowRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    ImGuiEditorWindowSystemComponent::~ImGuiEditorWindowSystemComponent()
    {
        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, UnregisterViewPane, s_ImGuiQtViewPaneName);
    }

    void ImGuiEditorWindowSystemComponent::OnOpenEditorWindow()
    {
        EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, OpenViewPane, s_ImGuiQtViewPaneName);
    }

    void ImGuiEditorWindowSystemComponent::NotifyRegisterViews()
    {
        QtViewOptions options;
        options.canHaveMultipleInstances = false;
        AzToolsFramework::EditorRequests::Bus::Broadcast(
            &AzToolsFramework::EditorRequests::RegisterViewPane,
            s_ImGuiQtViewPaneName,
            "Tools",
            options,
            []() { return new ImGui::ImGuiMainWindow(); });
    }
}
