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
#include "CylinderShape.h"

namespace LmbrCentral
{
    class CylinderShapeComponent
        : public AZ::Component
        , public CylinderShape
    {
    public:
        AZ_COMPONENT(CylinderShapeComponent, CylinderShapeComponentTypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        //////////////////////////////////////////////////////////////////////////    

        // Get the configuration
        CylinderShapeConfig& GetConfiguration() override { return m_configuration; }

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("CylinderShapeService", 0x507c688e));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            incompatible.push_back(AZ_CRC("CylinderShapeService", 0x507c688e));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        //////////////////////////////////////////////////////////////////////////
        // Serialized data

        //! Stores configuration of a cylinder for this component
        CylinderShapeConfig m_configuration;
    };
} // namespace LmbrCentral
