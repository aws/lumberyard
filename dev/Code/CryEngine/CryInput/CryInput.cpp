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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"

// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "BaseInput.h"

#include "Synergy/SynergyContext.h"
#include "Synergy/SynergyKeyboard.h"
#include "Synergy/SynergyMouse.h"

#include <AzFramework/Input/Buses/Requests/InputSystemRequestBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

#if defined(WIN32)
#ifndef AZ_MONOLITHIC_BUILD
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}
#endif
#endif // WIN32


//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryInput
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryInput, "EngineModule_CryInput", 0x3cc0516071bb44f6, 0xae525949f30277f9)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryInput";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        IInput* pInput = 0;
        if (!gEnv->IsDedicated())
        {
#if defined(USE_AZ_TO_LY_INPUT)
            pInput = new AzToLyInput();
#elif defined(USE_DXINPUT)
            void* hWnd = initParams.hWndForInputSystem ? initParams.hWndForInputSystem : initParams.hWnd;
            pInput = new CDXInput(pSystem, (HWND) hWnd);
#elif defined(USE_LINUXINPUT)
            pInput = new CLinuxInput(pSystem);
#elif defined(USE_ANDROIDINPUT)
            pInput = new CAndroidInput(pSystem);
#elif defined(USE_IOSINPUT)
            pInput = new CIosInput(pSystem);
#elif defined(USE_APPLETVINPUT)
            pInput = new CAppleTVInput(pSystem);
#else
            pInput = new CBaseInput();
#endif
        }
        else
        {
            pInput = new CBaseInput();
        }

        if (!pInput->Init())
        {
            delete pInput;
            return false;
        }

#ifdef USE_SYNERGY_INPUT
        const char* pServer = g_pInputCVars->i_synergyServer->GetString();
        if (pServer && pServer[0] != '\0')
        {
    #if defined(AZ_FRAMEWORK_INPUT_ENABLED)
            // Create an instance of SynergyClient that will broadcast RawInputNotificationBusSynergy events.
            // Not ideal that this is static, but we can better control its lifecycle once it moves to a Gem.
            static AZStd::unique_ptr<SynergyInput::SynergyClient> pContext = AZStd::make_unique<SynergyInput::SynergyClient>(g_pInputCVars->i_synergyScreenName->GetString(), pServer);

            // Set custom create functions for our synergy mouse/keyboard implementations.
            AzFramework::InputDeviceKeyboard::Implementation::CustomCreateFunctionPointer = SynergyInput::InputDeviceKeyboardSynergy::Create;
            AzFramework::InputDeviceMouse::Implementation::CustomCreateFunctionPointer = SynergyInput::InputDeviceMouseSynergy::Create;

            // Recreate all enabled input devices so the synergy implementations are created instead of the default ones.
            AzFramework::InputSystemRequestBus::Broadcast(&AzFramework::InputSystemRequests::RecreateEnabledInputDevices);
    #else
            _smart_ptr<CSynergyContext> pContext = new CSynergyContext(g_pInputCVars->i_synergyScreenName->GetString(), pServer);
            pInput->AddInputDevice(new CSynergyKeyboard(*pInput, pContext));
            pInput->AddInputDevice(new CSynergyMouse(*pInput, pContext));
    #endif // defined(AZ_FRAMEWORK_INPUT_ENABLED)
        }
#endif

        env.pInput = pInput;

        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryInput)

CEngineModule_CryInput::CEngineModule_CryInput()
{
};

CEngineModule_CryInput::~CEngineModule_CryInput()
{
};

#include <CrtDebugStats.h>

