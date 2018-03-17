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

#include "LmbrCentral_precompiled.h"

#include "ShapeComponentConverters.h"

#include "EditorCapsuleShapeComponent.h"
#include "EditorCylinderShapeComponent.h"
#include "EditorPolygonPrismShapeComponent.h"
#include "EditorSphereShapeComponent.h"
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        bool DeprecateEditorSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Cache the Configuration
            LmbrCentral::SphereShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<LmbrCentral::SphereShapeConfig>(configuration);
            }

            // Convert to EditorSphereShapeComponent
            bool result = classElement.Convert<LmbrCentral::EditorSphereShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<LmbrCentral::SphereShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<LmbrCentral::SphereShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

        bool UpgradeEditorSphereShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                LmbrCentral::SphereShapeConfig configuration;
                int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }

                // Convert to EditorSphereShapeComponent
                bool result = classElement.Convert<LmbrCentral::EditorSphereShapeComponent>(context);
                if (result)
                {
                    configIndex = classElement.AddElement<LmbrCentral::SphereShapeConfig>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<LmbrCentral::SphereShapeConfig>(context, configuration);
                    }
                    return true;
                }
                return false;
            }
            return true;
        }

        bool DeprecateEditorBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Cache the Configuration
            LmbrCentral::BoxShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<LmbrCentral::BoxShapeConfig>(configuration);
            }

            // Convert to EditorBoxShapeComponent
            if (classElement.Convert(context, LmbrCentral::EditorBoxShapeComponentTypeId))
            {
                configIndex = classElement.AddElement<LmbrCentral::BoxShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<LmbrCentral::BoxShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

        bool UpgradeEditorBoxShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                // Cache the Configuration
                LmbrCentral::BoxShapeConfig configuration;
                int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }

                // Convert to EditorBoxShapeComponent
                if (classElement.Convert(context, LmbrCentral::EditorBoxShapeComponentTypeId))
                {
                    configIndex = classElement.AddElement<LmbrCentral::BoxShapeConfig>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<LmbrCentral::BoxShapeConfig>(context, configuration);
                    }
                    return true;
                }
                return false;
            }
            return true;
        }

        bool DeprecateEditorCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Cache the Configuration
            LmbrCentral::CylinderShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<LmbrCentral::CylinderShapeConfig>(configuration);
            }

            // Convert to EditorCylinderShapeComponent
            bool result = classElement.Convert<LmbrCentral::EditorCylinderShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<LmbrCentral::CylinderShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<LmbrCentral::CylinderShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

        bool UpgradeEditorCylinderShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                // Cache the Configuration
                LmbrCentral::CylinderShapeConfig configuration;
                int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }

                // Convert to EditorCylinderShapeComponent
                bool result = classElement.Convert<LmbrCentral::EditorCylinderShapeComponent>(context);
                if (result)
                {
                    configIndex = classElement.AddElement<LmbrCentral::CylinderShapeConfig>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<LmbrCentral::CylinderShapeConfig>(context, configuration);
                    }
                    return true;
                }
                return false;
            }
            return true;
        }

        bool DeprecateEditorCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Cache the Configuration
            LmbrCentral::CapsuleShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<LmbrCentral::CapsuleShapeConfig>(configuration);
            }

            // Convert to EditorCapsuleShapeComponent
            bool result = classElement.Convert<LmbrCentral::EditorCapsuleShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<LmbrCentral::CapsuleShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<LmbrCentral::CapsuleShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

        bool UpgradeEditorCapsuleShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                // Cache the Configuration
                LmbrCentral::CapsuleShapeConfig configuration;
                int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }

                // Convert to EditorCapsuleShapeComponent
                bool result = classElement.Convert<LmbrCentral::EditorCapsuleShapeComponent>(context);
                if (result)
                {
                    configIndex = classElement.AddElement<LmbrCentral::CapsuleShapeConfig>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<LmbrCentral::CapsuleShapeConfig>(context, configuration);
                    }
                    return true;
                }
                return false;
            }
            return true;
        }

        bool UpgradeEditorPolygonPrismShapeComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                // Cache the Configuration
                LmbrCentral::PolygonPrismCommon configuration;
                int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).GetData(configuration);
                }

                // Convert to EditorPolygonPrismComponent
                bool result = classElement.Convert<LmbrCentral::EditorPolygonPrismShapeComponent>(context);
                if (result)
                {
                    configIndex = classElement.AddElement<LmbrCentral::PolygonPrismCommon>(context, "Configuration");
                    if (configIndex != -1)
                    {
                        classElement.GetSubElement(configIndex).SetData<LmbrCentral::PolygonPrismCommon>(context, configuration);
                    }
                    return true;
                }
                return false;
            }
            return true;
        }
    }
}