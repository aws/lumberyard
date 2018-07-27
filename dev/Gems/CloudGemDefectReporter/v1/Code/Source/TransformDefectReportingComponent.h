
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
#include <AzCore/Component/TransformBus.h>
#include "CloudGemDefectReporter/CloudGemDefectReporterBus.h"

namespace CloudGemDefectReporter
{

    class TransformDefectReportingComponent
        : public AZ::Component
        , protected CloudGemDefectReporterNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(TransformDefectReportingComponent, "{2C5DC92A-F764-4B92-BB2C-697ABB19797D}");

        // AZ::Component interface implementation.
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        // Optional functions for defining provided and dependent services.
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReporterNotificationBus interface implementation
        virtual void OnCollectDefectReporterData(int reportID) override;

        ////////////////////////////////////////////////////////////////////////
        // component data
        AZStd::string m_identifier = {};
    };

}