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
#include "ImGuiManager.h"
#include <AzCore/PlatformIncl.h>
#include <platform_impl.h>

#ifdef IMGUI_ENABLED

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include "IHardwareMouse.h"
#include <LyShine/Bus/UiCursorBus.h>
using namespace AzFramework;
using namespace ImGui;

const uint32_t IMGUI_WHEEL_DELTA = 120; // From WinUser.h, for Linux

/**
    An anonymous namespace containing helper functions for interoperating with AzFrameworkInput.
 */
namespace
{
    /**
        Utility function to map an AzFrameworkInput device key to its integer index.

        @param inputChannelId the ID for an AzFrameworkInput device key.
        @return the index of the indicated key, or -1 if not found.
     */
    unsigned int GetAzKeyIndex(const InputChannelId& inputChannelId)
    {
        const auto& keys = InputDeviceKeyboard::Key::All;
        const auto& it = AZStd::find(keys.cbegin(), keys.cend(), inputChannelId);
        return it != keys.cend() ? static_cast<unsigned int>(it - keys.cbegin()) : UINT_MAX;
    }

    /**
        Utility function to map an AzFrameworkInput device button to its integer index.

        @param inputChannelId the ID for an AzFrameworkInput device button.
        @return the index of the indicated button, or -1 if not found.
     */
    unsigned int GetAzButtonIndex(const InputChannelId& inputChannelId)
    {
        const auto& buttons = InputDeviceMouse::Button::All;
        const auto& it = AZStd::find(buttons.cbegin(), buttons.cend(), inputChannelId);
        return it != buttons.cend() ? static_cast<unsigned int>(it - buttons.cbegin()) : UINT_MAX;
    }
}

void ImGuiManager::Initialize()
{
    if (!gEnv || !gEnv->pGame || !gEnv->pRenderer)
    {
        AZ_Warning("%s %s", __func__, "gEnv Invalid -- Skipping ImGui Initialization.");
        return;
    }
    // Register for Game Framework Notifications
    auto framework = gEnv->pGame->GetIGameFramework();
    framework->RegisterListener(this, "ImGuiManager", FRAMEWORKLISTENERPRIORITY_HUD);
    ImGuiManagerListenerBus::Handler::BusConnect();

    // Register for Input Notifications
    InputChannelEventListener::Connect();
    InputTextEventListener::Connect();

    // Configure ImGui
#if defined(LOAD_IMGUI_LIB_DYNAMICALLY)

    AZ::OSString imgGuiLibPath = "imguilib";

    // Let the application process the path
    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::ResolveModulePath, imgGuiLibPath);
    m_imgSharedLib = AZ::DynamicModuleHandle::Create(imgGuiLibPath.c_str());
    if (!m_imgSharedLib->Load(false))
    {
        AZ_Warning("%s %s", __func__, "Unable to load " AZ_DYNAMIC_LIBRARY_PREFIX "imguilib" AZ_DYNAMIC_LIBRARY_EXTENSION "-- Skipping ImGui Initialization.");
        return;
    }

#endif // defined(LOAD_IMGUI_LIB_DYNAMICALLY)


    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";

    // clang-format off
    // Initialize Keyboard Mappings
    // ImGui requires that we pre-fill the array of special keys used for text manipulation.
    io.KeyMap[ImGuiKey_Tab]        = GetAzKeyIndex(InputDeviceKeyboard::Key::EditTab);
    io.KeyMap[ImGuiKey_LeftArrow]  = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationArrowLeft);
    io.KeyMap[ImGuiKey_RightArrow] = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationArrowRight);
    io.KeyMap[ImGuiKey_UpArrow]    = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationArrowUp);
    io.KeyMap[ImGuiKey_DownArrow]  = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationArrowDown);
    io.KeyMap[ImGuiKey_PageUp]     = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationPageUp);
    io.KeyMap[ImGuiKey_PageDown]   = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationPageDown);
    io.KeyMap[ImGuiKey_Home]       = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationHome);
    io.KeyMap[ImGuiKey_End]        = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationEnd);
    io.KeyMap[ImGuiKey_Delete]     = GetAzKeyIndex(InputDeviceKeyboard::Key::NavigationDelete);
    io.KeyMap[ImGuiKey_Backspace]  = GetAzKeyIndex(InputDeviceKeyboard::Key::EditBackspace);
    io.KeyMap[ImGuiKey_Enter]      = GetAzKeyIndex(InputDeviceKeyboard::Key::EditEnter);
    io.KeyMap[ImGuiKey_Escape]     = GetAzKeyIndex(InputDeviceKeyboard::Key::Escape);
    io.KeyMap[ImGuiKey_A]          = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericA);
    io.KeyMap[ImGuiKey_C]          = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericC);
    io.KeyMap[ImGuiKey_V]          = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericV);
    io.KeyMap[ImGuiKey_X]          = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericX);
    io.KeyMap[ImGuiKey_Y]          = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericY);
    io.KeyMap[ImGuiKey_Z]          = GetAzKeyIndex(InputDeviceKeyboard::Key::AlphanumericZ);
    // clang-format on

    // Set the Display Size
    IRenderer* renderer = gEnv->pRenderer;
    io.DisplaySize.x = static_cast<float>(renderer->GetWidth());
    io.DisplaySize.y = static_cast<float>(renderer->GetHeight());

    // Create Font Texture
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    ITexture* fontTexture =
        renderer->Create2DTexture("ImGuiFont", width, height, 1, FT_ALPHA, pixels, eTF_A8);
    if (fontTexture)
    {
        m_fontTextureId = fontTexture->GetTextureID();
        io.Fonts->SetTexID(static_cast<void*>(fontTexture));
    }

    // Broadcast ImGui Ready to Listeners
    ImGuiUpdateListenerBus::Broadcast(&IImGuiUpdateListener::OnImGuiInitialize);
}

void ImGuiManager::Shutdown()
{
    if (!gEnv || !gEnv->pGame)
    {
        AZ_Warning("%s %s", __func__, "gEnv Invalid -- Skipping ImGui Shutdown.");
        return;
    }
#if defined(LOAD_IMGUI_LIB_DYNAMICALLY)
    if (m_imgSharedLib->IsLoaded())
    {
        m_imgSharedLib->Unload();
    }
#endif

    // Unregister from Buses/Game Framework
    gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
    ImGuiManagerListenerBus::Handler::BusDisconnect();
    InputChannelEventListener::Disconnect();
    InputTextEventListener::Disconnect();

    // Destroy ImGui Font Texture
    if (gEnv->pRenderer && m_fontTextureId > 0)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->SetTexID(nullptr);
        gEnv->pRenderer->RemoveTexture(m_fontTextureId);
    }
}

DisplayState ImGuiManager::GetEditorWindowState()
{
    return m_editorWindowState;
}

void ImGuiManager::SetEditorWindowState(DisplayState state)
{
    m_editorWindowState = state;
}

//void ImGuiManager::OnPreUpdate(float fDeltaTime)
//{
//    ICVar* consoleDisabled = gEnv->pConsole->GetCVar("sys_DeactivateConsole");
//    if (consoleDisabled && consoleDisabled->GetIVal() != 0)
//    {
//        m_clientMenuBarState = DisplayState::Hidden;
//        m_editorWindowState = DisplayState::Hidden;
//    }
//
//    // Advance ImGui by Elapsed Frame Time
//    ImGuiIO& io = ImGui::GetIO();
//    io.DeltaTime = fDeltaTime;
//}

void ImGuiManager::OnPreRender()
{
    if (m_clientMenuBarState == DisplayState::Hidden && m_editorWindowState == DisplayState::Hidden)
    {
        return;
    }

    // Update Display Size
    IRenderer* renderer = gEnv->pRenderer;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = static_cast<float>(renderer->GetWidth());
    io.DisplaySize.y = static_cast<float>(renderer->GetHeight());

    // IHardwareMouse has been deprecated in Lumberyard v1.11, so once the following line starts
    // throwing a compiler error you can just remove it and uncomment the two lines after it.
    // You'll likely also have to add this include: #include <LyShine/Bus/UiCursorBus.h>
    //bool isCursorVisible =
    //    gEnv->pSystem->GetIHardwareMouse() && !gEnv->pSystem->GetIHardwareMouse()->IsHidden();
    bool isCursorVisible = false;
    UiCursorBus::BroadcastResult(isCursorVisible, &UiCursorInterface::IsUiCursorVisible);

    if (!isCursorVisible && !io.MouseDrawCursor)
    {
        ResetInput();
        io.MousePos.x = io.DisplaySize.x;
        io.MousePos.y = io.DisplaySize.y;
    }
    else
    {
        AZ::Vector2 systemCursorPositionNormalized = AZ::Vector2::CreateZero();
        InputSystemCursorRequestBus::EventResult(
            systemCursorPositionNormalized,
            InputDeviceMouse::Id,
            &InputSystemCursorRequests::GetSystemCursorPositionNormalized);
        io.MousePos.x = systemCursorPositionNormalized.GetX() * gEnv->pRenderer->GetWidth();
        io.MousePos.y = systemCursorPositionNormalized.GetY() * gEnv->pRenderer->GetHeight();
    }

    // Start New Frame
    ImGui::NewFrame();
}

void ImGuiManager::OnPostUpdate(float fDeltaTime)
{
    if (m_clientMenuBarState == DisplayState::Hidden && m_editorWindowState == DisplayState::Hidden)
    {
        return;
    }

    //// START FROM PREUPDATE
    ICVar* consoleDisabled = gEnv->pConsole->GetCVar("sys_DeactivateConsole");
    if (consoleDisabled && consoleDisabled->GetIVal() != 0)
    {
        m_clientMenuBarState = DisplayState::Hidden;
        m_editorWindowState = DisplayState::Hidden;
    }

    // Advance ImGui by Elapsed Frame Time
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = fDeltaTime;
    //// END FROM PREUPDATE

    IRenderer* renderer = gEnv->pRenderer;
    TransformationMatrices backupSceneMatrices;

    // Configure Renderer for 2D ImGui Rendering
    renderer->SetCullMode(R_CULL_DISABLE);
    renderer->Set2DMode(renderer->GetWidth(), renderer->GetHeight(), backupSceneMatrices);
    renderer->SetColorOp(eCO_REPLACE, eCO_MODULATE, eCA_Diffuse, DEF_TEXARG0);
    renderer->SetSrgbWrite(false);
    renderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);

    // Render!
    RenderImGuiBuffers();

    // Cleanup Renderer Settings
    renderer->Unset2DMode(backupSceneMatrices);
}

/**
    @return true if input should be consumed, false otherwise.
 */
bool ImGuiManager::OnInputChannelEventFiltered(const InputChannel& inputChannel)
{
    ImGuiIO& io = ImGui::GetIO();
    const InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
    const InputDeviceId& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();

    // Handle Keyboard Hotkeys
    if (inputDeviceId == InputDeviceKeyboard::Id && inputChannel.IsStateBegan())
    {
        // Cycle through ImGui Menu Bar States
        if (inputChannelId == InputDeviceKeyboard::Key::NavigationHome)
        {
            // clang-format off
            switch (m_clientMenuBarState)
            {
            case DisplayState::Hidden:  m_clientMenuBarState = DisplayState::Visible;        break;
            case DisplayState::Visible: m_clientMenuBarState = DisplayState::VisibleNoMouse; break;
            default:                    m_clientMenuBarState = DisplayState::Hidden;         break;
            }
            // clang-format on
        }

        // Cycle through Standalone Editor Window States
        if (inputChannel.GetInputChannelId() == InputDeviceKeyboard::Key::NavigationEnd)
        {
            if (gEnv->IsEditor() && m_editorWindowState == DisplayState::Hidden)
            {
                ImGuiUpdateListenerBus::Broadcast(&IImGuiUpdateListener::OnOpenEditorWindow);
            }
            else
            {
                m_editorWindowState = m_editorWindowState == DisplayState::Visible
                                          ? DisplayState::VisibleNoMouse
                                          : DisplayState::Visible;
            }
        }
    }

    // Handle Keyboard Modifier Keys
    if (inputDeviceId == InputDeviceKeyboard::Id)
    {
        if (inputChannelId == InputDeviceKeyboard::Key::ModifierShiftL
            || inputChannelId == InputDeviceKeyboard::Key::ModifierShiftR)
        {
            io.KeyShift = inputChannel.IsActive();
        }
        else if (inputChannelId == InputDeviceKeyboard::Key::ModifierAltL
                 || inputChannelId == InputDeviceKeyboard::Key::ModifierAltR)
        {
            io.KeyAlt = inputChannel.IsActive();
        }
        else if (inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlL
                 || inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlR)
        {
            io.KeyCtrl = inputChannel.IsActive();
        }

        // Set Keydown Flag in ImGui Keys Array
        const int keyIndex = GetAzKeyIndex(inputChannelId);
        if (0 <= keyIndex  && keyIndex < AZ_ARRAY_SIZE(io.KeysDown))
        {
            io.KeysDown[keyIndex] = inputChannel.IsActive();
        }
    }

    // Handle Mouse Visibility
    io.MouseDrawCursor = m_clientMenuBarState == DisplayState::Visible
                         || m_editorWindowState == DisplayState::Visible;

    // IHardwareMouse has been deprecated in Lumberyard v1.11, so once the following line starts
    // throwing a compiler error you can just remove it and uncomment the two lines after it.
    // You'll likely also have to add this include: #include <LyShine/Bus/UiCursorBus.h>
    //bool isCursorVisible =
    //    gEnv->pSystem->GetIHardwareMouse() && !gEnv->pSystem->GetIHardwareMouse()->IsHidden();
    bool isCursorVisible = false;
    UiCursorBus::BroadcastResult(isCursorVisible, &UiCursorInterface::IsUiCursorVisible);
    if (!isCursorVisible && !io.MouseDrawCursor)
    {
        return false;
    }

    // Handle Mouse Inputs
    if (inputDeviceId == InputDeviceMouse::Id)
    {
        const int mouseButtonIndex = GetAzButtonIndex(inputChannelId);
        if (0 <= mouseButtonIndex && mouseButtonIndex < AZ_ARRAY_SIZE(io.MouseDown))
        {
            io.MouseDown[mouseButtonIndex] = inputChannel.IsActive();
        }
        else if (inputChannelId == InputDeviceMouse::Movement::Z)
        {
            io.MouseWheel = inputChannel.GetValue() / static_cast<float>(IMGUI_WHEEL_DELTA);
        }
    }

    if (m_clientMenuBarState == DisplayState::Visible
        || m_editorWindowState == DisplayState::Visible)
    {
        io.WantCaptureMouse = true;
        io.WantCaptureKeyboard = true;
        return true;
    }
    else
    {
        io.WantCaptureMouse = false;
        io.WantCaptureKeyboard = false;
        return false;
    }

    return false;
}

bool ImGuiManager::OnInputTextEventFiltered(const AZStd::string& textUTF8)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(textUTF8.c_str());
    return io.WantCaptureKeyboard && m_clientMenuBarState == DisplayState::Visible;
}

/**
    Resets all keys and buttons to the "off" state.
 */
void ImGuiManager::ResetInput()
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeyShift = false;
    io.KeyAlt = false;
    io.KeyCtrl = false;
    io.MouseWheel = 0.0f;

    for (uint32 i = 0; i < AZ_ARRAY_SIZE(io.KeysDown); ++i)
    {
        io.KeysDown[i] = false;
    }

    for (uint32 i = 0; i < AZ_ARRAY_SIZE(io.MouseDown); ++i)
    {
        io.MouseDown[i] = false;
    }
}

void ImGuiManager::RenderImGuiBuffers()
{
    // Trigger all listeners to run their updates
    EBUS_EVENT(ImGuiUpdateListenerBus, OnImGuiUpdate);

    // Run imgui's internal render and retrieve resulting draw data
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();

    //@rky: Only render the main ImGui if it is visible
    if (m_clientMenuBarState != DisplayState::Hidden)
    {
        IRenderer* renderer = gEnv->pRenderer;

        // Expand vertex buffer if necessary
        m_vertBuffer.reserve(drawData->TotalVtxCount);
        if (m_vertBuffer.size() < drawData->TotalVtxCount)
        {
            m_vertBuffer.insert(m_vertBuffer.end(),
                                drawData->TotalVtxCount - m_vertBuffer.size(),
                                SVF_P3F_C4B_T2F());
        }

        // Expand index buffer if necessary
        m_idxBuffer.reserve(drawData->TotalIdxCount);
        if (m_idxBuffer.size() < drawData->TotalIdxCount)
        {
            m_idxBuffer.insert(m_idxBuffer.end(), drawData->TotalIdxCount - m_idxBuffer.size(), 0);
        }

        // Process each draw command list individually
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = drawData->CmdLists[n];

            // Cache max vert count for easy access
            int numVerts = cmd_list->VtxBuffer.Size;

            // Copy command list verts into buffer
            for (int i = 0; i < numVerts; ++i)
            {
                const ImDrawVert& imguiVert = cmd_list->VtxBuffer[i];
                SVF_P3F_C4B_T2F& vert = m_vertBuffer[i];

                vert.xyz = Vec3(imguiVert.pos.x, imguiVert.pos.y, 0.0f);
                // Convert color from RGBA to ARGB
                vert.color.dcolor = (imguiVert.col & 0xFF00FF00)
                                    | ((imguiVert.col & 0xFF0000) >> 16)
                                    | ((imguiVert.col & 0xFF) << 16);
                vert.st = Vec2(imguiVert.uv.x, imguiVert.uv.y);
            }

            // Copy command list indices into buffer
            for (int i = 0; i < cmd_list->IdxBuffer.Size; ++i)
            {
                m_idxBuffer[i] = uint16(cmd_list->IdxBuffer[i]);
            }

            // Use offset pointer to step along rendering operation
            uint16* idxBufferDataOffset = m_idxBuffer.data();

            // Process each draw command individually
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

                // Defer to user rendering callback, if appropriate
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                // Otherwise render our buffers
                else
                {
                    int textureId = ((ITexture*)pcmd->TextureId)->GetTextureID();
                    renderer->SetTexture(textureId);
                    renderer->SetScissor((int)pcmd->ClipRect.x,
                                         (int)(pcmd->ClipRect.y),
                                         (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                                         (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                    renderer->DrawDynVB(m_vertBuffer.data(),
                                        idxBufferDataOffset,
                                        numVerts,
                                        pcmd->ElemCount,
                                        prtTriangleList);
                }

                // Update offset pointer into command list's index buffer
                idxBufferDataOffset += pcmd->ElemCount;
            }
        }

        // Reset scissor usage on renderer
        renderer->SetScissor();
    }
}
#endif // ~IMGUI_ENABLED
