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
#include <AzCore/Component/Component.h>
#include <AzCore/Module/Module.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include "ImGuiManager.h"

#ifdef IMGUI_ENABLED
#include "LYCommonMenu/ImGuiLYCommonMenu.h"
#endif //IMGUI_ENABLED

namespace ImGui
{
    /*!
     * The ImGui::Module class coordinates with the application
     * to reflect classes and create system components.
     */
    class ImGuiModule : public CryHooksModule
    {
    public:
        AZ_RTTI(ImGuiModule, "{ECA9F41C-716E-4395-A096-5A519227F9A4}", AZ::Module);
        ImGuiModule() : CryHooksModule() {}

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList components;
            return components;
        }

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override
        {
#ifdef IMGUI_ENABLED
            switch (event)
            {
            case ESYSTEM_EVENT_GAME_POST_INIT:
            {
                manager.Initialize();
                lyCommonMenu.Initialize();
                break;
            }
            case ESYSTEM_EVENT_FULL_SHUTDOWN:
            case ESYSTEM_EVENT_FAST_SHUTDOWN:
                manager.Shutdown();
                lyCommonMenu.Shutdown();
                break;
            }
#endif //IMGUI_ENABLED
        }

#ifdef IMGUI_ENABLED
    private:
        ImGuiLYCommonMenu lyCommonMenu;
        ImGuiManager manager;
#endif // IMGUI_ENABLED
        
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(ImGui_bab8807a1bc646b3909f3cc200ffeedf, ImGui::ImGuiModule)
