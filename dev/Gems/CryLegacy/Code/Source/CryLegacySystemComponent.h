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
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <CryLegacy/CryLegacyBus.h>
#include <AzCore/Memory/AllocatorScope.h>

#include <IGameFramework.h>
#include <IWindowMessageHandler.h>
#include <IInput.h>
#include <IAISystem.h>
#include <ICryAnimation.h>
#include <IEntitySystem.h>
#include <IScriptSystem.h>

namespace CryLegacy
{
    // CryLegacy gem will always require AZ::LegacyAllocator and CryStringAllocator
    // This gets added to the CryLegacySystemComponent to ensure that these allocators
    // are available for the duration that cry legacy code is running
    using CryLegacyAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator>;

    class CryLegacySystemComponent
        : public AZ::Component
        , protected CryLegacyRequestBus::Handler
        , protected CryGameFrameworkBus::Handler
        , protected CryLegacyInputRequestBus::Handler
        , protected CryLegacyAISystemRequestBus::Handler
        , protected CryLegacyAnimationRequestBus::Handler
        , protected CryLegacyEntitySystemRequestBus::Handler
        , protected CryLegacyScriptSystemRequestBus::Handler
        , protected CryLegacyAllocatorScope
    {
    public:
        AZ_COMPONENT(CryLegacySystemComponent, "{D2051F81-6B46-4B23-A7F6-C19F814E63F0}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CryGameFrameworkBus interface implementation
        IGameFramework* CreateFramework() override;
        IGameFramework* InitFramework(SSystemInitParams& startupParams) override;
        void ShutdownFramework() override;

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryLegacyInputRequests interface implementation
        IInput* InitInput() override;
        void ShutdownInput(IInput* input) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryLegacyAISystemRequests interface implementation
        IAISystem* InitAISystem() override;
        void ShutdownAISystem(IAISystem* aiSystem) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryLegacyAnimationRequests interface implementation
        bool InitCharacterManager(const SSystemInitParams& initParams) override;
        void ShutdownCharacterManager() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryLegacyEntitySystemRequests interface implementation
        IEntitySystem* InitEntitySystem() override;
        void ShutdownEntitySystem(IEntitySystem* entitySystem) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryLegacyScriptSystemRequests interface implementation
        IScriptSystem* InitScriptSystem() override;
        void ShutdownScriptSystem(IScriptSystem* scriptSystem) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        IGameFramework* m_Framework = nullptr;
#if !defined(AZ_MONOLITHIC_BUILD)
        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_cryActionHandle;
#endif // !defined(AZ_MONOLITHIC_BUILD)
    };
}
