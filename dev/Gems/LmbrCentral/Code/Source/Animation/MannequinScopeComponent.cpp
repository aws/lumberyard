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
#include "MannequinScopeComponent.h"

#include <ISystem.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <ICryMannequin.h>
#include <AzCore/Component/Entity.h>
#include <LmbrCentral/Animation/MannequinComponentBus.h>

namespace LmbrCentral
{
    void MannequinScopeComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<MannequinScopeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Animation Database", &MannequinScopeComponent::m_animationDatabase)
                ->Field("Context Name", &MannequinScopeComponent::m_scopeContextName)
                ->Field("Target Entity", &MannequinScopeComponent::m_targetEntityId);
        }
    }

    void MannequinScopeComponent::ResetMannequinScopeContext()
    {
        EBUS_EVENT_ID(GetEntityId(), MannequinRequestsBus, SetScopeContext, m_scopeContextName.c_str(),
            m_targetEntityId.IsValid() ? m_targetEntityId : GetEntityId(), m_animationDatabase.GetAssetPath().c_str());
    }

    void MannequinScopeComponent::ClearMannequinScopeContext()
    {
        EBUS_EVENT_ID(GetEntityId(), MannequinRequestsBus, ClearScopeContext, m_scopeContextName.c_str());
    }

    void MannequinScopeComponent::Activate()
    {
        ResetMannequinScopeContext();
        const AZ::EntityId attachToId = m_targetEntityId.IsValid() ? m_targetEntityId : GetEntityId();
        MeshComponentNotificationBus::Handler::BusConnect(attachToId);
    }

    void MannequinScopeComponent::Deactivate()
    {
        MeshComponentNotificationBus::Handler::BusDisconnect();
        ClearMannequinScopeContext();
    }

    void MannequinScopeComponent::OnMeshDestroyed()
    {
        ClearMannequinScopeContext();
    }

    void MannequinScopeComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/)
    {
        ResetMannequinScopeContext();
    }

    //////////////////////////////////////////////////////////////////////////
} // namespace LmbrCentral
