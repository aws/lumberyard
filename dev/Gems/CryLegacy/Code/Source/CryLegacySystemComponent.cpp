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
#include "CryLegacy_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <ISystem.h>
#include <IRenderer.h>

#include "CryLegacySystemComponent.h"
#include <IPlayerProfiles.h>
#include <CryLibrary.h>
#include <IPlatformOS.h>

#include "Input/AzToLyInput.h"
#include "CryAction/CryAction.h"
#include "CryAISystem/CAISystem.h"
#include "CryAnimation/AnimationBase.h"
#include "CryScriptSystem/ScriptSystem.h"

CAISystem* g_pAISystem;

namespace CryLegacy
{
    void CryLegacySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CryLegacySystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CryLegacySystemComponent>("CryLegacy", "Component to provide Cry Legacy handling of initializing the IGameFramework system interface")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CryLegacySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CryLegacyService"));
    }

    void CryLegacySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CryLegacyService"));
    }

    void CryLegacySystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CryLegacySystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CryLegacySystemComponent::Init()
    {
    }

    void CryLegacySystemComponent::Activate()
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        // Legacy games containing code to explicitly load CryAction and call the CreateGameFramework function
        // were failing because the EBus call to CryGameFrameworkRequests::CreateFramework was returning nullptr
        // due to the global environment not yet having been attached to the CryAction module (when running a non
        // monolithic build). Explicitly loading the module here ensures that the global environment gets attached.
        m_cryActionHandle = AZ::DynamicModuleHandle::Create("CryAction");
        if (m_cryActionHandle)
        {
            m_cryActionHandle->Load(true);
        }
#endif // !defined(AZ_MONOLITHIC_BUILD)

        CryLegacy::CryLegacyRequestBus::Handler::BusConnect();
        CryGameFrameworkBus::Handler::BusConnect();
        CryLegacyInputRequestBus::Handler::BusConnect();
        CryLegacyAISystemRequestBus::Handler::BusConnect();
        CryLegacyAnimationRequestBus::Handler::BusConnect();
        CryLegacyEntitySystemRequestBus::Handler::BusConnect();
        CryLegacyScriptSystemRequestBus::Handler::BusConnect();
    }

    void CryLegacySystemComponent::Deactivate()
    {
        CryLegacyEntitySystemRequestBus::Handler::BusDisconnect();
        CryLegacyScriptSystemRequestBus::Handler::BusDisconnect();
        CryLegacyAnimationRequestBus::Handler::BusDisconnect();
        CryLegacyAISystemRequestBus::Handler::BusDisconnect();
        CryLegacyInputRequestBus::Handler::BusDisconnect();
        CryGameFrameworkBus::Handler::BusDisconnect();
        CryLegacy::CryLegacyRequestBus::Handler::BusDisconnect();

#if !defined(AZ_MONOLITHIC_BUILD)
        if (m_cryActionHandle)
        {
            m_cryActionHandle->Unload();
            m_cryActionHandle.reset(nullptr);
        }
#endif // !defined(AZ_MONOLITHIC_BUILD)
    }

    IGameFramework* CryLegacySystemComponent::CreateFramework()
    {
        return new CCryAction();
    }

    IGameFramework* CryLegacySystemComponent::InitFramework(SSystemInitParams& startupParams)
    {
        m_Framework = CreateFramework();
        if (!m_Framework)
        {
            return nullptr;
        }

        // Initialize the engine.
        if (!m_Framework->Init(startupParams))
        {
            return nullptr;
        }

        ISystem* system = m_Framework->GetISystem();
        ModuleInitISystem(system, "CryGame");

#ifdef WIN32
        if (gEnv->pRenderer)
        {
            SetWindowLongPtr(reinterpret_cast<HWND>(gEnv->pRenderer->GetHWND()), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        }
#endif

        return m_Framework;
    }

    void CryLegacySystemComponent::ShutdownFramework()
    {
        if (m_Framework)
        {
            m_Framework->Shutdown();
            m_Framework = nullptr;
        }
    }

    IInput* CryLegacySystemComponent::InitInput()
    {
        IInput* pInput = new AzToLyInput();
        if (!pInput->Init())
        {
            delete pInput;
            pInput = nullptr;
        }
        return pInput;
    }

    void CryLegacySystemComponent::ShutdownInput(IInput* input)
    {
        if (input)
        {
            input->ShutDown();
            delete input;
        }
    }

    IAISystem* CryLegacySystemComponent::InitAISystem()
    {
        AIInitLog(gEnv->pSystem);
        g_pAISystem = new CAISystem(gEnv->pSystem);

        // Unlike the other legacy CrySystems, we call Init later in SystemInit.cpp
        return g_pAISystem;
    }

    void CryLegacySystemComponent::ShutdownAISystem(IAISystem* aiSystem)
    {
        if (aiSystem && aiSystem == g_pAISystem)
        {
            aiSystem->Release();
            g_pAISystem = nullptr;
        }
    }

    bool CryLegacySystemComponent::InitCharacterManager(const SSystemInitParams& initParams)
    {
        return LegacyCryAnimation::InitCharacterManager(initParams);
    }

    void CryLegacySystemComponent::ShutdownCharacterManager(ICharacterManager* characterManager)
    {
        if (characterManager)
        {
            SAFE_RELEASE(characterManager);
        }
    }

    IEntitySystem* CryLegacySystemComponent::InitEntitySystem()
    {
        return LegacyCryEntitySystem::InitEntitySystem();
    }

    void CryLegacySystemComponent::ShutdownEntitySystem(IEntitySystem* entitySystem)
    {
        if (entitySystem)
        {
            SAFE_RELEASE(entitySystem);
        }
    }

    IScriptSystem* CryLegacySystemComponent::InitScriptSystem()
    {
        CScriptSystem* pScriptSystem = new CScriptSystem;

        bool bStdLibs = true;
        if (!pScriptSystem->Init(gEnv->pSystem, bStdLibs, 1024))
        {
            pScriptSystem->Release();
            return nullptr;
        }

        gEnv->pScriptSystem = pScriptSystem;
        return pScriptSystem;
    }

    void CryLegacySystemComponent::ShutdownScriptSystem(IScriptSystem* scriptSystem)
    {
        if (scriptSystem)
        {
            SAFE_RELEASE(scriptSystem);
        }
    }
}
