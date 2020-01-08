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
#ifndef __IMGUI_MANAGER_H__
#define __IMGUI_MANAGER_H__

#pragma once

#ifdef IMGUI_ENABLED

#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Input/Events/InputTextEventListener.h>
#include "IGame.h"
#include "IGameFramework.h"
#include <IInput.h>
#include <ImGui/imgui.h>
#include <ImGuiBus.h>
#include <VertexFormats.h>

#if defined(LOAD_IMGUI_LIB_DYNAMICALLY)
#include <AzCore/Module/DynamicModuleHandle.h>
#endif // defined(LOAD_IMGUI_LIB_DYNAMICALLY)

namespace ImGui
{
    class ImGuiManager : public IGameFrameworkListener
                       , public AzFramework::InputChannelEventListener
                       , public AzFramework::InputTextEventListener
                       , public ImGuiManagerListenerBus::Handler
    {
    public:
        void Initialize();
        void Shutdown();

        // This is called by the ImGuiGem to Register CVARs at the correct time
        void RegisterImGuiCVARs();
    protected:
        void RenderImGuiBuffers(const ImVec2& scaleRects);

        // -- ImGuiManagerListenerBus Interface -------------------------------------------------------------------
        DisplayState GetEditorWindowState() const override { return m_editorWindowState; }
        void SetEditorWindowState(DisplayState state) override { m_editorWindowState = state; }
        DisplayState GetClientMenuBarState() const override { return m_clientMenuBarState; }
        void SetClientMenuBarState(DisplayState state) override { m_clientMenuBarState = state; }
        bool IsControllerSupportModeEnabled(ImGuiControllerModeFlags::FlagType controllerMode) const override;
        void EnableControllerSupportMode(ImGuiControllerModeFlags::FlagType controllerMode, bool enable) override;
        void SetControllerMouseSensitivity(float sensitivity) { m_controllerMouseSensitivity = sensitivity; }
        float GetControllerMouseSensitivity() const { return m_controllerMouseSensitivity; }
        bool GetEnableDiscreteInputMode() const override { return m_enableDiscreteInputMode; }
        void SetEnableDiscreteInputMode(bool enabled) override { m_enableDiscreteInputMode = enabled; }
        ImGuiResolutionMode GetResolutionMode() const override { return m_resolutionMode; }
        void SetResolutionMode(ImGuiResolutionMode mode) override { m_resolutionMode = mode; }
        const ImVec2& GetImGuiRenderResolution() const override { return m_renderResolution; }
        void SetImGuiRenderResolution(const ImVec2& res) override { m_renderResolution = res; }
        // -- ImGuiManagerListenerBus Interface -------------------------------------------------------------------

        // -- IGameFrameworkListener Interface --------------------------------------------------------------------
        void OnPostUpdate(float fDeltaTime) override;
        void OnPreRender() override;
        // -- IGameFrameworkListener Interface --------------------------------------------------------------------

        // -- AzFramework::InputChannelEventListener and AzFramework::InputTextEventListener Interface ------------
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
        bool OnInputTextEventFiltered(const AZStd::string& textUTF8) override;
        int GetPriority() const override { return AzFramework::InputChannelEventListener::GetPriorityDebug(); }
        // -- AzFramework::InputChannelEventListener and AzFramework::InputTextEventListener Interface ------------

        // A function to toggle through the available ImGui Visibility States
        void ToggleThroughImGuiVisibleState(int controllerIndex);

    private:
        int m_fontTextureId = -1;
        DisplayState m_clientMenuBarState = DisplayState::Hidden;
        DisplayState m_editorWindowState = DisplayState::Hidden;

        // ImGui Resolution Settings
        ImGuiResolutionMode m_resolutionMode = ImGuiResolutionMode::MatchToMaxRenderResolution;
        ImVec2 m_renderResolution = ImVec2(1920.0f, 1080.0f);
        ImVec2 m_lastRenderResolution;

        // Rendering buffers
        std::vector<SVF_P3F_C4B_T2F> m_vertBuffer;
        std::vector<uint16> m_idxBuffer;

        //Controller navigation
        static const int MaxControllerNumber = 4;
        int m_currentControllerIndex;
        bool m_button1Pressed, m_button2Pressed, m_menuBarStatusChanged;

        bool m_hardwardeMouseConnected = false;
        bool m_enableDiscreteInputMode = false;
        ImGuiControllerModeFlags::FlagType m_controllerModeFlags = 0;
        float m_controllerMouseSensitivity = 4.0f;
        float m_controllerMousePosition[2] = { 0.0f, 0.0f };
        float m_lastPrimaryTouchPosition[2] = { 0.0f, 0.0f };
        bool m_useLastPrimaryTouchPosition = false;
        bool m_simulateBackspaceKeyPressed = false;

#if defined(LOAD_IMGUI_LIB_DYNAMICALLY)
        AZStd::unique_ptr<AZ::DynamicModuleHandle>  m_imgSharedLib;
#endif // defined(LOAD_IMGUI_LIB_DYNAMICALLY)
    };

    static void AddMenuItemHelper(bool& control, const char* show, const char* hide)
    {
        if (control)
        {
            if (ImGui::MenuItem(hide))
            {
                control = false;
            }
        }
        else if (ImGui::MenuItem(show))
        {
            control = true;
        }
    }
}

#endif // ~IMGUI_ENABLED
#endif //__IMGUI_MANAGER_H__
