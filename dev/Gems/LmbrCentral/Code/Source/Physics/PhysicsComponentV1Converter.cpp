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
#include "RigidPhysicsComponent.h"
#include "StaticPhysicsComponent.h"

#if defined(LMBR_CENTRAL_EDITOR)
#   include "EditorRigidPhysicsComponent.h"
#   include "EditorStaticPhysicsComponent.h"
#endif

#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    void ReadRigidPhysicsConfig(AzFramework::RigidPhysicsConfig& configuration, AZ::SerializeContext::DataElementNode* configurationNode, bool isProximityTriggerable)
    {
        if (configurationNode)
        {
            configurationNode->GetChildData(AZ::Crc32("EnabledInitially"), configuration.m_enabledInitially);
            configurationNode->GetChildData(AZ::Crc32("SpecifyMassOrDensity"), configuration.m_specifyMassOrDensity);
            configurationNode->GetChildData(AZ::Crc32("Mass"), configuration.m_mass);
            configurationNode->GetChildData(AZ::Crc32("Density"), configuration.m_density);
            configurationNode->GetChildData(AZ::Crc32("AtRestInitially"), configuration.m_atRestInitially);
            configurationNode->GetChildData(AZ::Crc32("RecordCollisions"), configuration.m_recordCollisions);
            configurationNode->GetChildData(AZ::Crc32("MaxRecordedCollisions"), configuration.m_maxRecordedCollisions);
            configurationNode->GetChildData(AZ::Crc32("SimulationDamping"), configuration.m_simulationDamping);
            configurationNode->GetChildData(AZ::Crc32("SimulationMinEnergy"), configuration.m_simulationMinEnergy);
            configurationNode->GetChildData(AZ::Crc32("BuoyancyDamping"), configuration.m_buoyancyDamping);
            configurationNode->GetChildData(AZ::Crc32("BuoyancyDensity"), configuration.m_buoyancyDensity);
            configurationNode->GetChildData(AZ::Crc32("BuoyancyResistance"), configuration.m_buoyancyResistance);
        }

        // "proximity triggerable" used to live elsewhere
        configuration.m_interactsWithTriggers = isProximityTriggerable;
    }

    void ReadStaticPhysicsConfiguration(AzFramework::StaticPhysicsConfig& configuration, AZ::SerializeContext::DataElementNode* configurationNode)
    {
        if (configurationNode)
        {
            configurationNode->GetChildData(AZ::Crc32("EnabledInitially"), configuration.m_enabledInitially);
        }
    }


    /**
     * Given an old PhysicsComponent node, write out the appropriate component type.
     *
     * In the past there was a single PhysicsComponent and users specified
     * the type of physics by adding a "behavior" class to the PhysicsComponent.
     * Now we have separate components for RigidPhysicsComponent and StaticPhysicsComponent.
     * 
     * Also, in the past we only had a runtime component but now we have
     * separate editor and runtime components
     */
    extern bool PhysicsComponentV1Converter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classNode)
    {
        // To determine whether we want an editor or runtime component, we check
        // if the old component was contained within GenericComponentWrapper::m_template
        bool isEditorComponent = (classNode.GetName() == AZ::Crc32("m_template"));

        // Get Component::m_id out of base class
        auto baseClassNode = classNode.FindSubElement(AZ::Crc32("BaseClass1"));
        if (!baseClassNode)
        {
            return false;
        }

        AZ::u64 componentId = 0;
        if (!baseClassNode->GetChildData(AZ::Crc32("Id"), componentId))
        {
            return false;
        }

        // The old physics component kept its settings in a "Configuration" struct.
        auto physicsConfigurationNode = classNode.FindSubElement(AZ::Crc32("Configuration"));
        if (!physicsConfigurationNode)
        {
            return false;
        }

        // The physics configuration stored whether or not to interact with triggers.
        bool isProximityTriggerable = true;
        physicsConfigurationNode->GetChildData(AZ::Crc32("Proximity Triggerable"), isProximityTriggerable);

        // The physics configuration stored the "Behavior" class Within a shared_ptr.
        auto behaviorPtrNode = physicsConfigurationNode->FindSubElement(AZ::Crc32("Behavior"));
        if (!behaviorPtrNode)
        {
            return false;
        }

        // Actual behavior class is first element in shared_ptr.
        // This behavior class had specific subclasses to handle rigid or static physics.
        // It's also possible that the user never set a behavior.
        AZ::SerializeContext::DataElementNode* behaviorNode = nullptr;
        if (behaviorPtrNode->GetNumSubElements() > 0)
        {
            behaviorNode = &behaviorPtrNode->GetSubElement(0);
        }

        //
        // Convert the classNode into the appropriate component type.
        // Upon conversion all child node are deleted.
        // We re-populate the child nodes by creating an instance of the
        // appropriate component type and calling: classNode->SetData(actualComponentInstance)
        //

        if (behaviorNode && (behaviorNode->GetId() == AZ::Uuid("{48366EA0-71FA-49CA-B566-426EE897468E}"))) // RigidPhysicsBehavior
        {
            // The rigid behavior class contained a AzFramework::RigidPhysicsConfig.
            auto rigidConfigurationNode = behaviorNode->FindSubElement(AZ::Crc32("Configuration"));

#if defined(LMBR_CENTRAL_EDITOR)
            if (isEditorComponent)
            {
                EditorRigidPhysicsConfig configuration;
                ReadRigidPhysicsConfig(configuration, rigidConfigurationNode, isProximityTriggerable);

                EditorRigidPhysicsComponent component;
                component.SetConfiguration(configuration);

                classNode.Convert(serializeContext, azrtti_typeid(component));
                classNode.SetData(serializeContext, component);
            }
            else
#endif
            {
                AzFramework::RigidPhysicsConfig configuration;
                ReadRigidPhysicsConfig(configuration, rigidConfigurationNode, isProximityTriggerable);

                RigidPhysicsComponent component;
                component.SetConfiguration(configuration);

                classNode.Convert(serializeContext, azrtti_typeid(component));
                classNode.SetData(serializeContext, component);
            }
        }
        else // Otherwise assume static physics, the user might have never set a behavior.
        {
            // The static behavior class contained a AzFramework::StaticPhysicsConfig.
            AZ::SerializeContext::DataElementNode* staticConfigurationNode = nullptr;
            if (behaviorNode && (behaviorNode->GetId() == AZ::Uuid("{BC0600CC-5EF5-4753-A8BE-E28194149CA5}"))) // StaticPhysicsBehavior
            {
                staticConfigurationNode = behaviorNode->FindSubElement(AZ::Crc32("Configuration"));
            }

#if defined(LMBR_CENTRAL_EDITOR)
            if (isEditorComponent)
            {
                EditorStaticPhysicsConfig configuration;
                ReadStaticPhysicsConfiguration(configuration, staticConfigurationNode);

                EditorStaticPhysicsComponent component;
                component.SetConfiguration(configuration);

                classNode.Convert(serializeContext, azrtti_typeid(component));
                classNode.SetData(serializeContext, component);
            }
            else
#endif 
            {
                AzFramework::StaticPhysicsConfig configuration;
                ReadStaticPhysicsConfiguration(configuration, staticConfigurationNode);

                StaticPhysicsComponent component;
                component.SetConfiguration(configuration);

                classNode.Convert(serializeContext, azrtti_typeid(component));
                classNode.SetData(serializeContext, component);
            }
        }

        // We want to set the component ID to match the old one.
        // Dig down through base classes until we reach AZ::Component.
        baseClassNode = classNode.FindSubElement(AZ::Crc32("BaseClass1"));
        while (baseClassNode)
        {
            if (baseClassNode->GetId() == azrtti_typeid<AZ::Component>())
            {
                if (auto idNode = baseClassNode->FindSubElement(AZ::Crc32("Id")))
                {
                    idNode->SetData(serializeContext, componentId);
                }
                break;
            }
            baseClassNode = baseClassNode->FindSubElement(AZ::Crc32("BaseClass1"));
        }

        return true;
    }
} // namespace LmbrCentral