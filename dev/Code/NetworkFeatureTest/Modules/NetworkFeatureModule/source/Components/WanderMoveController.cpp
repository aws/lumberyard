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
#include "StdAfx.h"
#include "Components/WanderMoveController.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Serialization/EditContext.h>

namespace NetworkFeatureTest
{
    WanderMoveController::WanderMoveController()
        : m_heading(AZ::Vector3::CreateAxisX())
    {
    }

    void WanderMoveController::Reflect(AZ::ReflectContext* context)
    {
        if (context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<WanderMoveController, AZ::Component>()
                    ->SerializerForEmptyClass();

                AZ::EditContext* editContext = serializeContext->GetEditContext();

                if (editContext)
                {
                    editContext->Class<WanderMoveController>(
                        "WanderMoveController", "Randomly moves the entity around on the horizontal plane.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "NetworkFeatureTest")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"));
                }
            }
        }
    }

    void WanderMoveController::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void WanderMoveController::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void WanderMoveController::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AZ::Transform xform;
        static AZ::SimpleLcgRandom randomGenerator;
        AZ::Quaternion rot = AZ::Quaternion::CreateRotationZ(randomGenerator.GetRandomFloat() / 2.f - 0.25f);
        m_heading = (rot * m_heading).GetNormalized();
        EBUS_EVENT_ID_RESULT(xform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        xform.SetPosition(xform.GetPosition() + m_heading * deltaTime);
        EBUS_EVENT_ID(GetEntityId(), AZ::TransformBus, SetWorldTM, xform);
    }
}
