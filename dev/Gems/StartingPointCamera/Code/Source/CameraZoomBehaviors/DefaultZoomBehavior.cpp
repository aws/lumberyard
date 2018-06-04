#include "StartingPointCamera_precompiled.h"
#include "DefaultZoomBehavior.h"
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Camera
{
    float const DefaultZoomBehavior::k_defaultTimeToZoom = 1.0f;
    float const DefaultZoomBehavior::k_defaultZoom = 1.0f;
		
	void DefaultZoomBehavior::Reflect( AZ::ReflectContext* reflection )
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>( reflection );
		if( serializeContext )
		{
			serializeContext->Class< DefaultZoomBehavior >()
				->Version( 1 )
                ->Field("ZoomOnActivate", &DefaultZoomBehavior::m_zoomOnActivate)
                ->Field("TimeToZoom", &DefaultZoomBehavior::m_timeToZoom)
                ->Field("ZoomTarget", &DefaultZoomBehavior::m_zoomTarget)
                ;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if( editContext )
			{
				editContext->Class<DefaultZoomBehavior>( "Default zoom", "Handles augmenting zoom based on overrides set on components" )
					->ClassElement( AZ::Edit::ClassElements::EditorData, "" )
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DefaultZoomBehavior::m_zoomOnActivate,
                        "Zoom on Activate", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DefaultZoomBehavior::m_timeToZoom, 
                        "Time to Zoom", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DefaultZoomBehavior::m_zoomTarget,
                        "Zoom Target", "")
                    ;
			}
		}
	}

	void DefaultZoomBehavior::Activate( AZ::EntityId entityId )
	{
        CameraZoomBehaviorBus::Handler::BusConnect(entityId);
        if (m_zoomOnActivate)
        {
		    AZ::TickBus::Handler::BusConnect();
        }
	}

	void DefaultZoomBehavior::Deactivate()
	{
        CameraZoomBehaviorBus::Handler::BusDisconnect();
		AZ::TickBus::Handler::BusDisconnect();
	}

	void DefaultZoomBehavior::ModifyZoom( const AZ::EntityId& targetEntityId, float& inOutZoom )
	{
		inOutZoom *= m_zoom;
	}

	void DefaultZoomBehavior::OnTick( float deltaTime, AZ::ScriptTimePoint /*time*/ )
	{
		AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Game);

		if( !AZ::IsClose( m_zoom, m_zoomTarget, FLT_EPSILON ) )
		{
			m_zoomVelocity = ( m_zoomTarget - m_zoom ) / m_timeToZoom;

			// Assuming that this is just a normal update that won't reach the target, what is the new value?
			float newZoom	= m_zoom + ( deltaTime * m_zoomVelocity );

			// Have we reached the target?
			if( ( m_zoomVelocity > 0.0f && newZoom >= m_zoomTarget ) ||
				( m_zoomVelocity < 0.0f && newZoom <= m_zoomTarget ) )
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

    void DefaultZoomBehavior::SetZoomTarget(float zoomTarget)
    {
        m_zoomTarget = zoomTarget;
    }

    void DefaultZoomBehavior::SetTimeToZoom(float timeToZoom)
    {
        m_timeToZoom = timeToZoom;
    }

    void DefaultZoomBehavior::StartZoom()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void DefaultZoomBehavior::StopZoom()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }
}