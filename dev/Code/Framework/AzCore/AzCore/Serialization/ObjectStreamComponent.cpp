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

#include <AzCore/Serialization/ObjectStreamComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    //=========================================================================
    // ObjectStreamComponent
    // [6/25/2012]
    //=========================================================================
    ObjectStreamComponent::ObjectStreamComponent()
    {
    }

    //=========================================================================
    // Activate
    // [6/25/2012]
    //=========================================================================
    void ObjectStreamComponent::Activate()
    {
    }

    //=========================================================================
    // Deactivate
    // [6/25/2012]
    //=========================================================================
    void ObjectStreamComponent::Deactivate()
    {
    }

    //=========================================================================
    // GetProvidedServices
    //=========================================================================
    void ObjectStreamComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ObjectStreamService", 0x7fa046aa));
    }

    //=========================================================================
    // GetIncompatibleServices
    //=========================================================================
    void ObjectStreamComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ObjectStreamService", 0x7fa046aa));
    }

    //=========================================================================
    // GetDependentServices
    //=========================================================================
    void ObjectStreamComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("MemoryService", 0x5c4d473c));
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void ObjectStreamComponent::Reflect(ReflectContext* reflection)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection))
        {
            serializeContext->Class<ObjectStreamComponent, AZ::Component>()
                ->Version(2)
                ;

            if (EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Class<ObjectStreamComponent>(
                    "Object Stream", "Provides async load/save functionality of structured object data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }
} // namespace AZ
