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
#include "StartingPointCamera_precompiled.h"
#include "SampleZoomBehavior.h"
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Camera
{
    float const SampleZoomBehavior::DefaultTimeToZoom = 1.0f;
    float const SampleZoomBehavior::DefaultZoom = 1.0f;

    void SampleZoomBehavior::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if(serializeContext)
        {
            serializeContext->Class<SampleZoomBehavior>()
                ->Version( 1 )
                ->Field("ZoomOnActivate", &SampleZoomBehavior::m_zoomOnActivate)
                ->Field("TimeToZoom", &SampleZoomBehavior::m_timeToZoom)
                ->Field("ZoomTarget", &SampleZoomBehavior::m_zoomTarget)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if( editContext )
            {
                editContext->Class<SampleZoomBehavior>( "Sample Zoom", "Demonstrates an example of an implemented zoom behavior." )
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SampleZoomBehavior::m_zoomOnActivate,
                        "Zoom on Activate", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SampleZoomBehavior::m_timeToZoom, 
                        "Time to Zoom", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SampleZoomBehavior::m_zoomTarget,
                        "Zoom Target Value", "")
                    ;
            }
        }
    }

    void SampleZoomBehavior::Activate(AZ::EntityId entityId)
    {
        CameraZoomBehaviorRequestBus::Handler::BusConnect(entityId);
        if (m_zoomOnActivate)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SampleZoomBehavior::Deactivate()
    {
        CameraZoomBehaviorRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SampleZoomBehavior::ModifyZoom(const AZ::Transform& targetTransform, float& inOutZoom)
    {
        inOutZoom *= m_zoom;
    }

    void SampleZoomBehavior::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Game);

        if(!AZ::IsClose(m_zoom, m_zoomTarget, FLT_EPSILON))
        {
            if (m_zoomVelocity == 0.0f)
            {
                m_zoomVelocity = (m_zoomTarget - m_zoom) / m_timeToZoom;
            }

            // Assuming that this is just a normal update that won't reach the target, what is the new value?
            float newZoom = m_zoom + (deltaTime * m_zoomVelocity);

            // Have we reached the target?
            if( (m_zoomVelocity > 0.0f && newZoom >= m_zoomTarget) ||
                (m_zoomVelocity < 0.0f && newZoom <= m_zoomTarget) )
            {
                m_zoom = m_zoomTarget;
                AZ::TickBus::Handler::BusDisconnect();
            }
            else
            {
                m_zoom = newZoom;
            }
        }
    }

    void SampleZoomBehavior::SetZoomTarget(float zoomTarget)
    {
        m_zoomTarget = zoomTarget;
    }

    void SampleZoomBehavior::SetTimeToZoom(float timeToZoom)
    {
        if (timeToZoom > 0.0f)
        {
            m_timeToZoom = timeToZoom;
        }
    }

    void SampleZoomBehavior::StartZoom()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void SampleZoomBehavior::StopZoom()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }
} 
