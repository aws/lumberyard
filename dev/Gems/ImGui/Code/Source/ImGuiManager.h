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

#include "IGame.h"
#include "IGameFramework.h"
#include <VertexFormats.h>
#include <IInput.h>
#include <imgui/imgui.h>

#include <ImGuiBus.h>

#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Input/Events/InputTextEventListener.h>

#if defined(LOAD_IMGUI_LIB_DYNAMICALLY)
#include <AzCore/Module/DynamicModuleHandle.h>
#endif // defined(LOAD_IMGUI_LIB_DYNAMICALLY)

namespace ImGui
{
    class ImGuiManager : public IGameFrameworkListener,
                         public AzFramework::InputChannelEventListener,
                         public AzFramework::InputTextEventListener,
                         public ImGuiManagerListenerBus::Handler
    {
    public:
        void Initialize();
        void Shutdown();

    protected:
        void ResetInput();
        void RenderImGuiBuffers();

        // ImGuiManagerListenerBus
        virtual DisplayState GetEditorWindowState() override;
        virtual void SetEditorWindowState(DisplayState state) override;

        // IGameFrameworkListener
        //void OnPreUpdate(float fDeltaTime) override;
        void OnPostUpdate(float fDeltaTime) override;
        void OnPreRender() override;

        // AzFramework::InputChannelEventListener and AzFramework::InputTextEventListener
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
        bool OnInputTextEventFiltered(const AZStd::string& textUTF8) override;
        int GetPriority() const override { return AzFramework::InputChannelEventListener::GetPriorityDebug(); }

    private:
        int m_fontTextureId = -1;
        DisplayState m_clientMenuBarState = DisplayState::Hidden;
        DisplayState m_editorWindowState = DisplayState::Hidden;

        // Rendering buffers
        std::vector<SVF_P3F_C4B_T2F> m_vertBuffer;
        std::vector<uint16> m_idxBuffer;

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
