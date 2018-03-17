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

#define DLL_INITFUNC_CREATEGAME "CreateGameFramework"

#if defined(AZ_MONOLITHIC_BUILD)
extern "C"
{
    IGameFramework* CreateGameFramework();
}
#endif

namespace CryLegacy
{
    void CryLegacySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CryLegacySystemComponent, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();

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
        if (!m_FrameworkDll)
        {
            m_FrameworkDll = CryLoadLibrary(GAME_FRAMEWORK_FILENAME);
            if (!m_FrameworkDll)
            {
                return;
            }
        }
#endif // !defined(AZ_MONOLITHIC_BUILD)
        CryLegacyInputRequestBus::Handler::BusConnect();
        CryLegacy::CryLegacyRequestBus::Handler::BusConnect();
        CryGameFrameworkBus::Handler::BusConnect();
    }

    void CryLegacySystemComponent::Deactivate()
    {
        CryGameFrameworkBus::Handler::BusDisconnect();
        CryLegacy::CryLegacyRequestBus::Handler::BusDisconnect();
        CryLegacyInputRequestBus::Handler::BusDisconnect();
    }

    IGameFramework* CryLegacySystemComponent::InitFramework(SSystemInitParams& startupParams)
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        IGameFramework::TEntryFunction CreateGameFramework = reinterpret_cast<IGameFramework::TEntryFunction>(CryGetProcAddress(m_FrameworkDll, DLL_INITFUNC_CREATEGAME));
        if (!CreateGameFramework)
        {
            return nullptr;
        }
#endif // !AZ_MONOLITHIC_BUILD
        m_Framework = CreateGameFramework();
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
}
