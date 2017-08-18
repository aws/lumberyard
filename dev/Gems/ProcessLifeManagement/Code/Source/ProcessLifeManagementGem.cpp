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
#include <platform_impl.h>
#include <IGameFramework.h>
#include "ProcessLifeManagementGem.h"
#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiCanvasBus.h>

using namespace AzFramework;
using namespace ProcessLifeManagement;

////////////////////////////////////////////////////////////////////////////////////////////////////
ProcessLifeManagementGem::ProcessLifeManagementGem()
    : CryHooksModule()
    , m_pausedCanvasId() // Defaults to AZ::EntityId::InvalidEntityId
{
    ApplicationLifecycleEvents::Bus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ProcessLifeManagementGem::~ProcessLifeManagementGem()
{
    ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessLifeManagementGem::OnApplicationConstrained(ApplicationLifecycleEvents::Event /*lastEvent*/)
{
    if (m_pausedCanvasId.IsValid())
    {
        // Already paused.
        return;
    }

    // Load/show a modal pause screen.
    if (gEnv && gEnv->pLyShine)
    {
        m_pausedCanvasId = gEnv->pLyShine->LoadCanvas("@assets@/ui/canvases/paused.uicanvas");
    }

    if (!m_pausedCanvasId.IsValid())
    {
        // Unable to load canvas.
        return;
    }

    // Update text to reflect input required to unpause (should be localized).
#if defined (AZ_PLATFORM_ANDROID) || defined (AZ_PLATFORM_APPLE_IOS)
    const char* instructionsText = "Touch the screen to resume";
#else
    const char* instructionsText = "Press any key or button to resume";
#endif

    AZ::Entity* instructionsTextElement = nullptr;
    EBUS_EVENT_ID_RESULT(instructionsTextElement, m_pausedCanvasId, UiCanvasBus, FindElementByName, "InstructionsText");
    if (instructionsTextElement != nullptr && instructionsTextElement->GetId().IsValid())
    {
        EBUS_EVENT_ID(instructionsTextElement->GetId(), UiTextBus, SetText, instructionsText);
    }

    // Pause the game.
    gEnv->pGame->GetIGameFramework()->PauseGame(true, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessLifeManagementGem::OnApplicationUnconstrained(ApplicationLifecycleEvents::Event /*lastEvent*/)
{
    if (m_pausedCanvasId.IsValid())
    {
        // Exclusively capture all input.
        gEnv->pInput->SetExclusiveListener(this);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool ProcessLifeManagementGem::OnInputEvent(const SInputEvent& inputEvent)
{
    bool shouldUnpause = false;
    switch (inputEvent.deviceType)
    {
    case eIDT_Keyboard:
    case eIDT_TouchScreen:
    {
        shouldUnpause = true;
    }
    break;
    case eIDT_Mouse:
    {
        if (inputEvent.keyId >= eKI_Mouse1 &&
            inputEvent.keyId <= eKI_Mouse8)
        {
            shouldUnpause = true;
        }
    }
    break;
    case eIDT_Gamepad:
    {
        if ((inputEvent.keyId >= eKI_XI_DPadUp && inputEvent.keyId <= eKI_XI_TriggerR) ||
            (inputEvent.keyId >= eKI_XI_TriggerLBtn && inputEvent.keyId <= eKI_XI_TriggerRBtn) ||
            (inputEvent.keyId >= eKI_Orbis_Options && inputEvent.keyId <= eKI_Orbis_Square))
        {
            shouldUnpause = true;
        }
    }
    break;
    }

    if (shouldUnpause)
    {
        // Unpause the game.
        gEnv->pGame->GetIGameFramework()->PauseGame(false, false);

        // Stop exclusively capturing input.
        gEnv->pInput->SetExclusiveListener(nullptr);

        // Unload/hide the modal pause screen.
        gEnv->pLyShine->ReleaseCanvas(m_pausedCanvasId, false);
        m_pausedCanvasId.SetInvalid();

        // Return false so that this input is forwarded through to other listeners now that we're unpaused.
        return false;
    }

    // Return true to prevent any other systems receiving this input event.
    return true;
}

AZ_DECLARE_MODULE_CLASS(ProcessLifeManagement_698948526ada4d13bada933e2d7ee463, ProcessLifeManagement::ProcessLifeManagementGem)
