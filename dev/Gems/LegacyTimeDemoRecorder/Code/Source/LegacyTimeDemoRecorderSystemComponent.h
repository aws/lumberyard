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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <CrySystemBus.h>
#include <LegacyTimeDemoRecorder/LegacyTimeDemoRecorderBus.h>
#include "TimeDemoRecorder.h"

namespace LegacyTimeDemoRecorder
{
    class LegacyTimeDemoRecorderSystemComponent
        : public AZ::Component
        , protected LegacyTimeDemoRecorderRequestBus::Handler
        , protected CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(LegacyTimeDemoRecorderSystemComponent, "{2B5F2085-EBCF-479D-8317-9F88D95C1539}");

        virtual ~LegacyTimeDemoRecorderSystemComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LegacyTimeDemoRecorderRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus interface implementation
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
        CTimeDemoRecorder* m_legacyTimeDemoRecorder = nullptr;
        bool m_canActivateTimeRecorder = false;
    };
}
