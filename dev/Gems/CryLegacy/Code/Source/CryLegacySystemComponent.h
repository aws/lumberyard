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
#include <CryLegacy/CryLegacyBus.h>

#include <IGameFramework.h>
#include <IWindowMessageHandler.h>
#include <IPlatformOS.h>

#if defined(APPLE)
#define GAME_FRAMEWORK_FILENAME  "libCryAction.dylib"
#elif defined(LINUX)
#define GAME_FRAMEWORK_FILENAME  "libCryAction.so"
#else
#define GAME_FRAMEWORK_FILENAME  "CryAction.dll"
#endif


namespace CryLegacy
{
    class CryLegacySystemComponent
        : public AZ::Component
        , protected CryLegacyRequestBus::Handler
        , protected CryGameFrameworkBus::Handler
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
        // CryLegacyRequestBus interface implementation
        IGameFramework* InitFramework(SSystemInitParams& startupParams) override;

        void ShutdownFramework() override;

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        IGameFramework*     m_Framework = nullptr;
#if !defined(AZ_MONOLITHIC_BUILD)
        HMODULE             m_FrameworkDll = nullptr;
#endif // !defined(AZ_MONOLITHIC_BUILD)
    };
}
