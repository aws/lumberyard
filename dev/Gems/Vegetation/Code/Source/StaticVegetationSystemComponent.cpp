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

#include "Vegetation_precompiled.h"
#include <StaticVegetationSystemComponent.h>
#include <MathConversion.h>
#include <IEntityRenderState.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Vegetation
{

    void StaticVegetationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    
    }

    void StaticVegetationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {

    }

    void StaticVegetationSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {

    }

    void StaticVegetationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<StaticVegetationSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<StaticVegetationSystemComponent>("Static Vegetation System", "Manages interactions between the legacy and dynamic vegetation systems and instances")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void StaticVegetationSystemComponent::Init()
    {

    }

    void StaticVegetationSystemComponent::Activate()
    {
        StaticVegetationNotificationBus::Handler::BusConnect();
        StaticVegetationRequestBus::Handler::BusConnect();
    }

    void StaticVegetationSystemComponent::Deactivate()
    {
        StaticVegetationNotificationBus::Handler::BusDisconnect();
        StaticVegetationRequestBus::Handler::BusDisconnect();
    }

    void StaticVegetationSystemComponent::InstanceAdded(UniqueVegetationInstancePtr vegetationInstance, AZ::Aabb aabb)
    {
        AZStd::unique_lock<decltype(m_mapMutex)> mapScopeLock(m_mapMutex);

        ++m_vegetationUpdateCount;
        m_vegetation[vegetationInstance] = { aabb.GetCenter(), aabb };
    }

    void StaticVegetationSystemComponent::InstanceRemoved(UniqueVegetationInstancePtr vegetationInstance, AZ::Aabb aabb)
    {
        AZStd::unique_lock<decltype(m_mapMutex)> mapScopeLock(m_mapMutex);

        ++m_vegetationUpdateCount;
        m_vegetation.erase(vegetationInstance);
    }

    void StaticVegetationSystemComponent::VegetationCleared()
    {
        AZStd::unique_lock<decltype(m_mapMutex)> mapScopeLock(m_mapMutex);

        ++m_vegetationUpdateCount;
        m_vegetation.clear();
    }

    MapHandle StaticVegetationSystemComponent::GetStaticVegetation()
    {
        if (m_vegetationUpdateCount != m_vegetationCopyUpdateCount)
        {
            AZStd::unique_lock<decltype(m_mapMutex)> mapScopeLock(m_mapMutex);
            AZStd::unique_lock<decltype(m_mapCopyReadMutex)> mapCopyScopeLock(m_mapCopyReadMutex);

            if (m_vegetationUpdateCount != m_vegetationCopyUpdateCount) // check again after lock in case we waited for another thread to finish updating.
            {
                m_vegetationReturnCopy = m_vegetation;
                m_vegetationCopyUpdateCount = m_vegetationUpdateCount;
            }
        }

        return MapHandle{ &m_vegetationReturnCopy, m_vegetationCopyUpdateCount, &m_mapCopyReadMutex }; // need to do this outside the read scope lock above as the MapHandle locks the ReadMutex on construction
    }
}