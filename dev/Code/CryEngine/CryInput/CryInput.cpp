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

#if !defined(_RELEASE) && !defined(WIN32) && !defined(DEDICATED_SERVER)
#   define SYNERGY_INPUT_ENABLED
#endif // !defined(_RELEASE) && !defined(WIN32) && !defined(DEDICATED_SERVER)

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
#if !defined(DEDICATED_SERVER)
        if (!gEnv->IsDedicated())
        {
            pInput = new AzToLyInput();
        }
        else
#endif // !defined(DEDICATED_SERVER)
        {
            pInput = new CBaseInput();
        }

        if (!pInput->Init())
        {
            delete pInput;
            return false;
        }

#if defined(SYNERGY_INPUT_ENABLED)
        const char* pServer = g_pInputCVars->i_synergyServer->GetString();
        if (pServer && pServer[0] != '\0')
        {
            // Create an instance of SynergyClient that will broadcast RawInputNotificationBusSynergy events.
            m_synergyContext = AZStd::make_unique<SynergyInput::SynergyClient>(g_pInputCVars->i_synergyScreenName->GetString(), pServer);

            // Create the custom synergy keyboard implementation.
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::Bus::Broadcast(
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceKeyboard>::CreateCustomImplementation,
                SynergyInput::InputDeviceKeyboardSynergy::Create);

            // Create the custom synergy mouse implementation.
            AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::Bus::Broadcast(
                &AzFramework::InputDeviceImplementationRequest<AzFramework::InputDeviceMouse>::CreateCustomImplementation,
                SynergyInput::InputDeviceMouseSynergy::Create);
        }
#endif // defined(SYNERGY_INPUT_ENABLED)

        env.pInput = pInput;

        return true;
    }

private:
#if defined(SYNERGY_INPUT_ENABLED)
    AZStd::unique_ptr<SynergyInput::SynergyClient> m_synergyContext;
#endif // defined(SYNERGY_INPUT_ENABLED)
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryInput)

CEngineModule_CryInput::CEngineModule_CryInput()
{
};

CEngineModule_CryInput::~CEngineModule_CryInput()
{
};

#include <CrtDebugStats.h>

