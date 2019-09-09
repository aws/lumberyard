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

#include "StdAfx.h"
#include "ImGuiLYCommonMenu.h"

#ifdef IMGUI_ENABLED
namespace ImGui
{
    ImGuiLYCommonMenu::ImGuiLYCommonMenu() 
    {
    }
    ImGuiLYCommonMenu::~ImGuiLYCommonMenu() 
    {
    }

    void ImGuiLYCommonMenu::Initialize()
    {
        // Connect EBusses
        ImGuiUpdateListenerBus::Handler::BusConnect();

        // init entity outliner
        m_entityOutliner.Initialize();
    }

    void ImGuiLYCommonMenu::Shutdown()
    {
        // Disconnect EBusses
        ImGuiUpdateListenerBus::Handler::BusDisconnect();

        // shutdown entity outliner
        m_entityOutliner.Shutdown();
    }

    void ImGuiLYCommonMenu::OnImGuiUpdate()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("LY Common Tools"))
            {
                if (ImGui::MenuItem("Entity Outliner"))
                {
                    m_entityOutliner.ToggleEnabled();
                }

                ImGui::EndMenu();
            }
            ImGuiUpdateListenerBus::Broadcast(&IImGuiUpdateListener::OnImGuiMainMenuUpdate);
            ImGui::EndMainMenuBar();
        }

        // Update entity outliner ( will only do stuff if enabled )
        m_entityOutliner.ImGuiUpdate();
    }
} // namespace ImGui

#endif // IMGUI_ENABLED