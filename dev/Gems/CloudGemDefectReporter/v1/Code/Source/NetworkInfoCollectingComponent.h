/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
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
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>

#include <CrySystemBus.h>

namespace CloudGemDefectReporter
{
    class NetworkInfoCollectingComponent
        : public AZ::Component
        , protected CloudGemDefectReporterNotificationBus::Handler
        , protected CrySystemEventBus::Handler
    {
    public:
        NetworkInfoCollectingComponent();
        AZ_COMPONENT(NetworkInfoCollectingComponent, "{78d8b29c-9457-44fc-a5c8-20b5006b4843}");

        static void Reflect(AZ::ReflectContext* context);

    protected:        
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReporterNotificationBus::Handler interface implementation
        virtual void OnCollectDefectReporterData(int reportID) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus interface implementation
        virtual void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params) override;
        ////////////////////////////////////////////////////////////////////////
    private:
        bool ReadServerDomainNames();
        const char* GetServerDomainNamesConfigPath() const;

        AZStd::vector<AZStd::string> m_serverDomainNames;
    };
}
